/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Bluetooth.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *************************************************************************/

#define LOGLEVEL LogTrace

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Bluetooth.h"
#undef PRIVATE_INCLUDE

#ifdef DISABLE_BLUETOOTH

static Boolean connected = false;

Err f_BTInit (Globals *g) {}
void f_BTClose (Globals *g) {}
Err f_BTListenInit (Globals *g) {}
Err f_BTWait (Globals *g) {}
void f_BTListenClose (Globals *g) {}
Boolean f_BTHandleEvent (Globals *g, EventType *e) {}

Err f_BTConnect (Globals *g) {
    connected = true;
}

void f_BTDisconnect (Globals *g) {
    connected = false
}

Boolean f_BTIsConnected (Globals *g) {
    return connected;
}

Err f_BTSelectDevice (Globals *g) {}
MemHandle f_BTDataReady (Globals *g) {}

#else

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Number of ticks to wait before trying to reconnect.  The Bluetooth
 * library sets a 20-second L2CAP timeout by default; we can not
 * override this directly, but we can set a timer to close the
 * connection explicitly.
 *
 * Note that newer devices such as the Treo 650 have an improved
 * Bluetooth library that is capable of buffering a series of packets
 * internally.  This is great for bandwidth, but it means the library
 * reports that packets are sent successfully, even if they are still
 * in the internal queue.  This defeats our application-level timeout
 * system.  The infrastructure does not hurt, though, so let's leave
 * it enabled for the Treo as well.  */
#define SEND_TIMEOUT		(3 * SysTicksPerSecond())

/* Define as 1 to enable Human Interface Device emulation.  */
#define EMULATE_HID 1

/* SDP service name.  */
#define SDP_SERVICE_NAME    "BlueRemote"

/* SDP service description.  */
#define SDP_SERVICE_DESC    "HID Mouse and Keyboard"

/* SDP provider name. */
#define SDP_PROVIDER_NAME   "BlueRemote"

/* HID device classes.  */
#define HID_DEVICE_CLASS_BASE			\
    (btLibCOD_LimitedDiscoverableMode           \
     | btLibCOD_Major_Peripheral)

#define HID_DEVICE_SUBCLASS_KEYBOARD		\
     btLibCOD_Minor_Peripheral_Keyboard

#define HID_DEVICE_SUBCLASS_MOUSE		\
     btLibCOD_Minor_Peripheral_Pointing

#define HID_DEVICE_SUBCLASS_COMBO		\
    btLibCOD_Minor_Peripheral_Combo

#define HID_DEVICE_SUBCLASS_MOUSEKEYBOARD	\
    (btLibCOD_Minor_Peripheral_Keyboard		\
     | btLibCOD_Minor_Peripheral_Pointing)


/* Value to override accessible mode with.  */
#define HID_ACCESSIBLE      btLibDiscoverableAndConnectable

/* The protocol UUID for HID, from the BT Assigned Numbers document.  */
#define HIDProtocolUUID     0x0011

/* The Service Class UUID for HID, from the BT Assigned Numbers document.  */
#define HIDServiceClassUUID 0x1124

/* PSM identifier of the HID control channel.  */
#define L2CAP_PSM_HIDP_CTRL 0x11

/* PSM identifier of the HID interrupt channel.  */
#define L2CAP_PSM_HIDP_INTR 0x13

/* Maximum Transmission Units.  */
#define HIDP_MINIMUM_MTU 48
#define HIDP_DEFAULT_MTU 48

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct BTChannel {
    Globals *g;
    char *name;
    UInt16 psm;

    Boolean connected;
    BtLibSocketRef listenerSocket;
    BtLibSocketRef dataSocket;

    MsgQueuePtr inqueue;
    MsgQueuePtr outqueue;

    Timer sendTimer;

    UInt8 *data;

    Err status;
} BTChannel;

typedef struct BTGlobals {

    /* The reference number of the Bluetooth library.  */
    UInt16 btLibRefNum;

    /* The HID control channel.  */
    BTChannel control;

    /* The HID interrupt channel.  */
    BTChannel interrupt;

    /* The SDP service record.  */
    BtLibSdpRecordHandle SDPRecord;

    /* Status flags for keeping track of Bluetooth status.  */
    Boolean libLoaded;          /* True if we needed to load the lib.  */
    Boolean libOpened;		/* True if the Bluetooth library is open. */

    /* True if we have initiated a connect.  */
    Boolean connecting;

    Boolean connected;

    Boolean disconnecting;

    /* Original value for the Bluetooth device class.  */
    BtLibClassOfDeviceType origClass;

    /* Original value for Bluetooth accessibility mode.  */
    BtLibAccessibleModeEnum origAccessible;
} BTGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static UInt32 BTSendTimer (Globals *g, UInt32 userdata)
    BLUETOOTH_SECTION;

/* Bluetooth management callback function.  */
static void BTManagementCallback (BtLibManagementEventType *eventP,
                                  Globals *g)
    BLUETOOTH_SECTION;

/* Bluetooth socket callback function.  */
static void BTSocketCallback (BtLibSocketEventType *e, BTChannel *c)
    BLUETOOTH_SECTION;

/* Message queue callback function.  */
static void BTMsgQueueCallback (Globals *g, MsgQueuePtr q, Boolean full)
    BLUETOOTH_SECTION;

static void DataCleanup (Globals *g, BTChannel *c)
    BLUETOOTH_SECTION;

/* Handle an updated device name.  */
static void HandleNameResult (Globals *g,
                              BtLibDeviceAddressType *addr,
                              BtLibFriendlyNameType *name)
    BLUETOOTH_SECTION;

/* Look up the name of the remote device with address ADDR.  */
static void UpdateRemoteName (Globals *g,
                              BtLibDeviceAddressType *addr,
                              Boolean cacheOnly)
    BLUETOOTH_SECTION;

/* Setup a new remote address.  Adds a new element to the DevList,
   sets it as selected and initiates a name query.  */
static void SetupNewAddress (Globals *g,
                             BtLibDeviceAddressType *a)
    BLUETOOTH_SECTION;

/* Start listening on a channel.  */
static Err Listen (Globals *g, BTChannel *c, UInt16 psm)
    BLUETOOTH_SECTION;

static void Masquarade (Globals *g)
    BLUETOOTH_SECTION;

static Err LinkConnect (Globals *g, BtLibDeviceAddressType *addr)
    BLUETOOTH_SECTION;

static void LinkDisconnect (Globals *g, Device *d)
    BLUETOOTH_SECTION;

static Err Authenticate (Globals *g, BtLibDeviceAddressType *addr)
    BLUETOOTH_SECTION;

static Err Encrypt (Globals *g, BtLibDeviceAddressType *addr)
    BLUETOOTH_SECTION;

/* Initiate a connection attempt to the selected device on a channel.  */
static Err Connect (Globals *g, BTChannel *c, Device *d, UInt16 psm)
    BLUETOOTH_SECTION;

static void Disconnect (Globals *g, BTChannel *c, Boolean quick)
    BLUETOOTH_SECTION;

/* Send next queued message, if available.  */
static MemHandle SendNext (Globals *g, BTChannel *c)
    BLUETOOTH_SECTION;

/* Add the ATTR SDP raw attribute starting at DATA and of size LENGTH.  */
static Err AddSDPAttribute (Globals *g, UInt16 attr,
                            UInt8 *data, UInt16 length)
    BLUETOOTH_SECTION;

/* (Re)publish the SDP service record.  */
static Err PublishSDPServiceRecord (Globals *g)
    BLUETOOTH_SECTION;

/* Wait in an internal event loop until an AsyncResult command of the
 * given kind is put in the command queue, or until the timeout is
 * reached.  TIMEOUT should be the number of ticks to wait for;
 * specify 0 for no timeout.
 *
 * Returns appErrProgressCanceled if the progress was canceled.
 * Returns appErrTimeout if the given timeout elapsed.
 */
static Err WaitForResult (Globals *g, AsyncKind kind,
			  Boolean cancelable, UInt16 timeout)
    BLUETOOTH_SECTION;

static Err WaitForCommand (Globals *g, Command cmd,
			   Boolean cancelable, UInt16 timeout)
    BLUETOOTH_SECTION;

/* Wait for HANDLE to be sent on C, popping up a progress dialog with
   message MSG.  Don't wait for more than TIMEOUT ticks.
   Returns zero if HANDLE was sent successfully, non-zero (an error
   code) otherwise.  */
static Err WaitForQueue (Globals *g, BTChannel *c, MemHandle handle,
			 Boolean cancelable, UInt16 timeout)
    BLUETOOTH_SECTION;


/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_btLibRefNum g->bt->btLibRefNum
#define g_origClass g->bt->origClass
#define g_origAccessible g->bt->origAccessible
#define g_control g->bt->control
#define g_interrupt g->bt->interrupt
#define g_SDPRecord g->bt->SDPRecord
#define g_ctrAccessibilityChange g->bt->ctrAccessibilityChange
#define g_oldAccessibilityChange g->bt->oldAccessibilityChange
#define g_libLoaded g->bt->libLoaded
#define g_libOpened g->bt->libOpened
#define g_connecting g->bt->connecting
#define g_connected g->bt->connected
#define g_disconnecting g->bt->disconnecting

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static UInt32
BTSendTimer (Globals *g, UInt32 userdata)
{
    LOG ("Send timed out");
    //ENQUEUE (GetAsyncResult (AsyncSend, appErrTimeout));
    ENQUEUE (CmdRadioError);
    return 0;
}



