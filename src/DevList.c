/**************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: DevList.c
 *
 **************************************************************************/

#define LOGLEVEL LogTrace

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "DevList.h"
#undef PRIVATE_INCLUDE

#include "BlueRemote_res.h"
#include "HardKeys.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define devListFileName         "BlueRemoteDeviceList"
#define devListFileType         'DevL'
#define devListFileVersion      0x0001

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct DevListGlobals {
    DmOpenRef db;
    MemHandle dev;
    UInt16 size;
    UInt32 modnum;
} DevListGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static Boolean AddressEquals (BtLibDeviceAddressType *a,
                              BtLibDeviceAddressType *b)
    DEVLIST_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_db        g->devlist->db
#define g_dev       g->devlist->dev
#define g_size      g->devlist->size
#define g_modnum    g->devlist->modnum

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static Boolean
AddressEquals (BtLibDeviceAddressType *a, BtLibDeviceAddressType *b)
{
    int i;
    if (a == b)
        return true;
    if (a == NULL || b == NULL)
        return false;
    for (i = 0; i < btLibDeviceAddressSize; i++)
        if (a->address[i] != b->address[i])
            return false;
    return true;
}



static Boolean
DontHandleEvent (EventType *e) 
{
    return false;
}


static void
DrawDetails (Device *d)
{
    char buf[dowLongDateStrLength + timeStringLength + 2];
    DateFormatType df = PrefGetPreference (prefLongDateFormat);
    TimeFormatType tf = PrefGetPreference (prefTimeFormat);
    DateTimeType dt;
    UInt16 l;
    
    WinDrawTruncChars (d->nameStr, StrLen (d->nameStr), 9, 33, 156 - 9);
    WinDrawTruncChars (d->addressStr, StrLen (d->addressStr),
		       9, 63, 156 - 9);

    TimSecondsToDateTime (d->lastConnected, &dt);
    DateToDOWDMFormat (dt.month, dt.day, dt.year, df, buf);
    l = StrLen (buf);
    buf[l++] = ' ';
    TimeToAscii (dt.hour, dt.minute, tf, buf + l);
    WinDrawTruncChars (buf, StrLen (buf), 9, 93, 156 - 9);
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/



void
f_DevListInit (Globals *g)
{
    Err err;
    LocalID dbID;
    UInt16 cardNo;
    UInt16 version;

    if (g->devlist)
        f_DevListClose (g);
    DEBUG ("DevListInit");
    g->devlist = CheckedMemPtrNew (sizeof (DevListGlobals), true);

    g_modnum = TimGetSeconds();

    /* Open or create the device list database.  */
reopen:
    SysCurAppDatabase (&cardNo, &dbID);
    g_db = DmOpenDatabaseByTypeCreator (devListFileType,
                                        appFileCreator,
                                        dmModeReadWrite);
    if (!g_db) {
        DEBUG ("Creating device list database");
        err = DmCreateDatabase (cardNo, devListFileName,
                                appFileCreator, devListFileType,
                                false);
        g_db = DmOpenDatabaseByTypeCreator (devListFileType,
                                            appFileCreator,
                                            dmModeReadWrite);
        PANICIF (g_db == 0, "Could not create device list database");
        err = DmOpenDatabaseInfo (g_db, &dbID, NULL, NULL, &cardNo, NULL);
        PANICIF (err, "Error getting database info: %x", err);
        version = devListFileVersion;
        DmSetDatabaseInfo (cardNo, dbID,
                           NULL, NULL, &version,
                           NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL, NULL);
    }
    else {
	DEBUG ("Opening existing device list database");
        err = DmOpenDatabaseInfo (g_db, &dbID, NULL, NULL, &cardNo, NULL);
        PANICIF (err, "Error getting database info: %x", err);
        err = DmDatabaseInfo (cardNo, dbID,
                              NULL, NULL, &version,
                              NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL);
        PANICIF (err, "Error getting database info: %x", err);
        if (version != devListFileVersion) {
	    LOG ("DevList database has version %d, expected %d",
		 version, devListFileVersion);
            DmCloseDatabase (g_db);
            err = DmDeleteDatabase (cardNo, dbID);
            PANICIF (err, "Error deleting database: %x", err);
            goto reopen;
        }
    }
    PANICIF (!g_db, "Assertion error");


    /* Load devices.  */
    g_size = DmNumRecordsInCategory (g_db, dmAllCategories);
    if (g_size == 0) {
        g_dev = 0;
    }
    else {
        Device **d;
        int i, j;
        g_dev = CheckedMemHandleNew (g_size * sizeof (MemPtr), true);
        d = MemHandleLock (g_dev);
        for (i = 0, j = 0; i < DmNumRecords (g_db) && j < g_size; i++) {
            UInt16 attr;
            UInt32 uid;
            DmRecordInfo (g_db, i, &attr, &uid, NULL);
            if (attr & dmRecAttrDelete) {
                DEBUG ("Record %d is deleted", i);
            }
            else {
                MemHandle record = DmGetRecord (g_db, i);
                if (MemHandleSize (record) != sizeof (Device)) {
                    LOG ("Record %d had the wrong size", i);
                    DmReleaseRecord (g_db, i, false);
                    DmDeleteRecord (g_db, i);
                }
                else {
                    Device *p = MemHandleLock (record);
		    if (p->version != DEVICE_STRUCT_VERSION) {
			LOG ("Record %d is the wrong version", i);
			MemPtrUnlock (p);
			DmReleaseRecord (g_db, i, false);
			DmDeleteRecord (g_db, i);
		    }
		    else {
			d[j] = CheckedMemPtrNew (sizeof (Device), true);
			MemMove (d[j], p, sizeof (Device));
			MemPtrUnlock (p);
			DmReleaseRecord (g_db, i, false);
			d[j]->uid = uid;
			j++;
		    }
                }
            }
        }
        MemPtrUnlock (d);
    }

    if (SPREF (device) >= g_size)
        SPREF (device) = 0;
}



void
f_DevListClose (Globals *g)
{
    if (!g->devlist)
        return;
    DEBUG ("DevListClose");

    if (g_db)
        DmCloseDatabase (g_db);
    g_db = 0;

    if (g_dev) {
        Device **d = MemHandleLock (g_dev);
        int i;
        for (i = 0; i < g_size; i++) {
            MemPtrFree (d[i]);
            d[i] = 0;
        }
        MemPtrUnlock (d);
        MemHandleFree (g_dev);
        g_dev = 0;
    }
    g_size = 0;

    MemPtrFree (g->devlist);
    g->devlist = 0;

}



UInt16
f_DevListLength (Globals *g)
{
    return g_size;
}



Device *
f_DevListSelected (Globals *g)
{
    Device *r;
    Device **d;
    if (g_size == 0)
        return 0;

    if (SPREF (device) == noListSelection)
        return 0;

    if (SPREF (device) >= g_size) {
        SPREF (device) = 0;
	g_modnum++;
	ENQUEUE (CmdDeviceListChanged);
    }

    d = MemHandleLock (g_dev);
    r = d[SPREF (device)];
    MemPtrUnlock (d);
    return r;
}



void
f_DevListSetSelected (Globals *g, Device *device)
{
    Device **d;
    int i;
    if (device == 0) {
        DEBUG ("Canceled device selection");
        SPREF (device) = noListSelection;
        return;
    }

    PANICIF (g_size == 0, "Empty device list");
    d = MemHandleLock (g_dev);
    for (i = 0; i < g_size; i++) {
        if (d[i] == device) {
            SPREF (device) = i;
            break;
        }
    }
    PANICIF (i == g_size, "Selected device is not in the list");
    MemPtrUnlock (d);
    g_modnum++;
    ENQUEUE (CmdDeviceListChanged);
}



Device *
f_DevListGet (Globals *g, UInt16 id)
{
    Device **d;
    Device *res;
    if (g_size == 0 || id >= g_size)
        return 0;
    d = MemHandleLock (g_dev);
    res = d[id];
    MemPtrUnlock (d);
    return res;
}



Device *
f_DevListFindByUID (Globals *g, UInt32 uid)
{
    Device **d;
    Device *res = 0;
    int i;
    if (g_size == 0)
        return 0;
    d = MemHandleLock (g_dev);
    for (i = 0; i < g_size; i++) {
        if (d[i]->uid == uid) {
            res = d[i];
            break;
        }
    }
    MemPtrUnlock (d);
    return res;
}


Device *
f_DevListFindByAddress (Globals *g, BtLibDeviceAddressType *addr)
{
    Device **d;
    Device *res = 0;
    int i;
    if (g_size == 0)
        return 0;
    d = MemHandleLock (g_dev);
    for (i = 0; i < g_size; i++) {
        if (AddressEquals (addr, &d[i]->address)) {
            res = d[i];
            break;
        }
    }
    MemPtrUnlock (d);
    return res;
}



void
f_DevListUpdate (Globals *g, Device *device)
{
    UInt16 id;
    MemHandle h;
    void *p;
    Err err;
    PANICIF (g_db == 0, "Device list database is not open");
    ASSERT (device != NULL);

    err = DmFindRecordByID (g_db, device->uid, &id);
    PANICIF (err, "Invalid device list entry");

    h = DmGetRecord (g_db, id);
    p = MemHandleLock (h);
    DmWrite (p, 0, device, sizeof (Device));
    MemPtrUnlock (p);
    DmReleaseRecord (g_db, id, true);

    g_modnum++;
    ENQUEUE (CmdDeviceListChanged);
}



void
f_DevListDelete (Globals *g, Device *device)
{
    UInt16 id;
    Device **d;
    Err err;
    UInt16 i;
    PANICIF (g_db == 0, "Device list database is not open");
    PANICIF (g_size == 0, "Delete from empty database");
    if (device == 0)
        return;

    err = DmFindRecordByID (g_db, device->uid, &id);
    PANICIF (err, "Invalid device list entry");
    DmDeleteRecord (g_db, id);

    d = MemHandleLock (g_dev);
    for (i = 0; i < g_size; i++) {
        if (device->uid == d[i]->uid)
            break;
    }
    PANICIF (i == g_size, "Could not find record to delete");
    if (SPREF (device) == i)
        SPREF (device) = (i == 0 ? 0 : i - 1);
    else if (SPREF (device) > i)
        SPREF (device)--;
    for (; i < g_size - 1; i++) {
        d[i] = d[i+1];
    }
    g_size--;
    if (g_size == 0)
        SPREF (device) = noListSelection;
    MemPtrUnlock (d);
    MemHandleResize (g_dev, g_size * sizeof (MemPtr));

    g_modnum++;
    ENQUEUE (CmdDeviceListChanged);
}



Device *
f_DevListAdd (Globals *g, Device *device)
{
    UInt16 id;
    MemHandle r;
    void *p;
    Device **d;
    Device *res;
    PANICIF (g_db == 0, "Device list database is not open");
    ASSERT (device != NULL);

    res = f_DevListFindByAddress (g, &device->address);
    if (res) {
        UInt32 uid = res->uid;
        MemMove (res, device, sizeof (Device));
        res->uid = uid;
        f_DevListUpdate (g, res);
        return res;
    }

    id = dmMaxRecordIndex;
    r = DmNewRecord (g_db, &id, sizeof (Device));
    device->version = DEVICE_STRUCT_VERSION;
    DmRecordInfo (g_db, id, NULL, &device->uid, NULL);
    p = MemHandleLock (r);
    DmWrite (p, 0, device, sizeof (Device));
    MemPtrUnlock (p);
    DmReleaseRecord (g_db, id, true);

    if (g_size == 0) {
        g_dev = CheckedMemHandleNew (sizeof (MemPtr), true);
    }
    else {
        CheckedMemHandleResize (g_dev, (g_size + 1) * sizeof (MemPtr));
    }
    d = MemHandleLock (g_dev);
    d[g_size] = CheckedMemPtrNew (sizeof (Device), true);
    res = d[g_size];
    MemMove (d[g_size], device, sizeof (Device));
    MemPtrUnlock (d);
    g_size++;

    g_modnum++;
    ENQUEUE (CmdDeviceListChanged);

    SPREF (device) = g_size - 1;
    return res;
}




void
f_DevListMoveTop (Globals *g, Device *device)
{
    UInt16 id;
    Device **d;
    UInt16 i;
    MemPtr t;
    Err err;
    PANICIF (g_db == 0, "Device list database is not open");
    ASSERT (device != NULL);
    err = DmFindRecordByID (g_db, device->uid, &id);
    PANICIF (err, "Invalid device list entry");

    DmMoveRecord (g_db, id, 0);

    d = MemHandleLock (g_dev);
    for (i = 0; i < g_size; i++) {
        if (d[i] == device)
            break;
    }
    PANICIF (i == g_size, "Could not find record to move");
    if (SPREF (device) == i)
        SPREF (device) = 0;
    else if (SPREF (device) < i)
        SPREF (device)++;
    for (t = d[i]; i > 0; i--) {
        d[i] = d[i - 1];
    }
    d[0] = t;
    MemPtrUnlock (d);

    g_modnum++;
    ENQUEUE (CmdDeviceListChanged);
}



UInt32
f_DevListModNum (Globals *g)
{
    return g_modnum;
}



void
f_DevListDetails (Globals *g)
{
    FormType *f;
    Boolean prev = HardKeyEnable (false);
    FormActiveStateType savedstate;
    EventType event;
    Device *d = DevListSelected();
    UInt16 idx;
    
    if (!DevListSelected()) {
	SndPlaySystemSound (sndError);
	return;
    }

    FrmSaveActiveState (&savedstate);
    f = FrmInitForm (DetailsForm);
    FrmSetEventHandler (f, DontHandleEvent);

    idx = FrmGetObjectIndex (f, DetailsEncryptLinkCheckbox);
    FrmSetControlValue (f, idx, d->encryptLink);
    
    FrmSetActiveForm (f);
    FrmDrawForm (f);
    DrawDetails (d);

    while (true) {
	EvtGetEvent (&event, evtWaitForever);
	if (SysHandleEvent (&event))
	    continue;
	if (FrmDispatchEvent (&event))
	    continue;

	switch (event.eType) {
	    case appStopEvent:
		ENQUEUE (CmdExitApp);
		goto exit;
	    case frmOpenEvent:
	    case frmUpdateEvent:
		FrmDrawForm (f);
		DrawDetails (d);
		continue;
	    case ctlSelectEvent:
		switch (event.data.ctlSelect.controlID) {
		    case DetailsOKButton:
			goto exit;
		    case DetailsDeleteButton:
			if (FrmCustomAlert (ConfirmationAlert,
					    "Are you sure you want "
					    "to delete ",
					    d->nameStr,
					    " from your list?") == 0)
			    f_DevListDelete (g, d);
			goto exit;
		    case DetailsEncryptLinkCheckbox:
			d->encryptLink = event.data.ctlSelect.on;
			f_DevListUpdate (g, d);
			continue;
		    default:
			break;
		}
	    default:
		break;
	}
    }

exit:
    FrmEraseForm (f);
    FrmRestoreActiveState (&savedstate);
    FrmDeleteForm (f);
    EvtFlushPenQueue();
    HardKeyEnable (prev);
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

