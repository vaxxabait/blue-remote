/**************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: HIDProtocol.c
 *
 **************************************************************************/

#define LOGLEVEL LogDebug

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "HIDProtocol.h"
#undef PRIVATE_INCLUDE

#include "KeyboardForm.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define STICKY_TIMEOUT (6 * sysTicksPerSecond)

#define BUFSIZE 100

#define KEYBUFFER_SIZE  9

#define HIDPTypeHandshake   0x00
#define HIDPTypeHIDControl  0x10
#define HIDPTypeGetReport   0x40
#define HIDPTypeSetReport   0x50
#define HIDPTypeGetProtocol 0x60
#define HIDPTypeSetProtocol 0x70
#define HIDPTypeGetIdle     0x80
#define HIDPTypeSetIdle     0x90
#define HIDPTypeData        0xA0
#define HIDPTypeDatC        0xB0
#define HIDPTypeMask        0xF0

#define HIDPParameterMask   0x0F

#define HIDPHandshakeSuccessful             0x00
#define HIDPHandshakeNotReady               0x01
#define HIDPHandshakeErrInvalidReportID     0x02
#define HIDPHandshakeErrUnsupportedRequest  0x03
#define HIDPHandshakeErrInvalidParameter    0x04
#define HIDPHandshakeErrUnknown             0x0E
#define HIDPHandshakeErrFatal               0x0F

#define HIDPControlNOP                      0x00
#define HIDPControlHardReset                0x01
#define HIDPControlSoftReset                0x02
#define HIDPControlSuspend                  0x03
#define HIDPControlExitSuspend              0x04
#define HIDPControlVirtualCableUnplug       0x05

#define HIDPGetReportSizeBit                0x08

#define HIDPReportTypeOther             0x00
#define HIDPReportTypeInput             0x01
#define HIDPReportTypeOutput            0x02
#define HIDPReportTypeFeature           0x03
#define HIDPReportTypeMask              0x03

#define HIDPSetProtocolBit              0x01

#define HIDP_NORMALIZE(val) (UInt8) ((val) < -127	\
                                     ? -127		\
                                     : ((val) > 127	\
                                        ? 127		\
                                        : (val)))

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct HIDPGlobals {

    /* True if we are currently using the boot protocol.  */
    Boolean bootProtocol;

    /* The idle repeat rate.  */
    UInt8 idleRate;

    /* True if host suspended us.  */
    Boolean suspended;


    /* Current keyboard LED state.  */
    UInt8 leds;

    /* Current modifier on/off state.  */
    UInt16 modifiers;

    /* Current modifier toggle state.  */
    ModState modstate[8];

    /* Timer for escaping sticky state.  */
    Timer modtimer;

    /* List of keys that are currently pressed.  */
    UInt16 pressed[KEYBUFFER_SIZE];


    /* Current mouse button state.  */
    UInt8 buttonState;

    /* Current mouse movement.  */
    Int16 X;
    Int16 Y;
    Int16 wheel;


    /* True if mouse position changed since the last report was sent.  */
    Boolean posChanged;

} HIDPGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static MemHandle SendHandshake (Globals *g, UInt8 code)
    HIDPROTOCOL_SECTION;