static void
BTManagementCallback (BtLibManagementEventType *eventP, Globals *g)
{
    if (!g || !g->bt) return;

    TRACE ("BTManagementCallback");

    switch (eventP->event) {
	case btLibManagementEventRadioState:
	    /* The radio state has changed.  */
	    switch (eventP->status) {
		case btLibErrRadioInitialized:
		    /* No problem.  */
		    LOG ("Radio initialized successfully");
		    break;

		case btLibErrRadioInitFailed:
		    LOG ("Radio failed to initialize");
		    break;

		case btLibErrRadioFatal:
		    LOG ("Fatal radio error");
		    break;

		case btLibErrRadioSleepWake:
		    LOG ("Radio reset after sleep");
		    break;

		default:
		    break;
	    }
	    ENQUEUE (GetAsyncResult (AsyncRadio, eventP->status));
	    EvtWakeup();
	    break;

	case btLibManagementEventAccessibilityChange:
	    switch (eventP->eventData.accessible) {
		case btLibNotAccessible:
		    DEBUG ("Device is now inaccessible");
		    break;
		case btLibConnectableOnly:
		    DEBUG ("Device is now connectable");
		    break;
		case btLibDiscoverableAndConnectable:
		    DEBUG ("Device is now discoverable and connectable");
		    break;
		default:
		    LOG ("Device has unknown accessibility");
		    break;
	    }
	    ENQUEUE (GetAsyncResult (AsyncAccessibleMode, 0));
	    EvtWakeup();
	    break;

	case btLibManagementEventLocalNameChange:
	    TRACE ("Local name change");
	    break;

	case btLibManagementEventNameResult:
	    /* A remote device name request has completed.  */
	    if (eventP->status == 0)
		HandleNameResult (g, &eventP->eventData.nameResult.bdAddr,
				  &eventP->eventData.nameResult.name);
	    else
		LOG ("Error looking up name: %s", StrError (eventP->status));
	    break;

	case btLibManagementEventACLConnectOutbound:
	{
	    /* An ACL link has been established with a remote device.  */
	    if (eventP->status) {
		LOG ("ACL link failed: %s", StrError (eventP->status));
		ENQUEUE (GetAsyncResult (AsyncACLConnect, eventP->status));
		EvtWakeup();
	    }
	    else {
		Device *d = DevListFindByAddress (&eventP->eventData.bdAddr);
		if (!d || d != DevListSelected()) {
		    LOG ("Unexpected ACL connection with %s",
			 (d ? d->nameStr : "an unknown device"));
		    /* Ignore.  */
		}
		else {
		    INFO ("ACL link established");
		    ENQUEUE (GetAsyncResult (AsyncACLConnect, 0));
		    EvtWakeup();
		}
	    }
	    break;
	}
	case btLibManagementEventACLConnectInbound:
	{
	    /* An ACL link has been established by a remote device.  */
	    Device *d = DevListFindByAddress (&eventP->eventData.bdAddr);
	    TRACE ("ACLConnectInbound");
	    INFO ("Contact from %s", (d ? d->nameStr : "a new device"));
	    break;
	}
	case btLibManagementEventACLDisconnect:
	{
	    /* An ACL link has been disconnected.  */
	    TRACE ("ACLDisconnect");
	    if (eventP->status == btLibMeStatusUserTerminated
		|| eventP->status == btLibMeStatusLocalTerminated)
		INFO ("Link disconnected");
	    else
		INFO ("Link closed: %s", StrError (eventP->status));
	    ENQUEUE (GetAsyncResult (AsyncACLDisconnect, eventP->status));
	    EvtWakeup();
	    break;
	}
	case btLibManagementEventInquiryResult:
	    /* A device has been found.  */
	    TRACE ("Inquiry result");
	    break;

	case btLibManagementEventInquiryComplete:
	    /* The inquiry has finished.  */
	    TRACE ("Inquiry complete");
	    break;

	case btLibManagementEventInquiryCanceled:
	    /* The inquiry has been canceled.  */
	    TRACE ("Inquiry canceled");
	    break;

	case btLibManagementEventPasskeyRequest:
	{
	    Device *d = DevListFindByAddress (&eventP->eventData.bdAddr);
	    TRACE ("PasskeyRequest");
	    INFO ("Pairing initiated with %s",
		  (d ? d->nameStr : "a new device"));
	    break;
	}
	case btLibManagementEventPasskeyRequestComplete:
	{
	    Device *d = DevListFindByAddress (&eventP->eventData.bdAddr);
	    TRACE ("PasskeyRequestComplete");
	    if (eventP->status) {
		INFO ("Pairing with %s canceled",
		      (d ? d->nameStr : "new device"));
	    }
	    else INFO ("Got passkey for %s",
		       (d ? d->nameStr : "new device"));
	    break;
	}
	case btLibManagementEventPairingComplete:
	{
	    TRACE ("PairingComplete");
	    /* It seems the bdAddr field is never set in this event.  */
	    INFO ("Pairing successful");
	    ENQUEUE (CmdPaired);
	    EvtWakeup();
	    break;
	}
	case btLibManagementEventAuthenticationComplete:
	{
	    Device *d = DevListFindByAddress (&eventP->eventData.bdAddr);
	    d = d;
	    TRACE ("AuthenticationComplete");
	    if (eventP->status == btLibErrNoError)
		DEBUG ("Authenticated with %s",
		      (d ? d->nameStr : "new device"));
	    else if (eventP->status == btLibErrCanceled)
		LOG ("Authentication canceled");
	    else if (eventP->status == btLibMeStatusCommandDisallowed)
		LOG ("Authentication denied by %s",
		     (d ? d->nameStr : "remote device"));
	    else
		LOG ("Authentication failed: %s", StrError (eventP->status));
	    ENQUEUE (GetAsyncResult (AsyncAuthenticate, eventP->status));
	    EvtWakeup();
	    break;
	}
	case btLibManagementEventEncryptionChange:
	{
	    TRACE ("EncryptionChange");
	    if (eventP->eventData.encryptionChange.enabled) {
		INFO ("Encryption enabled");
		ENQUEUE (GetAsyncResult (AsyncEncrypt, 0));
		EvtWakeup();
	    }
	    else {
		INFO ("Encryption disabled");
		ENQUEUE (GetAsyncResult (AsyncEncrypt,
					 appErrEncryptionFailed));
		EvtWakeup();
	    }
	    break;
	}
	default :
	    /* Unknown event.  */
	    DEBUG ("Unknown management event %x", eventP->event);
	    break;
    }
}



