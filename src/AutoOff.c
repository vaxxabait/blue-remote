/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: AutoOff.c
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