static MemHandle SendData (Globals *g, MsgQueuePtr q,
                           UInt8 reportType,
                           UInt8 *data, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle SendInputReport (Globals *g, MsgQueuePtr q,
				  UInt8 reportID)
    HIDPROTOCOL_SECTION;

static MemHandle SendOutputReport (Globals *g, MsgQueuePtr q,
				   UInt8 reportID)
    HIDPROTOCOL_SECTION;

static MemHandle SendFeatureReport (Globals *g, MsgQueuePtr q,
                                    UInt8 reportID)
    HIDPROTOCOL_SECTION;

/* Input processors.  */

static MemHandle ProcessOutputReport (Globals *g, MemHandle h,
                                      UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessFeatureReport (Globals *g, MemHandle h,
                                       UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessHandshake (Globals *g, MemHandle h,
                                   UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessControl (Globals *g, MemHandle h,
                                 UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessGetReport (Globals *g, MemHandle h,
                                   UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessSetReport (Globals *g, MemHandle h,
                                   UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessGetProtocol (Globals *g, MemHandle h,
                                     UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessSetProtocol (Globals *g, MemHandle h,
                                     UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessGetIdle (Globals *g, MemHandle h,
                                 UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessSetIdle (Globals *g, MemHandle h,
                                 UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessData (Globals *g, MemHandle h,
                              UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessDatC (Globals *g, MemHandle h,
                              UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessControlMessage (Globals *g, MemHandle h,
                                        UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

static MemHandle ProcessInterruptMessage (Globals *g, MemHandle h,
                                          UInt8 *buf, UInt16 length)
    HIDPROTOCOL_SECTION;

/* Change g_modifiers to correspond to MODIFIERS.  Send Input reports
   as necessary.  */
static MemHandle SetModifiers (Globals *g, UInt16 modifiers)
    HIDPROTOCOL_SECTION;

static MemHandle ModifierPress (Globals *g, UInt16 keycode)
    HIDPROTOCOL_SECTION;

static MemHandle ModifierRelease (Globals *g, UInt16 keycode)
    HIDPROTOCOL_SECTION;

static MemHandle NormalKey (Globals *g)
    HIDPROTOCOL_SECTION;

static UInt32 StickyTimer (Globals *g, UInt32 index)
    HIDPROTOCOL_SECTION;

static char *ModString (Globals *g, int i)
    HIDPROTOCOL_SECTION __attribute__ ((unused));

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_bootProtocol      hidp->bootProtocol
#define g_idleRate          hidp->idleRate
#define g_pressed           hidp->pressed
#define g_modifiers         hidp->modifiers
#define g_modstate          hidp->modstate
#define g_modtimer          hidp->modtimer
#define g_buttonState       hidp->buttonState
#define g_X                 hidp->X
#define g_Y                 hidp->Y
#define g_wheel             hidp->wheel
#define g_posChanged        hidp->posChanged
#define g_suspended         hidp->suspended
#define g_leds              hidp->leds

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static MemHandle
SendHandshake (Globals *g, UInt8 code)
{
    MemHandle handle = CheckedMemHandleNew (1, true);
    UInt8 *d = MemHandleLock (handle);
    d[0] = HIDPTypeHandshake | (code & HIDPParameterMask);
    MemPtrUnlock (d);

    MsgAddHandle (g_controlOutQueue, handle);
    MDEBUG (handle, "Sending handshake %d", code);
    return handle;
}



static MemHandle
SendData (Globals *g, MsgQueuePtr q, UInt8 reportType,
          UInt8 *data, UInt16 length)
{
    MemHandle handle = CheckedMemHandleNew (length + 1, true);
    UInt8 *d = MemHandleLock (handle);
    d[0] = (HIDPTypeData | (reportType & HIDPReportTypeMask));
    MemMove (d + 1, data, length);
    MemPtrUnlock (d);

    MsgAddHandle (q, handle);
    return handle;
}



MemHandle
SendInputReport (Globals *g, MsgQueuePtr q, UInt8 reportID)
{
    UInt8 msg[20];
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    switch (reportID) {
	case 1:                     /* Keyboard report */
	{
	    MemHandle result;
	    int i, j, size, maxkeys;
	    msg[0] = 1;             /* Report ID */
	    msg[1] = g_modifiers;
	    msg[2] = 0;             /* Reserved */
	    i = 3;
	    for (j = 0; j < KEYBUFFER_SIZE; j++) {
		msg[i++] = g_pressed[j];
	    }

	    size = 9;
	    maxkeys = 6;

	    if (g_pressed[maxkeys]) {
		/* Report keyboard rollover. */
		LOG ("Too many keys pressed at once!");
		for (i = 3; i < 3 + maxkeys; i++)
		    msg[i] = KeyCodeErrorRollOver;
	    }

	    result = SendData (g, q, HIDPReportTypeInput, msg, size);
	    MDEBUG (result, "Sending %skeyboard input report",
		    (g_bootProtocol ? "boot " : ""));
	    return result;
	}

	case 2:                     /* Mouse report */
	{
	    MemHandle result;
	    Int8 x = HIDP_NORMALIZE (g_X);
	    Int8 y = HIDP_NORMALIZE (g_Y);
	    Int8 wheel = HIDP_NORMALIZE (g_wheel);
	    msg[0] = 2;                    /* Report ID */
	    msg[1] = g_buttonState;        /* Button state */
	    msg[2] = x;                    /* X displacement */
	    msg[3] = y;                    /* Y displacement */
	    msg[4] = wheel;                /* Wheel displacement */
	    g_posChanged = false;
	    result = SendData (g, q, HIDPReportTypeInput, msg, 5);
	    MDEBUG (result, "Sending mouse input report");
	    g_X -= x;
	    g_Y -= y;
	    g_wheel -= wheel;
	    return result;
	}

	case 3:                     /* Game pad report */
	{
	    // TODO: Not implemented yet.
	    MemHandle result;
	    msg[0] = 3;         /* Report ID */
	    msg[1] = 0;         /* D-Pad. */
	    msg[2] = 0;         /* Buttons. */
	    result = SendData (g, q, HIDPReportTypeInput, msg, 3);
	    MDEBUG (result, "Sending game pad input report");
	    return result;
	}

	case 4:                     /* Consumer control */
	{
	    // TODO: Not implemented yet.
	    MemHandle result;
	    msg[0] = 4;         /* Report ID */
	    msg[1] = 50;        /* Volume. */
	    result = SendData (g, q, HIDPReportTypeInput, msg, 3);
	    MDEBUG (result, "Sending consumer control input report");
	    return result;
	}

	default:
	    DEBUG ("Unknown input report ID %d", reportID);
	    return 0;
    }
}



static MemHandle
SendOutputReport (Globals *g, MsgQueuePtr q, UInt8 reportID)
{
    UInt8 msg[2];
    HIDPGlobals *hidp = g->hidp;
    MemHandle result;

    if (hidp == 0) return 0;

    switch (reportID) {
	case 1:                     /* LED report */
	    msg[0] = 1;
	    msg[1] = g_leds;

	    result = SendData (g, q, HIDPReportTypeOutput, msg, 2);
	    MDEBUG (result, "Sending LED output report");
	    return result;

	default:
	    return 0;
    }
}



static MemHandle
SendFeatureReport (Globals *g, MsgQueuePtr q, UInt8 reportID)
{
    UInt8 msg[2];
    HIDPGlobals *hidp = g->hidp;
    MemHandle result;

    if (hidp == 0) return 0;

    switch (reportID) {
	case 5:                     /* Battery report */
	    msg[0] = 5;
	    SysBatteryInfo (false, NULL, NULL, NULL, NULL, NULL, &msg[1]);
	    result = SendData (g, q, HIDPReportTypeFeature, msg, 2);
	    MDEBUG (result, "Sending battery report (%d%%)", (Int16)msg[1]);
	    return result;

	default:
	    return 0;
    }
}




static MemHandle
ProcessOutputReport (Globals *g, MemHandle h,
                     UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    UInt8 reportID;

    if (hidp == 0) return 0;

    reportID = buf[0];

    switch (reportID) {
	case 1:
	    g_leds = buf[1];
	    MDEBUG (h, "LED state: %x", g_leds);
	    ENQUEUE (CmdLedsChanged);
	    return 0;
	default:
	    MLOG (h, "Unknown output report ID %d", (Int16)reportID);
	    return 0;
    }
}



static MemHandle
ProcessFeatureReport (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    UInt8 reportID;

    if (hidp == 0) return 0;

    reportID = buf[0];
    switch (reportID) {
	case 5:                     /* Battery info */
	    MDEBUG (h, "Error: Received battery report (%d%%)",
		    (Int16)buf[1]);
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
	default:
	    MDEBUG (h, "Unknown feature report ID %d", (Int16)reportID);
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
}



static MemHandle
ProcessHandshake (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    switch (buf[0] & HIDPParameterMask) {
	case HIDPHandshakeSuccessful:
	    MDEBUG (h, "Handshake: Successful");
	    break;
	case HIDPHandshakeNotReady:
	    MDEBUG (h, "Handshake: Not ready");
	    break;
	case HIDPHandshakeErrInvalidReportID:
	    MDEBUG (h, "Handshake: Invalid Report ID Error");
	    break;
	case HIDPHandshakeErrUnsupportedRequest:
	    MDEBUG (h, "Handshake: Unsupported Request Error");
	    break;
	case HIDPHandshakeErrInvalidParameter:
	    MDEBUG (h,"Handshake: Invalid Parameter Error");
	    break;
	case HIDPHandshakeErrUnknown:
	    MDEBUG (h, "Handshake: Unknown Error");
	    break;
	case HIDPHandshakeErrFatal:
	    MDEBUG (h, "Handshake: Fatal Error");
	    break;
	default:
	    MDEBUG (h, "Unknown Handshake parameter %x",
		    buf[0] & HIDPParameterMask);
	    /* Silently ignore.  */
	    break;
    }
    /* We never send answers to handshake messages.  */
    return 0;
}



static MemHandle
ProcessControl (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    switch (buf[0] & HIDPParameterMask) {
	case HIDPControlNOP:
	    MDEBUG (h, "Control=NOP");
	    return SendHandshake (g, HIDPHandshakeSuccessful);
	case HIDPControlHardReset:
	    MDEBUG (h, "Control=HARD_RESET");
	    f_HIDPReset (g);
	    // TODO: Maybe disconnect?
	    return SendHandshake (g, HIDPHandshakeSuccessful);
	case HIDPControlSoftReset:
	    MDEBUG (h, "Control=SOFT_RESET");
	    f_HIDPReset (g);
	    // TODO: Maybe disconnect?
	    return SendHandshake (g, HIDPHandshakeSuccessful);
	case HIDPControlSuspend:
	    MDEBUG (h, "Control=SUSPEND");
	    g_suspended = true;
	    return SendHandshake (g, HIDPHandshakeSuccessful);
	case HIDPControlExitSuspend:
	    MDEBUG (h, "Control=EXIT_SUSPEND");
	    g_suspended = false;
	    return SendHandshake (g, HIDPHandshakeSuccessful);
	case HIDPControlVirtualCableUnplug:
	    MDEBUG (h, "Control=VIRTUAL_CABLE_UNPLUG");
	    ENQUEUE (CmdUnplugReceived);
	    return 0;
	default:
	    MDEBUG (h, "Unknown Control parameter %x",
		    buf[0] & HIDPParameterMask);
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
}



static MemHandle
ProcessGetReport (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    Boolean size;
    UInt8 reportID;
    MemHandle answer;

    if (length < 2) {
        /* No report ID specified.  */
        MDEBUG (h, "GET_REPORT with no report ID");
        return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
    reportID = buf[1];
    size = (buf[0] & HIDPGetReportSizeBit);

    /* Construct an answer.  */
    switch (buf[0] & HIDPReportTypeMask) {
	case HIDPReportTypeInput:
	    MDEBUG (h, "GET_REPORT:Input report %d", reportID);
	    answer = SendInputReport (g, g_controlOutQueue, reportID);
	    if (!answer)
		answer = SendHandshake (g, HIDPHandshakeErrInvalidParameter);
	    return answer;

	case HIDPReportTypeOutput:
	    MDEBUG (h, "GET_REPORT:Output report %d", reportID);
	    answer = SendOutputReport (g, g_controlOutQueue, reportID);
	    if (!answer)
		answer = SendHandshake (g, HIDPHandshakeErrInvalidParameter);
	    return answer;

	case HIDPReportTypeFeature:
	    MDEBUG (h, "GET_REPORT:Feature report %d", reportID);
	    answer = SendFeatureReport (g, g_controlOutQueue, reportID);
	    if (!answer)
		answer = SendHandshake (g, HIDPHandshakeErrInvalidParameter);
	    return answer;

	default:
	    MDEBUG (h, "GET_REPORT:Unknown report type");
	    /* TODO */
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
}



static MemHandle
ProcessSetReport (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    UInt8 reportID;
    HIDPGlobals *hidp = g->hidp;

    if (hidp == 0) return 0;

    if (length < 2) {
        /* No report ID specified.  */
        MDEBUG (h, "SET_REPORT with no report ID");
        return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
    reportID = buf[1];

    switch (buf[0] & HIDPReportTypeMask) {
	case HIDPReportTypeInput:
	    MDEBUG (h, "SET_REPORT:Input report %d", reportID);
	    /* TODO */
	    return SendHandshake (g, HIDPHandshakeSuccessful);

	case HIDPReportTypeOutput:
	    MDEBUG (h, "SET_REPORT:Output report %d", reportID);
	    ProcessOutputReport (g, h, buf + 1, length - 1);
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);

	case HIDPReportTypeFeature:
	    MDEBUG (h, "SET_REPORT:Feature report %d", reportID);
	    return ProcessFeatureReport (g, h, buf + 1, length - 1);

	default:
	    MDEBUG (h, "SET_REPORT:Unknown report type");
	    /* TODO */
	    return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
}



static MemHandle
ProcessGetProtocol (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    UInt8 value;
    HIDPGlobals *hidp = g->hidp;
    MemHandle answer;

    if (hidp == 0) return 0;

    value = (g_bootProtocol ? 0 : 1);
    MDEBUG (h, "GET_PROTOCOL");
    answer =  SendData (g, g_controlOutQueue,
			HIDPReportTypeOther, &value, 1);
    MDEBUG (answer, "Sending DATA(ReportOther): %2x (%s)", value,
            (g_bootProtocol ? "Boot Protocol" : "Report Protocol"));
    return answer;
}



static MemHandle
ProcessSetProtocol (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    g_bootProtocol = ((buf[0] & HIDPSetProtocolBit) ? false : true);
    MDEBUG (h, "SET_PROTOCOL: %s", (g_bootProtocol ? "Boot" : "Report"));
    return SendHandshake (g, HIDPHandshakeSuccessful);
}



static MemHandle
ProcessGetIdle (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    MemHandle answer;
    if (hidp == 0) return 0;

    MDEBUG (h, "GET_IDLE: %d", g_idleRate);
    answer = SendData (g, g_controlOutQueue,
		       HIDPReportTypeOther, &g_idleRate, 1);
    MDEBUG (answer, "Sending DATA(ReportOther): %2x (IDLE=%d)",
            g_idleRate, g_idleRate);
    return answer;
}



static MemHandle
ProcessSetIdle (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    if (length < 2) {
        MDEBUG (h, "SET_IDLE with no value");
        return SendHandshake (g, HIDPHandshakeErrInvalidParameter);
    }
    MDEBUG (h, "SET_IDLE: %d", buf[1]);
    g_idleRate = buf[1];
    return SendHandshake (g, HIDPHandshakeSuccessful);
}



static MemHandle
ProcessData (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    switch (buf[0] & HIDPReportTypeMask) {
	case HIDPReportTypeOutput:
	    MDEBUG (h, "DATA(ReportOutput) %d", buf[1]);
	    return ProcessOutputReport (g, h, buf + 1, length - 1);

	case HIDPReportTypeInput:
	    MDUMP (LogDebug, h, "??? DATA(ReportInput)", buf, length);
	    return 0;

	case HIDPReportTypeFeature:
	    MDUMP (LogDebug, h, "??? DATA(ReportFeature)", buf, length);
	    return 0;

	case HIDPReportTypeOther:
	    MDUMP (LogDebug, h, "??? DATA(ReportOther)", buf, length);
	    return 0;

	default:
	    MDUMP (LogDebug, h, "??? DATA(???)", buf, length);
	    return 0;
    }

}



static MemHandle
ProcessDatC (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    MDUMP (LogDebug, h, "??? DATC", buf, length);
    return 0;
}



static MemHandle
ProcessControlMessage (Globals *g, MemHandle h, UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    switch (buf[0] & HIDPTypeMask) {
	case HIDPTypeHandshake:
	    return ProcessHandshake (g, h, buf, length);

	case HIDPTypeHIDControl:
	    return ProcessControl (g, h, buf, length);

	case HIDPTypeGetReport:
	    return ProcessGetReport (g, h, buf, length);

	case HIDPTypeSetReport:
	    return ProcessSetReport (g, h, buf, length);

	case HIDPTypeGetProtocol:
	    return ProcessGetProtocol (g, h, buf, length);

	case HIDPTypeSetProtocol:
	    return ProcessSetProtocol (g, h, buf, length);

	case HIDPTypeGetIdle:
	    return ProcessGetIdle (g, h, buf, length);

	case HIDPTypeSetIdle:
	    return ProcessSetIdle (g, h, buf, length);

	case HIDPTypeData:
	    MDEBUG (h, "Unexpected DATA on control channel");
	    /* Silently ignore.  */
	    return 0;

	case HIDPTypeDatC:
	    // TODO
	    MDEBUG (h, "Unexpected DATC on control channel");
	    /* Silently ignore.  */
	    return 0;

	default:
	    MDUMP (LogDebug, h, "??? CTRL", buf, length);
	    return SendHandshake (g, HIDPHandshakeErrUnsupportedRequest);
    }
}



static MemHandle
ProcessInterruptMessage (Globals *g, MemHandle h,
                         UInt8 *buf, UInt16 length)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    if ((buf[0] & HIDPTypeMask) == HIDPTypeData) {
        return ProcessData (g, h, buf, length);
    }
    else if ((buf[0] & HIDPTypeMask) == HIDPTypeDatC) {
        return ProcessDatC (g, h, buf, length);
    }
    else {
        MDUMP (LogDebug, h, "??? INTR", buf, length);
        /* Silently ignore.  */
        return 0;
    }
}



static MemHandle
SetModifiers (Globals *g, UInt16 modifiers)
{
    HIDPGlobals *hidp = g->hidp;
    MemHandle result = 0;
    if (hidp == 0) return 0;

    if (modifiers == ModCurrent
        || modifiers == g_modifiers)
        return 0;

    TRACE ("***SetModifiers %x", modifiers);

    if (modifiers & ModCurrent) {
        /* Shift. */
        if ((modifiers & ModShiftOn) && !(g_modifiers & ModShiftMask)) {
            g_modifiers |= ModLeftShift;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
        if ((modifiers & ModShiftOff) && (g_modifiers & ModLeftShift)) {
            g_modifiers &= ~ModLeftShift;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
        if ((modifiers & ModShiftOff) && (g_modifiers & ModRightShift)) {
            g_modifiers &= ~ModRightShift;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        /* Right alt. */
        if ((modifiers & ModRightAltOn) && !(g_modifiers & ModRightAlt)) {
            g_modifiers |= ModRightAlt;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
        if ((modifiers & ModRightAltOff) && (g_modifiers & ModRightAlt)) {
            g_modifiers &= ~ModRightAlt;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
    }
    else {
        if ((g_modifiers & ModLeftControl) != (modifiers & ModLeftControl)) {
            g_modifiers ^= ModLeftControl;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModLeftShift) != (modifiers & ModLeftShift)) {
            g_modifiers ^= ModLeftShift;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModLeftAlt) != (modifiers & ModLeftAlt)) {
            g_modifiers ^= ModLeftAlt;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModLeftGUI) != (modifiers & ModLeftGUI)) {
            g_modifiers ^= ModLeftGUI;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModRightControl)
            != (modifiers & ModRightControl)) {
            g_modifiers ^= ModRightControl;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModRightShift) != (modifiers & ModRightShift)) {
            g_modifiers ^= ModRightShift;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModRightAlt) != (modifiers & ModRightAlt)) {
            g_modifiers ^= ModRightAlt;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        if ((g_modifiers & ModRightGUI) != (modifiers & ModRightGUI)) {
            g_modifiers ^= ModRightGUI;
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
    }
    return result;
}



static MemHandle
ModifierPress (Globals *g, UInt16 keycode)
{
    HIDPGlobals *hidp = g->hidp;
    int index = keycode - KeyCodeLeftControl;
    UInt16 bit;
    UInt16 modifiers;

    if (hidp == 0) return 0;
    if (index < 0 || index > 7)
        return 0;
    bit = 1 << index;

    TRACE ("***ModifierPress %x[%x,%s]",
           g_modifiers, bit, HIDPModStateStr (g_modstate[index]));
    modifiers = g_modifiers;

    switch (g_modstate[index]) {
	case ModReleased:
	default:
	    g_modstate[index] = ModPressed;
	    modifiers |= bit;
	    break;

	case ModPressed:
	    break;

	case ModSticky:
	    //TimerCancel (g_modtimer);
	    g_modstate[index] = ModStickyPressed;
	    break;

	case ModStickyPressed:
	    break;

	case ModLocked:
	    g_modstate[index] = ModLockedPressed;
	    modifiers &= ~bit;
	    break;

	case ModLockedPressed:
	    break;
    }
    TRACE ("***New state: %x[%x,%s]",
           modifiers, bit, HIDPModStateStr (g_modstate[index]));
    return SetModifiers (g, modifiers);
}



static MemHandle
ModifierRelease (Globals *g, UInt16 keycode)
{
    HIDPGlobals *hidp = g->hidp;
    int index = keycode - KeyCodeLeftControl;
    UInt16 bit;
    UInt16 modifiers;

    if (hidp == 0) return 0;
    if (index < 0 || index > 7)
        return 0;
    bit = 1 << index;

    TRACE ("***ModifierRelease %x[%x,%s]",
           g_modifiers, bit, HIDPModStateStr (g_modstate[index]));
    modifiers = g_modifiers;

    switch (g_modstate[index]) {
	case ModReleased:
	default:
	    modifiers &= ~bit;
	    break;

	case ModPressed:
	    g_modstate[index] = ModSticky;
	    TimerReschedule (g_modtimer,
			     TimGetTicks() + STICKY_TIMEOUT);
	    break;

	case ModSticky:
	    //TimerCancel (g_modtimer);
	    g_modstate[index] = ModReleased;
	    modifiers &= ~bit;
	    break;

	case ModStickyPressed:
	    g_modstate[index] = ModLocked;
	    break;

	case ModLocked:
	    modifiers |= bit;
	    break;

	case ModLockedPressed:
	    g_modstate[index] = ModReleased;
	    break;
    }
    TRACE ("***New state: %x[%x,%s]",
           modifiers, bit, HIDPModStateStr (g_modstate[index]));
    return SetModifiers (g, modifiers);
}



static MemHandle
NormalKey (Globals *g)
{
    HIDPGlobals *hidp = g->hidp;
    MemHandle result = 0;
    int index;

    if (hidp == 0) return 0;

    TRACE ("***NormalKey");
    for (index = 0; index < 8; index++) {
        UInt16 bit = 1 << index;
        UInt16 modifiers = g_modifiers;
        MemHandle h = 0;
        switch (g_modstate[index]) {
	    case ModReleased:
	    default:
		break;

	    case ModPressed:
		g_modstate[index] = ModReleased;
		break;

	    case ModSticky:
		//TimerCancel (g_modtimer);
		g_modstate[index] = ModReleased;
		modifiers &= ~bit;
		break;

	    case ModStickyPressed:
		g_modstate[index] = ModReleased;
		break;

	    case ModLocked:
		break;

	    case ModLockedPressed:
		g_modstate[index] = ModLocked;
		break;
        }
        if (modifiers != g_modifiers) {
            TRACE ("***New state: %x[%x,%s]",
                   modifiers, bit, HIDPModStateStr (g_modstate[index]));
            h = SetModifiers (g, modifiers);
            if (h)
                result = h;
        }
    }
    TRACE ("***Final state: %x", g_modifiers);
    if (result)
        ENQUEUE (CmdModifiersChanged);
    return result;
}


static UInt32
StickyTimer (Globals *g, UInt32 userdata)
{
    HIDPGlobals *hidp = g->hidp;
    int i;
    Boolean changed = false;
    if (hidp == 0) return 0;

    DEBUG ("StickyTimer triggered");
    for (i = 0; i < 8; i++) {
	if (g_modstate[i] == ModSticky) {
	    DEBUG ("Sticky modifier for %s timed out", ModString (g, i));
	    g_modstate[i] = ModReleased;
	    SetModifiers (g, g_modifiers & ~(1 << i));
	    changed = true;
	}
    }
    if (changed)
	ENQUEUE (CmdModifiersChanged);
    return 0;
}



static char *
ModString (Globals *g, int i) 
{
    switch (i) {
	case 0:
	    return "Left Control";
	case 1:
	    return "Left Shift";
	case 2:
	    return "Left Alt";
	case 3:
	    return "Left GUI";
	case 4:
	    return "Right Control";
	case 5:
	    return "Right Shift";
	case 6:
	    return "Right Alt";
	case 7:
	    return "Right GUI";
	default:
	    PANIC ("Invalid modifier value: %d", i);
	    return "Invalid";
    }
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


void
f_HIDPInit (Globals *g)
{
    HIDPGlobals *hidp;
    if (g->hidp)
        f_HIDPClose (g);
    g->hidp = CheckedMemPtrNew (sizeof (HIDPGlobals), true);
    hidp = g->hidp;

    g_modtimer = TimerNew ("Sticky Modifier", 0, StickyTimer, 0);

    f_HIDPReset (g);
    DEBUG ("HIDP ready");
}


void
f_HIDPClose (Globals *g)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return;

    if (g_modtimer) {
	TimerFree (g_modtimer);
	g_modtimer = 0;
    }

    MemPtrFree (hidp);
    g->hidp = 0;
    DEBUG ("HIDP closed");
}



MemHandle
f_HIDPProcessMessages (Globals *g)
{
    MemHandle handle;
    UInt16 length;
    UInt8 *d;
    MemHandle answer = 0, answer2;
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    while (MsgAvailable (g_controlInQueue)) {
        handle = MsgGet (g_controlInQueue);
        length = MemHandleSize (handle);
        d = MemHandleLock (handle);
        if (length == 0) {
            MDEBUG (handle, "Empty message received");
            continue;
        }
        answer2 = ProcessControlMessage (g, handle, d, length);
        if (answer2)
            answer = answer2;
        MemPtrUnlock (d);
        MemHandleFree (handle);
        MsgProcessed (g_controlInQueue, handle);
    }

    while (MsgAvailable (g_interruptInQueue)) {
        handle = MsgGet (g_interruptInQueue);
        length = MemHandleSize (handle);
        d = MemHandleLock (handle);
        if (length == 0) {
            DEBUG ("Empty message received");
            continue;
        }
        answer2 = ProcessInterruptMessage (g, handle, d, length);
        if (answer2)
            answer = answer2;
        MemPtrUnlock (d);
        MemHandleFree (handle);
        MsgProcessed (g_interruptInQueue, handle);
    }
    return answer;
}



MemHandle
f_HIDPKeyboard (Globals *g, Action action, KeyCode keycode, UInt16 modifiers)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    /* Handle modifier keys specially.  */
    if (keycode >= KeyCodeLeftControl
        && keycode <= KeyCodeRightGUI) {
        ENQUEUE (CmdModifiersChanged);
        if (action == ActPress)
            return ModifierPress (g, keycode);
        else if (action == ActRelease)
            return ModifierRelease (g, keycode);
        else if (action == ActClick) {
            ModifierPress (g, keycode);
            return ModifierRelease (g, keycode);
        }
        else if (action == ActDoubleClick) {
            ModifierPress (g, keycode);
            ModifierRelease (g, keycode);
            ModifierPress (g, keycode);
            return ModifierRelease (g, keycode);
        }
        return 0;
    }
    else {                      /* Normal keys. */
        UInt16 save = g_modifiers;
        int i;
        Boolean found = false;
        MemHandle result = 0, h;

        /* Setup modifiers.  */
        SetModifiers (g, modifiers);

        /* Handle clicks and doubleclicks by recursive calls.  */
        if (action == ActClick) {
            TRACE ("***Click %d m%x", keycode, modifiers);
            /* Note that we use g_modifiers here instead of ModCurrent;
               this is necessary in order for the NormalKey call below
               to work correctly. */
            f_HIDPKeyboard (g, ActPress, keycode, g_modifiers);
            result = f_HIDPKeyboard (g, ActRelease, keycode, g_modifiers);
        }
        else if (action == ActDoubleClick) {
            TRACE ("***DoubleClick %d m%x", keycode, modifiers);
            /* See above comment on g_modifiers.  */
            f_HIDPKeyboard (g, ActPress, keycode, g_modifiers);
            f_HIDPKeyboard (g, ActRelease, keycode, g_modifiers);
            f_HIDPKeyboard (g, ActPress, keycode, g_modifiers);
            result = f_HIDPKeyboard (g, ActRelease, keycode, g_modifiers);
        }
        else if (action == ActPress) {
            TRACE ("***Press %d m%x", keycode, modifiers);
            for (i = 0; i < KEYBUFFER_SIZE; i++) {
                if (g_pressed[i] == keycode) {
                    DEBUG ("Key %x already pressed", keycode);
                    found = true;
                    break;
                }
                if (g_pressed[i] == 0) {
                    g_pressed[i] = keycode;
                    found = true;
                    break;
                }
            }
            if (!found)
                /* SendInputReport will report the rollover
                   automatically.  */
                DEBUG ("Too many keys pressed at once");

            /* Send data immediately.  If we would delay sending here,
               then we would lose key presses when their corresponding
               release comes before the press event was sent.  */
            result = SendInputReport (g, g_interruptOutQueue, 1);
        }
        else if (action == ActRelease) {
            TRACE ("***Release %d m%x", keycode, modifiers);
            for (i = 0; i < KEYBUFFER_SIZE; i++) {
                if (g_pressed[i] == keycode) {
                    found = true;
                    for (; i < KEYBUFFER_SIZE - 1; i++) {
                        g_pressed[i] = g_pressed[i + 1];
                    }
                    g_pressed[i] = 0;
                    break;
                }
            }
            if (!found)
                DEBUG ("Key %x released without keypress", keycode);

            /* Send an LED update now in case the host forgets to
               send one and the UI button gets stuck in the selected
               state.  It is better to get stuck unselected.  */
            switch (keycode) {
		case KeyCodeCapsLock:
		case KeyCodeKeypadNumLock:
		case KeyCodeScrollLock:
		    ENQUEUE (CmdLedsChanged);
		    break;
		default:
		    break;
            }

            /* Send data immediately.  If we would delay sending here,
               then we would lose key presses when their corresponding
               release comes before the press event was sent.  */
            if (found)
                result = SendInputReport (g, g_interruptOutQueue, 1);
        }

        /* Restore modifiers, if changed.  */
        if (g_modifiers != save) {
            h = SetModifiers (g, save);
            if (h) result = h;
        }
        if ((action != ActPress) && (modifiers & ModCurrent)) {
            h = NormalKey (g);
            if (h) result = h;
        }
        return result;
    }
}



MemHandle
f_HIDPMouseMove (Globals *g, Int16 x, Int16 y)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    TRACE ("***Mouse moved (%d, %d)", x, y);
    g_X += x;
    g_Y += y;
    g_posChanged = true;

    return BTDataReady();
}



MemHandle
f_HIDPButton (Globals *g, Action action, UInt16 button, UInt16 modifiers)
{
    HIDPGlobals *hidp = g->hidp;
    UInt16 save;
    MemHandle result = 0, h;
    if (hidp == 0) return 0;
    save = g_modifiers;

    TRACE ("***Button %d %s m%x", button, HIDPActionStr (action), modifiers);
    if (button > 3 || button == 0)
        return 0;

    result = SetModifiers (g, modifiers);
    if (action == ActPress)
        g_buttonState |= (1 << (button - 1));
    else if (action == ActRelease)
        g_buttonState &= ~(1 << (button - 1));
    else if (action == ActClick) {
        f_HIDPButton (g, ActPress, button, ModCurrent);
        f_HIDPButton (g, ActRelease, button, ModCurrent);
    }
    else {
        f_HIDPButton (g, ActPress, button, ModCurrent);
        f_HIDPButton (g, ActRelease, button, ModCurrent);
        f_HIDPButton (g, ActPress, button, ModCurrent);
        f_HIDPButton (g, ActRelease, button, ModCurrent);
    }
    result = SendInputReport (g, g_interruptOutQueue, 2);
    h = SetModifiers (g, save);
    if (h)
        result = h;
    return result;
}



MemHandle
f_HIDPWheelMove (Globals *g, Int16 dir)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    TRACE ("***Wheel moved %d steps", dir);
    g_wheel += dir;
    g_posChanged = true;

    return BTDataReady();
}



UInt8
f_HIDPLeds (Globals *g)
{
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;
    return g_leds;
}



UInt16
f_HIDPModifiers (Globals *g)
{
    HIDPGlobals *hidp = g->hidp;
    UInt16 state;
    int i;
    if (hidp == 0) return 0;
    state = g_modifiers;
    /* Adjust state slightly to better handle LockedPressed state.  */
    for (i = 0; i < 8; i++) {
        if (g_modstate[i] == ModLockedPressed)
            state |= (1 << i);
    }
    return state;
}



ModState
f_HIDPModifierState (Globals *g, KeyCode keycode)
{
    HIDPGlobals *hidp = g->hidp;
    int index = keycode - KeyCodeLeftControl;
    if (hidp == 0) return ModReleased;
    if (index < 0 || index > 7)
        return ModReleased;
    return g_modstate[index];
}



char *
f_HIDPModStateStr (Globals *g, UInt8 state)
{
    switch (state) {
	case ModReleased:
	default:
	    return "Released";
	case ModPressed:
	    return "Pressed";
	case ModSticky:
	    return "Sticky";
	case ModStickyPressed:
	    return "StickyPressed";
	case ModLocked:
	    return "Locked";
	case ModLockedPressed:
	    return "LockedPressed";
    }
}



char *
f_HIDPActionStr (Globals *g, Action action)
{
    switch (action) {
	case ActPress:
	    return "Press";
	case ActRelease:
	    return "Release";
	case ActClick:
	    return "Click";
	case ActDoubleClick:
	    return "Double click";
	default:
	    return "Unknown action";
    }
}


MemHandle
f_HIDPSendReport (Globals *g, Boolean force)
{
    MemHandle handle = 0, h;
    HIDPGlobals *hidp = g->hidp;
    if (hidp == 0) return 0;

    if (force || g_posChanged) {
        handle = SendInputReport (g, g_interruptOutQueue, 2);
    }

    if (force) {
        h = SendInputReport (g, g_interruptOutQueue, 1);
        if (h) handle = h;
    }
    return handle;
}



MemHandle
f_HIDPReset (Globals *g)
{
    int i;
    HIDPGlobals *hidp = g->hidp;
    MemHandle result;
    if (hidp == 0) return 0;

    g_leds = 0;

    /* Release all keys.  */
    for (i = KEYBUFFER_SIZE - 1; i >= 0; i--) {
        if (g_pressed[i]) {
            g_pressed[i] = 0;
            SendInputReport (g, g_interruptOutQueue, 1);
        }
    }
    /* Clear all modifiers.  */
    SetModifiers (g, 0);
    ASSERT (g_modifiers == 0);

    for (i = 0; i < 8; i++) {
        g_modstate[i] = ModReleased;
    }
    TimerCancel (g_modtimer);
    g_X = 0;
    g_Y = 0;
    g_wheel = 0;
    g_buttonState = 0;
    result = f_HIDPSendReport (g, true);
    ENQUEUE (CmdResetHIDState);
    MDEBUG (result, "HID state reset");
    return result;
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