static void
BTSocketCallback (BtLibSocketEventType *e, BTChannel *c)
{
    Err err;
    Globals *g;

    if (!e || !c) return;
    g = c->g;
    if (!g || !g->bt) return;

    TRACE ("BTSocketCallback %s", c->name);

    switch (e->event) {

	case btLibSocketEventConnectRequest:
	{   /* A remote device has requested a connection.  */
	    Boolean accept = true;
	    Device *d =
		DevListFindByAddress (&e->eventData.requestingDevice);

	    TRACE ("btLibSocketEventConnectRequest");
	    if (c->listenerSocket == -1) {
		INFO ("%s connection request when not listening",
		      c->name);
		accept = false;
	    }
	    else if (g_disconnecting) {
		INFO ("%s connection request while disconnecting", c->name);
		accept = false;
	    }
	    else if (c->dataSocket != -1) {
		INFO ("%s connection request from %s denied: "
		      "Already connected", c->name,
		      (d ? d->nameStr : "unknown device"));
		accept = false;
	    }
#if 0
	    else if (g_connecting) {
		INFO ("%s connection request from %s denied: "
		      "Wrong direction", c->name,
		      (d ? d->nameStr : "unknown device"));
		accept = false;
	    }
#endif
	    else {                  /* No data connection yet. */
		INFO ("%s connection request from %s", c->name,
		      (d ? d->nameStr : "unknown device"));
		ProgressSelectAnimation (AnimationChannelBackward);
		SetupNewAddress (g, &e->eventData.requestingDevice);
		d = DevListSelected();
	    }

	    /* Allow or disallow the request.  */
	    err = BtLibSocketRespondToConnection (g_btLibRefNum,
						  c->listenerSocket,
						  accept);
	    if (err && err != btLibErrPending) {
		LOG ("%s while responding to %s connection from %s",
		     StrError (err), c->name,
		     (d ? d->nameStr : "unknown device"));
		break;
	    }
	    break;
	}

	case btLibSocketEventConnectedInbound:
	    /* Remote device connected.  */
	    TRACE ("btLibSocketEventConnectedInbound");

	    if (e->status) {
		INFO ("%s connection failed: %s",
		      c->name, StrError (e->status));
		ENQUEUE (GetAsyncResult (AsyncConnect, e->status));
		EvtWakeup();
		break;
	    }
	    else if (c->listenerSocket == -1) {
		LOG ("Inbound %s connection when not listening", c->name);
		BtLibSocketClose (g_btLibRefNum, e->eventData.newSocket);
	    }
	    else if (c->dataSocket != -1) {
		LOG ("Attempt to reconnect %s channel", c->name);
		BtLibSocketClose (g_btLibRefNum, e->eventData.newSocket);
	    }
	    else if (g_disconnecting) {
		LOG ("New %s connection while disconnecting", c->name);
		BtLibSocketClose (g_btLibRefNum, e->eventData.newSocket);
	    }
	    else {
		INFO ("%s channel connected", c->name);
		c->dataSocket = e->eventData.newSocket;
		c->connected = true;
		ENQUEUE (GetAsyncResult (AsyncConnect, e->status));
		EvtWakeup();
	    }
	    break;

	case btLibSocketEventConnectedOutbound:
	    TRACE ("btLibSocketEventConnectedOutbound");
	    if (e->status) {
		DEBUG ("Failed to connect %s channel: %s",
		       c->name, StrError (e->status));
	    }
	    else {
		DEBUG ("Connected to %s channel", c->name);
		c->connected = true;
	    }
	    ENQUEUE (GetAsyncResult (AsyncConnect, e->status));
	    EvtWakeup();
	    break;

	case btLibSocketEventDisconnected:
	    /* Remote device disconnected.  */
	    TRACE ("btLibSocketEventDisconnected");
	    if (e->socket != c->dataSocket) {
		LOG ("Disconnect on unexpected %s socket", c->name);
		BtLibSocketClose (g_btLibRefNum, e->socket);
		break;
	    }
	    INFO ("%s channel disconnected: %s",
		  c->name, StrError (e->status));
	    BtLibSocketClose (g_btLibRefNum, e->socket);
	    DataCleanup (g, c);
	    c->dataSocket = -1;
	    c->connected = false;
	    g_connected = false;
	    if (e->status == btLibL2DiscUserRequest) {
		ENQUEUE (CmdInitiateDisconnect);
		EvtWakeup();
	    }
	    else if (e->status == btLibL2DiscLinkDisc)
		;		/* Let the management callback enqueue
				 * the correct command. */
	    else {
		ENQUEUE (CmdConnectionError);
		EvtWakeup();
	    }
	    break;

	case btLibSocketEventData:  /* Data received.  */
	{
	    MemHandle handle;
	    UInt8 *d;
	    if (e->socket != c->dataSocket) {
		LOG ("Data received on unexpected %s socket", c->name);
		DUMP (LogDebug, c->name,
		      e->eventData.data.data, e->eventData.data.dataLen);
		break;
	    }
	    handle = CheckedMemHandleNew (e->eventData.data.dataLen, true);
	    d = MemHandleLock (handle);
	    MemMove (d, e->eventData.data.data, e->eventData.data.dataLen);
	    MemPtrUnlock (d);
	    MsgAddHandle (c->inqueue, handle);
	    DEBUG ("Data received on %s socket", c->name);
	    MDUMP (LogDebug, handle, c->name,
		   e->eventData.data.data, e->eventData.data.dataLen);

	    /* Process incoming messages on our L2CAP sockets.  */
	    HIDPProcessMessages();
	    EvtWakeup();
	    break;
	}

	case btLibSocketEventSendComplete:
	    if (e->status) {
		MLOG ((c->data ? MemPtrRecoverHandle (c->data) : 0),
		      "Error sending data on %s channel: %s",
		      c->name, StrError (e->status));
		c->status = e->status;
		DataCleanup (g, c);
		ENQUEUE (GetAsyncResult (AsyncSend, e->status));
		EvtWakeup();
	    }
	    else {
		if (c->data != 0) {
		    MDEBUG (MemPtrRecoverHandle (c->data),
			    "Data sent on %s channel (%d bytes)",
			    c->name, e->eventData.data.dataLen);
		    DataCleanup (g, c);
		    c->status = 0;
		}
		else {
		    LOG ("Unexpected data sent on %s channel (%d bytes)",
			 c->name, e->eventData.data.dataLen);
		}
		SendNext (g, c);
		ENQUEUE (GetAsyncResult (AsyncSend, e->status));
		EvtWakeup();
	    }
	    break;

	default:
	    DEBUG ("Unknown socket event %x", e->event);
	    break;
    }
}



static void
BTMsgQueueCallback (Globals *g, MsgQueuePtr q, Boolean full)
{
    BTChannel *c;
    if (!g || !g->bt) return;

    if (full)
        /* Sorry, we can't speed this up.  */
        return;

    if (q == g_control.outqueue)
        c = &g_control;
    else if (q == g_interrupt.outqueue)
        c = &g_interrupt;
    else {
        DEBUG ("BTMsgQueueCallback called on unknown queue");
        return;
    }

    if (!c->connected)
        MsgFlush (q);
    else
        SendNext (g, c);
}



static void
DataCleanup (Globals *g, BTChannel *c)
{
    if (c->data) {
        MemHandle handle;
        TRACE ("DataCleanup");
	handle = MemPtrRecoverHandle (c->data);
        if (c->data) {
            MemPtrUnlock (c->data);
            MemHandleFree (handle);
            c->data = 0;
	    c->status = 0;
            MsgProcessed (c->outqueue, handle);
        }
    }
    TimerCancel (c->sendTimer);
}



static void
HandleNameResult (Globals *g,
                  BtLibDeviceAddressType *addr,
                  BtLibFriendlyNameType *name)
{
    Device *d = DevListFindByAddress (addr);
    if (!d) {
        LOG ("Name result for unknown device");
        return;
    }
    d->name.nameLength = MIN (BLUETOOTH_NAME_SIZE - 1, StrLen (name->name));
    d->name.name = d->nameStr;

    if (name->name != (UInt8 *)d->nameStr)
        StrNCopy (d->nameStr, name->name, d->name.nameLength);

    /* Make sure the name is NULL-terminated.  */
    d->nameStr[d->name.nameLength] = 0;

    /* TODO: Conversion from UTF-8 to Palm coding.  */

    DEBUG ("%s is now called %s", d->addressStr, d->nameStr);
    DevListUpdate (d);
}



static void
UpdateRemoteName (Globals *g,
                  BtLibDeviceAddressType *addr,
                  Boolean cacheOnly)
{
    Err err;
    Device *d = DevListFindByAddress (addr);
    if (!d) {
        LOG ("Can not update name of unknown device");
        return;
    }

    /* By default, use the address as the name of the device.  */
    err = BtLibAddrBtdToA (g_btLibRefNum, addr,
                           d->nameStr, BLUETOOTH_NAME_SIZE);
    if (err != btLibErrNoError) {
        LOG ("Error converting device address: %s", StrError (err));
        return;
    }

    /* Get the user-friendly name of the device.  */
    d->name.name = d->nameStr;
    d->name.nameLength = BLUETOOTH_NAME_SIZE - 1;
    err = BtLibGetRemoteDeviceName (g_btLibRefNum,
                                    addr, &d->name,
                                    (cacheOnly
                                     ? btLibCachedOnly
                                     : btLibCachedThenRemote));
    DevListUpdate (d);
    switch (err) {
	case btLibErrNoError:
	    HandleNameResult (g, addr, &d->name);
	    break;

	case btLibErrPending:
	    /* Let the manager callback call HandleNameResult later.  */
	    break;

	default:
	    DEBUG ("Error getting device name: %s", StrError (err));
	    break;
    }
    return;
}



static void
SetupNewAddress (Globals *g, BtLibDeviceAddressType *a)
{
    Err err;
    Device dev;

    MemMove (&dev.address, a, sizeof (BtLibDeviceAddressType));

    dev.version = DEVICE_STRUCT_VERSION;
    /* Get an ASCII rendering of the device address.  */
    err = BtLibAddrBtdToA (g_btLibRefNum,
                           &dev.address,
                           dev.addressStr,
                           BLUETOOTH_ADDR_SIZE);
    if (err != btLibErrNoError)
        ERROR ("Error converting device address: %s", StrError (err));
    dev.addressStr[BLUETOOTH_ADDR_SIZE - 1] = 0;
    dev.nameStr[0] = 0;
    dev.name.name = dev.nameStr;
    dev.name.nameLength = 0;
    dev.lastConnected = TimGetSeconds();
    dev.encryptLink = false;
    DevListAdd (&dev);
    UpdateRemoteName (g, a, false);
}



static Err
Listen (Globals *g, BTChannel *c, UInt16 psm)
{
    Err err;
    BtLibSocketListenInfoType listenInfo;

    TRACE ("Listen %d (%s)", psm, c->name);

    c->psm = psm;
    c->g = g;
    if (c->listenerSocket)
        BtLibSocketClose (g_btLibRefNum, c->listenerSocket);
    err = BtLibSocketCreate (g_btLibRefNum,
                             &c->listenerSocket,
                             (BtLibSocketProcPtr) BTSocketCallback,
                             (UInt32) c,
                             btLibL2CapProtocol);
    if (err) {
        ERROR ("Couldn't create %s socket: %s", c->name, StrError (err));
        return err;
    }

    MemSet (&listenInfo, sizeof (listenInfo), 0);
    listenInfo.data.L2Cap.localPsm = psm;
    listenInfo.data.L2Cap.localMtu = HIDP_DEFAULT_MTU;
    listenInfo.data.L2Cap.minRemoteMtu = HIDP_MINIMUM_MTU;

    err = BtLibSocketListen (g_btLibRefNum,
                             c->listenerSocket,
                             &listenInfo);
    if (err) {
        ERROR ("Couldn't listen on %s socket: %s",
               c->name, StrError (err));
        BtLibSocketClose (g_btLibRefNum, c->listenerSocket);
        c->listenerSocket = -1;
        return err;
    }

    return 0;
}



