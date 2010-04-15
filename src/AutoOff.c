/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: AutoOff.c
 *
 ***********************************************************************/

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "AutoOff.h"
#undef PRIVATE_INCLUDE

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct AutoOffGlobals {
    Boolean enabled;
} AutoOffGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static Err AutoOffCallback (SysNotifyParamType *p)
    AUTOOFF_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_enabled	g->autooff->enabled

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static Err
AutoOffCallback (SysNotifyParamType *p) 
{
    Globals *g = (Globals *) p->userDataP;
    SleepEventParamType *sleep = (SleepEventParamType *) p->notifyDetailsP;
    
    switch (sleep->reason) {
    case sysSleepAutoOff:
	if (!g_enabled)
	    sleep->deferSleep++;
	break;
    case sysSleepPowerButton:
    case sysSleepResumed:
    case sysSleepUnknown:
    default:
	break;
    }
    return 0;
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
f_AutoOffInit (Globals *g) 
{
    Err err;
    UInt16 cardNo;
    LocalID dbID;

    if (g->autooff)
	f_AutoOffClose (g);
    g->autooff = CheckedMemPtrNew (sizeof (AutoOffGlobals), true);
    g_enabled = true;

    SysCurAppDatabase (&cardNo, &dbID);
    err = SysNotifyRegister (cardNo, dbID, sysNotifySleepRequestEvent,
			     AutoOffCallback, sysNotifyNormalPriority,
			     g);
    if (err && err != sysNotifyErrDuplicateEntry)
	LOG ("Could not register AutoOff callback: %s", StrError (err));
    DEBUG ("AutoOff initialized");
}

void
f_AutoOffClose (Globals *g)
{
    Err err;
    UInt16 cardNo;
    LocalID dbID;

    if (g->autooff) {
	MemPtrFree (g->autooff);
	g->autooff = 0;
    }

    SysCurAppDatabase (&cardNo, &dbID);
    err = SysNotifyUnregister (cardNo, dbID, sysNotifySleepRequestEvent,
			       sysNotifyNormalPriority);
    if (err && err != sysNotifyErrEntryNotFound)
	LOG ("Could not unregister AutoOff callback: %s", StrError (err));

    EvtSetAutoOffTimer (ResetTimer, 60);
    EvtResetAutoOffTimer();
    DEBUG ("AutoOff closed");
}


Boolean
f_AutoOffEnable (Globals *g, Boolean enable) 
{
    Boolean old = g_enabled;
    DEBUG ("AutoOff %s", (enable ? "enabled" : "disabled"));
    g_enabled = enable;
    if (enable)
	EvtSetAutoOffTimer (ResetTimer, 60);
    else
	EvtSetAutoOffTimer (SetAtLeast, 65535);
    EvtResetAutoOffTimer();
    return old;
}


/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