static void
Masquarade (Globals *g)
{
    Err err;
    BtLibClassOfDeviceType newClass;

    TRACE ("Masquarade");
    ASSERT (g && g->bt);

    if (!SPREF (masquarade)) {
	DEBUG ("Masquarading disabled");
	return;
    }

    newClass = HID_DEVICE_CLASS_BASE;
    switch (SPREF (deviceSubclass)) {
	case classKeyboard:
	    newClass |= HID_DEVICE_SUBCLASS_KEYBOARD;
	    DEBUG ("Masquarading as a keyboard");
	    break;
	case classMouse:
	    newClass |= HID_DEVICE_SUBCLASS_MOUSE;
	    DEBUG ("Masquarading as a mouse");
	    break;
	case classCombo:
	    newClass |= HID_DEVICE_SUBCLASS_COMBO;
	    DEBUG ("Masquarading as a combo device");
	    break;
	default:
	case classMouseKeyboard:
	    newClass |= HID_DEVICE_SUBCLASS_MOUSEKEYBOARD;
	    DEBUG ("Masquarading as mouse/keyboard");
	    break;
    }

    /* Set special device class.  */
    err = BtLibSetGeneralPreference (g_btLibRefNum,
                                     btLibPref_LocalClassOfDevice,
                                     &newClass,
                                     sizeof (BtLibClassOfDeviceType));
    switch (err) {
	case btLibErrNoError:
	    DEBUG ("Device class overridden");
	    break;
	default:
	    LOG ("Error setting device class: %s", StrError (err));
	    break;
    }
}



static Err
LinkConnect (Globals *g, BtLibDeviceAddressType *addr)
{
    Err err;
    ASSERT (Progressing());
    err = BtLibLinkConnect (g_btLibRefNum, addr);
    if (err == btLibErrAlreadyConnected || err == 0) {
	return 0;
    }
    else if (err == btLibErrPending)
        return WaitForResult (g, AsyncACLConnect, true, 0);
    else
	return err;
}


static void
LinkDisconnect (Globals *g, Device *d)
{
    Err err;
    ASSERT (Progressing());

    if (!d)
	return;
restart:
    INFO ("Disconnecting link");
    err = BtLibLinkDisconnect (g_btLibRefNum, &d->address);
    switch (err) {
	case 0:
	case btLibErrNoConnection:
	case btLibErrNoAclLink:
	    break;
	    
	case btLibErrPending:
	    WaitForResult (g, AsyncACLDisconnect, false, 0);
	    break;

	case btLibErrInProgress:
	    /* A previous connection is still in progress.   */
	    /* TODO.  Now what?  */
	    if (WaitForResult (g, AsyncACLConnect, false, 1000) == 0)
		goto restart;
	    break;

	default:
	    DEBUG ("Can't disconnect link: %s", StrError (err));
	    break;
    }
}



static Err
Authenticate (Globals *g, BtLibDeviceAddressType *addr)
{
    Err err;
    Boolean value = false;
    ASSERT (Progressing());

    err = BtLibLinkGetState (g_btLibRefNum, addr,
			     btLibLinkPref_Authenticated,
			     &value, sizeof (value));
    if (err) {
	DEBUG ("Error getting authentication state: %s", StrError (err));
	return err;
    }
    if (value)
	/* Already authenticated.  */
	return 0;

    value = true;
    err = BtLibLinkSetState (g_btLibRefNum, addr,
			     btLibLinkPref_Authenticated,
			     &value, sizeof (value));
    if (err == btLibErrPending)
        return WaitForResult (g, AsyncAuthenticate, true, 0);
    else if (err == 0)
	return 0;
    else {
	DEBUG ("Failed to request authentication: %s", StrError (err));
	return err;
    }
}



static Err
Encrypt (Globals *g, BtLibDeviceAddressType *addr)
{
    Err err;
    Boolean value = false;
    ASSERT (Progressing());

    err = BtLibLinkGetState (g_btLibRefNum, addr,
			     btLibLinkPref_Encrypted,
			     &value, sizeof (value));
    if (err) {
	DEBUG ("Error getting encryption state: %s", StrError (err));
	return err;
    }
    if (value)
	/* Already encrypted.  */
	return 0;

    value = true;
    err = BtLibLinkSetState (g_btLibRefNum, addr,
			     btLibLinkPref_Encrypted,
			     &value, sizeof (value));
    if (err == 0)
	return 0;
    else if (err == btLibErrPending)
        return WaitForResult (g, AsyncEncrypt, true, 0);
    else if (err == btLibErrFailed) {
	DEBUG ("Failed to request encryption: need to authenticate first");
	return err;
    }
    else {
	DEBUG ("Failed to request encryption: %s", StrError (err));
	return err;
    }
}



static Err
Connect (Globals *g, BTChannel *c, Device *d, UInt16 psm)
{
    Err err;
    BtLibSocketConnectInfoType connectInfo;
    ASSERT (Progressing());

    TRACE ("Connecting %s (%d) channel", c->name, psm);

    if (c->connected) {
	DEBUG ("Already connected");
	return 0;
    }
    
    if (c->dataSocket != -1) {
	LOG ("Channel not connected, but dataSocket is alive");
	BtLibSocketClose (g_btLibRefNum, c->dataSocket);
	c->dataSocket = -1;
    }
    c->connected = false;
    g_connected = false;
    err = BtLibSocketCreate (g_btLibRefNum,
                             &c->dataSocket,
                             (BtLibSocketProcPtr) BTSocketCallback,
                             (UInt32) c,
                             btLibL2CapProtocol);
    if (err) {
        LOG ("Couldn't create socket: %s", StrError (err));
        return err;
    }

    MemSet (&connectInfo, sizeof (connectInfo), 0);
    connectInfo.remoteDeviceP = &d->address;
    connectInfo.data.L2Cap.remotePsm = psm;
    connectInfo.data.L2Cap.minRemoteMtu = HIDP_MINIMUM_MTU;
    connectInfo.data.L2Cap.localMtu = HIDP_DEFAULT_MTU;

    err = BtLibSocketConnect (g_btLibRefNum, c->dataSocket, &connectInfo);
    if (err == 0) {
	c->connected = true;
        return 0;
    }
    else if (err == btLibErrPending)
	return WaitForResult (g, AsyncConnect, true, 0);
    else {
        LOG ("Couldn't connect socket: %s", StrError (err));
        BtLibSocketClose (g_btLibRefNum, c->dataSocket);
        c->dataSocket = -1;
        return err;
    }
}



static void
Disconnect (Globals *g, BTChannel *c, Boolean quick)
{
    ASSERT (Progressing());
    TRACE ("Disconnect %s", c->name);
    if (!c->connected)
	goto exit;

    // TODO: Wait for input queues?

    if (!quick && MsgAvailable (c->outqueue)) {
	INFO ("Sending remaining data");
	SendNext (g, c);
	WaitForQueue (g, c, MsgLast (c->outqueue), true, 500);
    }
    if (c->data) {
	DEBUG ("A message was stuck in the %s queue", c->name);
	MsgProcessed (c->outqueue, MemPtrRecoverHandle (c->data));
	c->data = 0;
    }
exit:
    c->connected = false;
    if (c->dataSocket != -1)
	BtLibSocketClose (g_btLibRefNum, c->dataSocket);
    c->dataSocket = -1;
    MsgFlush (c->outqueue);
    MsgFlush (c->inqueue);
}



static MemHandle
SendNext (Globals *g, BTChannel *c)
{
    Err err;
    MemHandle handle = 0;

    if (!c->connected) {
        MsgFlush (c->outqueue);
        TRACE ("Can't send: not connected");
        return 0;
    }

    if (c->data != 0) {
        TRACE ("Can't send: channel busy");
        return 0;
    }

    /* This ugly special case is here to get fast reaction times for
     * mouse movement events.  */
    if (!g_disconnecting
	&& c == &g_interrupt
	&& !MsgAvailable (c->outqueue)) {
        /* The message queue callback will call us recursively here.  */
        return HIDPSendReport (false);
    }

    if (!MsgAvailable (c->outqueue)) {
        TRACE ("Can't send: no data");
        return 0;
    }

    handle = MsgGet (c->outqueue);
    if (handle == 0)
        return 0;                   /* All data sent. */

    MTRACE (handle, "Sending next packet on %s channel", c->name);
    c->data = MemHandleLock (handle);
    c->status = 0;
    MDUMP (LogDebug, handle, c->name, c->data, MemHandleSize (handle));

    err = BtLibSocketSend (g_btLibRefNum, c->dataSocket,
                           c->data, MemHandleSize (handle));
    switch (err) {
	case btLibErrPending:       /* Send accepted. */
	    break;

	case btLibErrBusy:          /* A send is already in progress. */
	default:
	    MDEBUG (handle, "Error sending on %s channel: %s",
		   c->name, StrError (err));
	    ENQUEUE (GetAsyncResult (AsyncSend, err));
	    break;
    }
    TimerReschedule (c->sendTimer, TimGetTicks() + SEND_TIMEOUT);
    return handle;
}



static Err
AddSDPAttribute (Globals *g, UInt16 attr, UInt8 *data, UInt16 length)
{
    Err err;

    TRACE ("Setting SDP attribute %x", attr);

    DUMP (LogTrace, "SDP Attribute", data, length);

    if (!SDPVerifyRecord (data, data + length)) {
        DUMP (LogError, "SDP format error", data, length);
        return 1;
    }

    err = BtLibSdpServiceRecordSetRawAttribute
        (g_btLibRefNum, g_SDPRecord, attr, data, length);
    if (err) {
        ERROR ("Error setting SDP attribute %x: %s",
               attr, StrError (err));
        return err;
    }

    return 0;
}



static Err
PublishSDPServiceRecord (Globals *g)
{
    Err err;
    UInt8 *b;
    UInt8 *start[5];
    int depth = 0;
    BtLibSdpUuidType uuid;
    UInt8 *buf = CheckedMemPtrNew (256, false);
    b = buf;

    TRACE ("PublishSDPServiceRecord");

    if (g_SDPRecord) {
        BtLibSdpServiceRecordStopAdvertising (g_btLibRefNum, g_SDPRecord);
        BtLibSdpServiceRecordDestroy (g_btLibRefNum, g_SDPRecord);
        MemPtrFree (buf);
        g_SDPRecord = 0;
    }

#define SEQSTART start[depth++] = b; b = SDPSequenceStart (b);
#define SEQEND b = SDPSequenceEnd (start[--depth], b)
#define STRINGSTART start[depth++] = b; b = SDPStringStart (b);
#define STRINGEND b = SDPStringEnd (start[--depth], b)

#define ADDATTRIBUTE(a)					\
    do {						\
        ASSERT (depth == 0);				\
        err = AddSDPAttribute (g, a, buf, b - buf);	\
        if (err) goto error;				\
        b = buf;					\
    } while (0)

#if 0
    /* Publish the Device Identification record.  */
    err = BtLibSdpServiceRecordCreate (g_btLibRefNum, &g_SDPRecord);
    if (err) {
        LOG ("Couldn't create SDP record: %s", StrError (err));
        return err;
    }

    SEQSTART {
	/* Device Identification Service Class UUID */
        BtLibSdpUuidInitialize (uuid, "\x12\x00", 2); /* PnPInformation */
        b = SDPUUID (b, &uuid);
    }
    SEQEND;
    ADDATTRIBUTE (btLibServiceClassIdList);
    
    b = SDPUInt16 (b, 0x0102);
    ADDATTRIBUTE (0x0200);	/* SpecificationID */

    b = SDPUInt16 (b, 0x045e);	/* Microsoft */
    ADDATTRIBUTE (0x0201);	/* VendorID */

    b = SDPUInt16 (b, 0x007b);	/* Microsoft Wireless Optical Desktop */
    ADDATTRIBUTE (0x0202);	/* ProductID */

    b = SDPUInt16 (b, 0x0033);
    ADDATTRIBUTE (0x0203);	/* Version */

    b = SDPBoolean (b, true);
    ADDATTRIBUTE (0x0204);	/* PrimaryRecord */

    b = SDPUInt16 (b, 0x0001);	/* Bluetooth SIG */
    ADDATTRIBUTE (0x0205);	/* VendorIDSource */

    {
	BtLibStringType str;
	BtLibLanguageBaseTripletType lang = {
	    btLibLangEnglish,
	    btLibCharSet_UTF_8,
	    0x0100
	};

        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord,
             btLibLanguageBaseAttributeIdList,
             (BtLibSdpAttributeDataType *)&lang, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 btLibLanguageBaseAttributeIdList,
		 StrError (err));
            goto error;
        }

        str.str = SDP_SERVICE_NAME " " SDP_SERVICE_DESC;
        str.strLen = StrLen (str.str);
        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord, 0x0101,
             (BtLibSdpAttributeDataType *)&str, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 0x0100 + btLibServiceNameOffset,
		 StrError (err));
            goto error;
        }
    }
    
    err = BtLibSdpServiceRecordStartAdvertising (g_btLibRefNum, g_SDPRecord);
    if (err) {
        LOG ("Can't advertise SDP record: %s", StrError (err));
        goto error;
    }
#endif
    
    /* Publish the Human Interface Device record.  */

    err = BtLibSdpServiceRecordCreate (g_btLibRefNum, &g_SDPRecord);
    if (err) {
        LOG ("Couldn't create SDP record: %s", StrError (err));
        return err;
    }

    /* Service Class ID List.  */
    SEQSTART {
        /* HID Service Class UUID */
        BtLibSdpUuidInitialize (uuid, "\x11\x24", 2);
        b = SDPUUID (b, &uuid);
    }
    SEQEND;
    ADDATTRIBUTE (btLibServiceClassIdList);

    /* Browse group list.  */
    SEQSTART {
        /* PublicBrowseRoot */
        BtLibSdpUuidInitialize (uuid, "\x10\x02", 2);
        b = SDPUUID (b, &uuid);
    }
    SEQEND;
    ADDATTRIBUTE (btLibBrowseGroupList);  /* BrowseGroupList */

    /* Protocol Descriptor List.  */
    SEQSTART {
        SEQSTART {
            /* L2CAP UUID */
            BtLibSdpUuidInitialize (uuid, "\x01\x00", 2);
            b = SDPUUID (b, &uuid);
            b = SDPUInt16 (b, L2CAP_PSM_HIDP_CTRL);
        }
        SEQEND;

        SEQSTART {
            /* HIDProtocolUUID */
            BtLibSdpUuidInitialize (uuid, "\x00\x11", 2);
            b = SDPUUID (b, &uuid);
        }
        SEQEND;
    }
    SEQEND;
    ADDATTRIBUTE (btLibProtocolDescriptorList);

    /* Additional Protocol Descriptor Lists.  */
    SEQSTART {
        /* First (and only) Protocol Descriptor List. */
        SEQSTART {
            /* L2CAP descriptor. */
            SEQSTART {
                /* L2CAP UUID */
                BtLibSdpUuidInitialize (uuid, "\x01\x00", 2);
                b = SDPUUID (b, &uuid);
                b = SDPUInt16 (b, L2CAP_PSM_HIDP_INTR);
            }
            SEQEND;

            /* HID descriptor. */
            SEQSTART {
                /* HIDProtocolUUID */
                BtLibSdpUuidInitialize (uuid, "\x00\x11", 2);
                b = SDPUUID (b, &uuid);
            }
            SEQEND;
        }
        SEQEND;
    }
    SEQEND;
    ADDATTRIBUTE (0x000d);  /* Alternate Protocol Descriptor Lists */

    /* Bluetooth Profile Descriptor List.  */
    SEQSTART {
        SEQSTART {
            /* HID Service Class UUID */
	    /* I think this is wrong, but Windows SP2 seems to only
	     * understand this one. */
	    BtLibSdpUuidInitialize (uuid, "\x11\x24", 2);
            b = SDPUUID (b, &uuid);
            b = SDPUInt16 (b, 0x0100); /* HID version number (1.00). */
        }
        SEQEND;
        SEQSTART {
            /* HID Profile UUID */
	    /* I think this is the correct value.  */
            BtLibSdpUuidInitialize (uuid, "\x00\x11", 2);
            b = SDPUUID (b, &uuid);
            b = SDPUInt (b, 0x0100); /* HID version number (1.00). */
        }
        SEQEND;
    }
    SEQEND;
    ADDATTRIBUTE (btLibProfileDescriptorList);

    /* We must use the PalmOS API to set string attributes.  */
    {
        BtLibStringType str;

        BtLibLanguageBaseTripletType lang = {
            btLibLangEnglish,
            btLibCharSet_UTF_8,
            0x0100
        };

        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord,
             btLibLanguageBaseAttributeIdList,
             (BtLibSdpAttributeDataType *)&lang, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 btLibLanguageBaseAttributeIdList,
		 StrError (err));
            goto error;
        }

        str.str = SDP_SERVICE_NAME;
        str.strLen = StrLen (str.str);
        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord,
             0x0100 + btLibServiceNameOffset,
             (BtLibSdpAttributeDataType *)&str, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 0x0100 + btLibServiceNameOffset,
		 StrError (err));
            goto error;
        }

        str.str = SDP_SERVICE_DESC;
        str.strLen = StrLen (str.str);
        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord,
             0x0100 + btLibServiceDescriptionOffset,
             (BtLibSdpAttributeDataType *)&str, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 0x0100 + btLibServiceDescriptionOffset,
		 StrError (err));
            goto error;
        }

        str.str = SDP_PROVIDER_NAME;
        str.strLen = StrLen (str.str);
        err = BtLibSdpServiceRecordSetAttribute
            (g_btLibRefNum, g_SDPRecord,
             0x0100 + btLibProviderNameOffset,
             (BtLibSdpAttributeDataType *)&str, 0, 0);
        if (err) {
            LOG ("Error setting attribute %x: %s",
		 0x0100 + btLibProviderNameOffset,
		 StrError (err));
            goto error;
        }
    }

    /* HID Device Release Number.  */
    b = SDPUInt (b, 0x0100);
    ADDATTRIBUTE (0x0200);  /* HID Device Release Number */

    /* HID Parser Version.  (USB version) */
    b = SDPUInt (b, 0x0111);
    ADDATTRIBUTE (0x0201);  /* HIDParserVersion */

    /* HID Device Subclass.  */
    switch (SPREF (deviceSubclass)) {
	case classKeyboard:
	    b = SDPUInt (b, HID_DEVICE_SUBCLASS_KEYBOARD);
	    break;
	case classMouse:
	    b = SDPUInt (b, HID_DEVICE_SUBCLASS_MOUSE);
	    break;
	case classMouseKeyboard:
	    b = SDPUInt (b, HID_DEVICE_SUBCLASS_MOUSEKEYBOARD);
	    break;
	default:
	case classCombo:
	    b = SDPUInt (b, HID_DEVICE_SUBCLASS_COMBO);
	    break;
    }
    ADDATTRIBUTE (0x0202);  /* HIDDeviceSubclass */

    /* HID Country Code.  */
    b = SDPUInt (b, 33);    /* USA */
    ADDATTRIBUTE (0x0203);  /* HIDCountryCode */

    /* HID Virtual Cable.  */
    b = SDPBoolean (b, true);
    ADDATTRIBUTE (0x0204);  /* HIDVirtualCable */

    /* HID Reconnect Initiate.  */
    b = SDPBoolean (b, true);
    ADDATTRIBUTE (0x0205);  /* HIDReconnectInitiate */

    /* HID Descriptor List.  */
    SEQSTART {
	/* HID Class Descriptor.  */
	SEQSTART {
	    b = SDPUInt (b, 0x22); /* Report Descriptor. */
	    STRINGSTART {
		/* HID-format Report Descriptor.  */
		b = HIDDescriptor (b);
	    }
	    STRINGEND;
	}
	SEQEND;
    }
    SEQEND;
    ADDATTRIBUTE (0x0206);  /* HIDDescriptorList */

    /* HID LANGID Base List.  */
    SEQSTART {
        /* HID LANGID -> Bluetooth language mapping.  */
        SEQSTART {
            b = SDPUInt (b, 0x0409); /* English (United States) */
            b = SDPUInt (b, 0x0100); /* Bluetooth String Offset */
        }
        SEQEND;
    }
    SEQEND;
    ADDATTRIBUTE (0x0207);  /* HIDLANGIDBaseList */

    /* HID SDP Disable.  */
    b = SDPBoolean (b, false); /* Device does support SDP while connected. */
    ADDATTRIBUTE (0x0208);  /* HIDSDPDisable */

    /* HID Battery Power.  */
    b = SDPBoolean (b, true); /* Device is battery powered. */
    ADDATTRIBUTE (0x0209);  /* HIDBatteryPower */

    /* HID Remote Wake.  */
    b = SDPBoolean (b, true); /* Device wants to wake up host. */
    ADDATTRIBUTE (0x020A);

    /* HID Profile Version.  */
    b = SDPUInt (b, 0x0100); /* Version 1.00 */
    ADDATTRIBUTE (0x020B);

    /* HID Supervision Timeout.  */
    b = SDPUInt (b, 0x1F40);
    ADDATTRIBUTE (0x020C);
    
    /* HID Normally Connectable.  */
    b = SDPBoolean (b, false);
    ADDATTRIBUTE (0x020D);      /* HIDNormallyConnectable */

    /* HID Boot Device.  */
    b = SDPBoolean (b, true); /* Device supports the boot protocol. */
    ADDATTRIBUTE (0x020E);

    err = BtLibSdpServiceRecordStartAdvertising (g_btLibRefNum, g_SDPRecord);
    if (err) {
        LOG ("Can't advertise SDP record: %s", StrError (err));
        goto error;
    }

    return 0;

error:
    BtLibSdpServiceRecordDestroy (g_btLibRefNum, g_SDPRecord);
    MemPtrFree (buf);
    g_SDPRecord = 0;
    return err;
}



static Err
WaitForResult (Globals *g, AsyncKind kind,
	       Boolean cancelable, UInt16 timeout)
{
    EventType event;
    UInt32 stop = TimGetTicks() + timeout;
    Err result = 0;
    Boolean exitapp = false;

    if (Progressing() && cancelable)
	ProgressEnableCancel (true);
    
    while (1) {
	TimerTick();
	
	if (!CmdGet (&event)) {
	    Command cmd = EVENT_COMMAND (&event);
	    if (CmdIsAsyncResult (cmd)
		&& (CmdAsyncKind (cmd) == kind
		    || CmdAsyncKind (cmd) == AsyncRadio)) {
		result = CmdAsyncErr (cmd);
		break;
	    }
	    if (cmd == CmdExitApp) {
		exitapp = true;
		if (cancelable) {
		    result = appErrProgressCanceled;
		    break;
		}
	    }
	    
	}
	else EvtGetEvent (&event, TimerTimeout());

	if (SysHandleEvent (&event))
	    continue;
	
        if (FrmDispatchEvent (&event))
            continue;

	if (event.eType == appStopEvent) {
	    exitapp = true;
	    if (cancelable) {
		result = appErrProgressCanceled;
		break;
	    }
	}
	
	if (cancelable && Progressing() && ProgressCanceled()) {
	    result = appErrProgressCanceled;
	    break;
	}

        if (timeout && TimGetTicks() >= stop) {
	    result = appErrTimeout;
	    break;
	}
    };
    if (exitapp)
	ENQUEUE (CmdExitApp);
    return result;
}



static Err
WaitForCommand (Globals *g, Command cmd, Boolean cancelable, UInt16 timeout)
{
    EventType event;
    UInt32 stop = TimGetTicks() + timeout;
    Err result = 0;
    Boolean exitapp = false;

    if (Progressing() && cancelable)
	ProgressEnableCancel (true);
    
    while (1) {
	TimerTick();
	
	if (!CmdGet (&event)) {
	    Command c = EVENT_COMMAND (&event);
	    if (CmdIsAsyncResult (c)
		&& CmdAsyncKind (c) == AsyncRadio) {
		result = CmdAsyncErr (c);
		break;
	    }
	    if (c == cmd) {
		result = 0;
		break;
	    }
	    if (c == CmdExitApp) {
		exitapp = true;
		if (cancelable) {
		    result = appErrProgressCanceled;
		    break;
		}
	    }
	}
	else EvtGetEvent (&event, TimerTimeout());

	if (SysHandleEvent (&event))
	    continue;
	
        if (FrmDispatchEvent (&event))
            continue;

	if (event.eType == appStopEvent) {
	    exitapp = true;
	    if (cancelable) {
		result = appErrProgressCanceled;
		break;
	    }
	}
	
	if (cancelable && Progressing() && ProgressCanceled()) {
	    result = appErrProgressCanceled;
	    break;
	}

        if (timeout && TimGetTicks() >= stop) {
	    result = appErrTimeout;
	    break;
	}
    };
    if (exitapp)
	ENQUEUE (CmdExitApp);
    return result;
}



static Err
WaitForQueue (Globals *g, BTChannel *c, MemHandle handle,
	      Boolean cancelable, UInt16 timeout)
{
    EventType event;
    UInt32 stop = TimGetTicks() + timeout;
    Err result = 0;
    Boolean exitapp = false;

    DEBUG ("WaitForQueue %s %lx %d",
           c->name, (UInt32)handle, timeout);

    /* Try the quick path first.  */
    if (!c->connected)
	return appErrNotConnected;

    if (MsgIsProcessed (c->outqueue, handle))
        return 0;

    ASSERT (Progressing());

    if (cancelable)
	ProgressEnableCancel (true);
    
    while (1) {
	ASSERT (Progressing());
	
        TimerTick();

        if (CmdGet (&event) == 0) {
	    Command cmd = EVENT_COMMAND (&event);
	    if (CmdIsAsyncResult (cmd)
		&& CmdAsyncKind (cmd) == AsyncSend
		&& CmdAsyncErr (cmd)) {
		result = CmdAsyncErr (cmd);
		break;
	    }
	    if (cmd == CmdExitApp) {
		exitapp = true;
		if (cancelable) {
		    result = appErrProgressCanceled;
		    break;
		}
	    }
	}
	else {
            EvtGetEvent (&event,
			 MIN (TimerTimeout(), stop - TimGetTicks()));
	}
	
	LogFlush (false);

        if (BTHandleEvent (&event))
            continue;

	if (!c->connected) {
	    result = appErrNotConnected;
	    break;
	}

	if (MsgIsProcessed (c->outqueue, handle)) {
	    result = 0;
	    break;
	}
	
	if (SysHandleEvent (&event))
	    continue;
	
        if (FrmDispatchEvent (&event))
            continue;

	if (event.eType == appStopEvent) {
	    exitapp = true;
	    if (cancelable) {
		result = appErrProgressCanceled;
		break;
	    }
	}
	
        if (cancelable && ProgressCanceled()) {
	    result = appErrProgressCanceled;
	    break;
	}

        if (TimGetTicks() >= stop) {
            result = appErrTimeout;
	    break;
        }
    };
    if (exitapp)
	ENQUEUE (CmdExitApp);
    return result;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

Err
f_BTRadioInit (Globals *g) 
{
    Err err = errNone;
    BtLibAccessibleModeEnum newAccessible = HID_ACCESSIBLE;
    Boolean cancel;

    ASSERT (g && g->bt);
    ASSERT (Progressing());

    g_libLoaded = false;
    g_libOpened = false;

    if (ProgressCanceled())
	return appErrProgressCanceled;
    cancel = ProgressEnableCancel (false);
    
    /* Get a handle to the Bluetooth library.  */
    INFO ("Opening Bluetooth library");
    if (SysLibFind (btLibName, &g_btLibRefNum) != errNone) {
        err = SysLibLoad (sysFileTLibrary, sysFileCBtLib, &g_btLibRefNum);
        if (err) {
            ERROR ("Could not load Bluetooth library: %s", StrError (err));
            return err;
        }
        g_libLoaded = true;
    }

    /* Open the Bluetooth library.  */
    err = BtLibOpen (g_btLibRefNum, false);
    if (err && err != btLibErrAlreadyOpen) {
        if (g_libLoaded)
            SysLibRemove (g_btLibRefNum);
	g_libLoaded = false;
	if (err == btLibErrInUseByService || err == btLibErrBusy)
	    ERROR ("The Bluetooth radio is in use by another service.  "
		   "Please disable that service to start BlueRemote.");
	else
	    ERROR ("Could not open Bluetooth library: %s", StrError (err));
        return err;
    }

    /* Register our management callback function.  */
    err = BtLibRegisterManagementNotification
        (g_btLibRefNum,
         (BtLibManagementProcPtr) BTManagementCallback,
         (UInt32) g);
    if (err) {
	BtLibClose (g_btLibRefNum);
        if (g_libLoaded)
            SysLibRemove (g_btLibRefNum);
	g_libLoaded = false;
        ERROR ("Could not register management callback: %s",
               StrError (err));
        return err;
    }
    g_libOpened = true;

    /* BtLibOpen will report the radio state once it has initialized it.  */
    INFO ("Initializing radio");
    err = WaitForResult (g, AsyncRadio, true, 0);
    if (err == appErrProgressCanceled) {
	return err;
    }
    else if (err != btLibErrRadioInitialized) {
	ERROR ("Could not initialize Bluetooth radio: %s",
	       StrError (err));
	return err;
    }
    
    err = WaitForResult (g, AsyncAccessibleMode, true, 0);
    if (err == appErrProgressCanceled) {
	return err;
    }
    if (err) {
	ERROR ("Could not initialize Bluetooth radio: %s",
	       StrError (err));
	return err;
    }
    
    if (g_origClass == 0) {
	/* Get original local class.  */
	INFO ("Configuring Bluetooth settings");
	err = BtLibGetGeneralPreference (g_btLibRefNum,
					 btLibPref_LocalClassOfDevice,
					 &g_origClass,
					 sizeof (BtLibClassOfDeviceType));
	if (err) {
	    LOG ("Error getting Bluetooth class: %s", StrError (err));
	    /* Fallback to some reasonable guesses.  */
	    if (ToolsGetDeviceFlags() & DeviceFlagSmartPhone)
		/* 0x50020C (Treo 650) */
		g_origClass = btLibCOD_ObjectTransfer
		    | btLibCOD_Telephony
		    | btLibCOD_Major_Phone
		    | btLibCOD_Minor_Phone_Smart;
	    else
		/* Guess at a normal Palm's default class.  */
		// 0x100114 (Tungsten T2)
		g_origClass = btLibCOD_ObjectTransfer
		    | btLibCOD_Major_Computer
		    | btLibCOD_Minor_Comp_Palm;
	}
    
	/* Get original accessibility mode.  */
	err = BtLibGetGeneralPreference (g_btLibRefNum,
					 btLibPref_UnconnectedAccessible,
					 &g_origAccessible,
					 sizeof (BtLibAccessibleModeEnum));
	if (err) {
	    LOG ("Error getting accessibility: %s", StrError (err));
	    g_origAccessible = HID_ACCESSIBLE;
	}
    }

    ProgressEnableCancel (cancel);

    /* Set open accessibility mode.  */
    err = BtLibSetGeneralPreference (g_btLibRefNum,
				     btLibPref_UnconnectedAccessible,
				     &newAccessible,
				     sizeof (BtLibAccessibleModeEnum));
    switch (err) {
	case btLibErrPending:
	    DEBUG ("Accessible mode pending");
	    err = WaitForResult (g, AsyncAccessibleMode, true, 1000);
	    if (err == appErrProgressCanceled)
		return err;
	    if (err)
		LOG ("Could not set accessible mode: %s", StrError (err));
	    else
		DEBUG ("Accessible mode overridden");
	    break;
	case btLibErrNoError:
	    DEBUG ("Accessible mode overridden");
	    break;
	default:
	    LOG ("Could not set accessible mode: %s", StrError (err));
	    break;
    }

    /* Start masquarading as a peripheral device.  */
    Masquarade (g);

    return 0;
}

void
f_BTRadioClose (Globals *g)
{
    ASSERT (g && g->bt);
    ASSERT (Progressing());

    ProgressEnableCancel (false);
    g_connecting = false;
    g_connected = false;
    g_disconnecting = false;
    Disconnect (g, &g_interrupt, true);
    Disconnect (g, &g_control, true);
    f_BTListenClose (g);

    /* Unregister our callback.  */
    if (g_libOpened) {
	BtLibUnregisterManagementNotification
	    (g_btLibRefNum,
	     (BtLibManagementProcPtr) BTManagementCallback);
	BtLibClose (g_btLibRefNum);
	g_libOpened = false;
    }

    /* Remove Bluetooth library if we loaded it.  */
    if (g_libLoaded) {
        SysLibRemove (g_btLibRefNum);
	g_libLoaded = false;
    }

    g_btLibRefNum = 0;
}


Err
f_BTInit (Globals *g)
{
    Err err;
    TRACE ("BTInit");

    if (g->bt != 0)
        f_BTClose (g);

    g->bt = CheckedMemPtrNew (sizeof (BTGlobals), true);

    err = f_BTRadioInit (g);
    if (err) {
        MemPtrFree (g->bt);
        g->bt = 0;
	return err;
    }

    g_controlInQueue = MsgNewQueue ("CtrlInQueue", 10, true, true, NULL);
    g_controlOutQueue = MsgNewQueue ("CtrlOutQueue", 10, true, true,
                                     BTMsgQueueCallback);
    g_interruptInQueue = MsgNewQueue ("IntrInQueue", 10, true, true, NULL);
    g_interruptOutQueue = MsgNewQueue ("IntrOutQueue", 10, true, true,
                                       BTMsgQueueCallback);

    g_control.listenerSocket = -1;
    g_control.dataSocket = -1;
    g_control.inqueue = g_controlInQueue;
    g_control.outqueue = g_controlOutQueue;
    g_control.g = g;
    g_control.psm = L2CAP_PSM_HIDP_CTRL;
    g_control.name = "Control";
    g_control.sendTimer = TimerNew ("Control channel", 0, BTSendTimer,
				    (UInt32) &g_control);
    

    g_interrupt.listenerSocket = -1;
    g_interrupt.dataSocket = -1;
    g_interrupt.inqueue = g_interruptInQueue;
    g_interrupt.outqueue = g_interruptOutQueue;
    g_interrupt.g = g;
    g_interrupt.psm = L2CAP_PSM_HIDP_INTR;
    g_interrupt.name = "Interrupt";
    g_interrupt.sendTimer = TimerNew ("Interrupt channel", 0, BTSendTimer,
				    (UInt32) &g_interrupt);

    g_connected = false;
    return 0;
}



void
f_BTClose (Globals *g)
{
    Err err = 0;
    if (g->bt == NULL)
        return;

    TRACE ("BTClose");

    ProgressStart ("Cleaning up");

    MsgFreeQueue (g_controlInQueue);
    g_controlInQueue = 0;
    g_control.inqueue = 0;
    MsgFreeQueue (g_controlOutQueue);
    g_controlOutQueue = 0;
    g_control.outqueue = 0;
    MsgFreeQueue (g_interruptInQueue);
    g_interruptInQueue = 0;
    g_interrupt.inqueue = 0;
    MsgFreeQueue (g_interruptOutQueue);
    g_interruptOutQueue = 0;
    g_interrupt.outqueue = 0;

    TimerFree (g_control.sendTimer);
    TimerFree (g_interrupt.sendTimer);
    
    /* Restore original device class.  */
    INFO ("Restoring Bluetooth settings");
    if (g_origClass != 0) {
        err = BtLibSetGeneralPreference (g_btLibRefNum,
                                         btLibPref_LocalClassOfDevice,
                                         &g_origClass,
                                         sizeof (BtLibClassOfDeviceType));
        switch (err) {
	    case btLibErrNoError:
		DEBUG ("Device class restored");
		break;
	    default:
		DEBUG ("Error restoring device class: %s", StrError (err));
		break;
        }
    }

    /* Set original accessibility mode.  */
    err = BtLibSetGeneralPreference (g_btLibRefNum,
                                     btLibPref_UnconnectedAccessible,
                                     &g_origAccessible,
                                     sizeof (BtLibAccessibleModeEnum));
    switch (err) {
	case btLibErrPending:
	    TRACE ("Accessible mode pending");
	    err = WaitForResult (g, AsyncAccessibleMode, true, 0);
	    if (err)
		DEBUG ("Error restoring accessible mode: %s",
		       StrError (err));
	    break;
	case btLibErrNoError:
	    DEBUG ("Accessible mode restored");
	    break;
	default:
	    DEBUG ("Error restoring accessible mode: %s", StrError (err));
	    break;
    }

    f_BTRadioClose (g);

    ProgressStop (false);

    MemPtrFree (g->bt);
    g->bt = NULL;
}



Err
f_BTListenInit (Globals *g)
{
    Err err;

    TRACE ("BTListenInit");
    ASSERT (g && g->bt);

    /* Create control socket.  */
    err = Listen (g, &g_control, L2CAP_PSM_HIDP_CTRL);
    if (err) {
        f_BTListenClose (g);
        return err;
    }

    /* Create interrupt socket.  */
    err = Listen (g, &g_interrupt, L2CAP_PSM_HIDP_INTR);
    if (err) {
        f_BTListenClose (g);
        return err;
    }

    /* Create and advertise SDP record.  */
    err = PublishSDPServiceRecord (g);
    if (err) {
        f_BTListenClose (g);
        return err;
    }

    return errNone;
}

Err
f_BTWait (Globals *g)
{
    Err err;
    MemHandle report;
 
    ASSERT (g && g->bt);
    ASSERT (Progressing());

    g_connecting = false;

    while (!g_control.connected || !g_interrupt.connected) {
	err = WaitForResult (g, AsyncConnect, true, 0);
	if (err)
	    return err;
    }

    /* Delay for a few seconds to give time for Windows to start up
     * the pairing process.  It *should* start it earlier, but
     * sometimes it doesn't, and we don't want to switch screens
     * before it has finished.  */
    WaitForResult (g, AsyncNone, false, 200);

    INFO ("Sending first report");
    report = HIDPSendReport (true);
    err = WaitForQueue (g, &g_interrupt, report, true, 500);
    if (err) {
	if (err == appErrNotConnected)
	    err = appErrConnTerminated;
	return err;
    }
    
    INFO ("Connected!");
    
    g_connected = true;
    return 0;
}



void
f_BTListenClose (Globals *g)
{
    ASSERT (g && g->bt);

    /* Delete SDP record.  */
    if (g_SDPRecord) {
        BtLibSdpServiceRecordStopAdvertising (g_btLibRefNum, g_SDPRecord);
        BtLibSdpServiceRecordDestroy (g_btLibRefNum, g_SDPRecord);
        g_SDPRecord = 0;
    }

    /* Close control socket.  */
    if (g_control.listenerSocket != -1) {
        BtLibSocketClose (g_btLibRefNum, g_control.listenerSocket);
        g_control.listenerSocket = -1;
    }

    /* Close interrupt socket.  */
    if (g_interrupt.listenerSocket) {
        BtLibSocketClose (g_btLibRefNum, g_interrupt.listenerSocket);
        g_interrupt.listenerSocket = -1;
    }
}



Boolean
f_BTHandleEvent (Globals *g, EventType *e)
{
    Command cmd;
    Err status;
    Err err;

    if (!g || !g->bt) return false;

    if (e->eType != commandEvent)
        return false;

    cmd = EVENT_COMMAND (e);

    if (!CmdIsAsyncResult (cmd))
        return false;

    status = CmdAsyncErr (cmd);

    TRACE ("BTHandleEvent %ld 0x%lx %d %x", cmd, cmd,
	   (int)CmdAsyncKind (cmd), status);

    if (!status)
	return false;
    
    switch (CmdAsyncKind (cmd)) {
	case AsyncACLDisconnect:
	    LOG ("Link closed: %s", StrError (status));
	    
	    g_connected = false;
	    if (status == btLibMeStatusUserTerminated
		|| status == btLibMeStatusLocalTerminated)
		ENQUEUE (CmdInitiateDisconnect);
	    else {
		/* User moved out of range, disconnected or reset
		   Bluetooth radio on host, or similar.  */
		if (ToolsGetDeviceFlags()
		    & DeviceFlagBluetoothCrashOnLinkError)
		    ENQUEUE (CmdRadioError);
		else
		    ENQUEUE (CmdConnectionError);
	    }
	    return true;
	    
	case AsyncSend:
	    TRACE ("AsyncSend");
	    LOG ("Error sending data: %s", StrError (status));

	    if (g_connected) {
		g_connected = false;
		if (status == btLibErrNoAclLink
		    && (ToolsGetDeviceFlags()
			& DeviceFlagBluetoothCrashOnLinkError))
		    ENQUEUE (CmdRadioError);
		else
		    ENQUEUE (CmdConnectionError);
	    }
	    return true;
	    
	case AsyncRadio:
	    switch (status) {
		default:
		case btLibErrRadioInitialized:
		    ENQUEUE (CmdRadioError);
		    return true;
		    
		case btLibErrRadioSleepWake:
		case btLibErrRadioFatal:
		case btLibErrRadioInitFailed:
		    ProgressStart ("Reinitializing radio");
		    err = WaitForResult (g, AsyncRadio, false, 300);
		    if (err == btLibErrRadioInitialized)
			DEBUG ("Got RadioInitialized");
		    else {
			ERROR ("%s", StrError (err));
			ENQUEUE (CmdExitApp);
		    }
		    ENQUEUE (CmdRadioError);
		    return true;
	    }

	default:
	    return false;
    }    
}



Err
f_BTConnect (Globals *g)
{
    Err err;
    Device *d = DevListSelected();
    MemHandle report;

    ASSERT (g && g->bt);
    ASSERT (Progressing());

    if (g_connected) {
        ERROR ("Already connected");
        return 0;
    }

    if (!d) {
        err = f_BTSelectDevice (g);
	if (err)
            return err;
        d = DevListSelected();
        ASSERT (d);
    }

    g_connecting = true;

    /* Create an Asynchronous Connectionless (ACL) link.  */
    INFO ("Establishing link");
    err = LinkConnect (g, &d->address);
    if (err)
	goto cleanup;

    UpdateRemoteName (g, &d->address, false);

    ProgressSelectAnimation (AnimationChannelForward);

    if (d->encryptLink) {
	INFO ("Requesting authentication");
	err = Authenticate (g, &d->address);
	if (err == btLibMeStatusCommandDisallowed) {
	    /* Linux doesn't support authenticated HID.  Bad Linux!  */
	    INFO ("%s denied authentication", d->nameStr);
	}
	else if (err == appErrProgressCanceled) {
	    goto cleanup;
	}
	else if (err) {
	    INFO ("Authentication failed");
	    DEBUG ("Authentication failed: %s", StrError (err));
	    DevListDelete (DevListSelected());
	    err = appErrEncryptionFailed;
	    goto cleanup;
	}
	else {			/* Successful authentication. */
	    /* Encryption requires an authenticated link.  */
	    INFO ("Requesting encryption");
	    err = Encrypt (g, &d->address);
	    if (err)
		goto cleanup;
	}
    }
    
    INFO ("Connecting Control channel");
    if (!g_control.connected) {
	err = Connect (g, &g_control, d, L2CAP_PSM_HIDP_CTRL);
	if (err)
	    goto cleanup;
    }

    if (!g_interrupt.connected) {
	INFO ("Connecting Interrupt channel");
	err = Connect (g, &g_interrupt, d, L2CAP_PSM_HIDP_INTR);
	if (err)
	    goto cleanup;
    }

    INFO ("Preparing connection");
    if (WaitForCommand (g, CmdUnplugReceived, false, 30) == 0) {
	/* HACK HACK HACK.  Windows/WIDCOMM sends an UNPLUG
	 * immediately after initialization of the interrupt channel
	 * if the device tried to connect on an encrypted link.
	 * Interestingly, it still allows the connection to continue.
	 * Windows shouldn't send us UNPLUGs anymore, but handle them
	 * anyway. */
	INFO ("Ignoring unplug request");
	SndPlaySystemSound (sndWarning);
	WaitForResult (g, AsyncNone, false, 200);
    }

    INFO ("Sending first report");
    report = HIDPSendReport (true);
    err = WaitForQueue (g, &g_interrupt, report, true, 500);
    if (err) {
	if (err == appErrNotConnected)
	    err = appErrConnTerminated;
	goto cleanup;
    }

    INFO ("Connected!");
    d->lastConnected = TimGetSeconds();
    DevListUpdate (d);
    
    g_connected = true;
    g_connecting = false;
    return 0;
    
cleanup:
    INFO ("Cleaning up");
    Disconnect (g, &g_interrupt, true);
    Disconnect (g, &g_control, true);
    g_connected = false;
    g_connecting = false;
    return err;
}



void
f_BTDisconnect (Globals *g)
{
    Device *d = DevListSelected();

    DEBUG ("BTDisconnect called");
    ASSERT (g && g->bt);
    ASSERT (Progressing());

    if (g_connected) {
        HIDPReset();
        DEBUG ("BTDisconnect HID reset");
    }

    g_connecting = false;
    g_connected = false;
    g_disconnecting = true;

    /* Disconnect channels; the order is important.  */
    Disconnect (g, &g_interrupt, false);
    Disconnect (g, &g_control, false);

    LinkDisconnect (g, d);
    
    g_disconnecting = false;
}



Boolean
f_BTIsConnected (Globals *g)
{
    if (!g || !g->bt) return false;
    return g_connected;
}



Err
f_BTSelectDevice (Globals *g)
{
    Err err;
    BtLibDeviceAddressType addr;

    ASSERT (g && g->bt);

    /* Discover a single device.  */
    err = BtLibDiscoverSingleDevice
        (g_btLibRefNum, "Select a device:",
         NULL, 0,               /* Any device. */
         &addr,
         false,                 /* Display names, not addresses.  */
         false);                /* Don't display a previous list.  */
    if (err == btLibErrCanceled) {
        /* User canceled the discovery.  */
        return err;
    }
    else if (err != btLibErrNoError) {
        ERROR ("Error discovering devices: %s", StrError (err));
        return err;
    }

    SetupNewAddress (g, &addr);

    return 0;
}



MemHandle
f_BTDataReady (Globals *g)
{
    if (!g || !g->bt) return 0;

    if (!g_connected)
        return 0;

    if (g_interrupt.data != 0)
        return 0;

    return HIDPSendReport (false);
}

#endif DISABLE_BLUETOOTH
