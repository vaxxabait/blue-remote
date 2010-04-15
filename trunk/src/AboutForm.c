/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: About.c
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

#include <FrmGlue.h>

#include "BlueRemote_res.h"
#include "HardKeys.h"
#include "AboutForm.h"

#define g globals

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

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static void Draw (void)
    ABOUTFORM_SECTION;

static Boolean AboutFormHandleEvent (EventType *e)
    ABOUTFORM_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

static RectangleType aboutBounds = { { 0, 40 }, { 156, 95 } };


/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static void
Draw (void)
{
    char text[256];
    text[0] = 0;
    StrCat (text, "Version ");
    StrCat (text, VersionString());
    StrCat (text, "\n"
	    APP_COPYRIGHT "\n\n"
	    APP_SUPPORT "\n\n"
	    "For updates, visit:\n"
	    APP_URL);
    FrmDrawForm (FrmGetActiveForm());
    DrawText (&aboutBounds, JustifyCenter, text, true);
}

static Boolean
AboutFormHandleEvent (EventType *e) 
{
    switch (e->eType) {
    case frmOpenEvent:
    case frmUpdateEvent:
	TRACE ("frmOpenEvent/frmUpdateEvent");
	Draw();
	return true;

    default:
	return false;
    }
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
AboutShow (void)
{
    FormType *f;
    Boolean prev = HardKeyEnable (false);
    FormActiveStateType savedstate;
    EventType event;
    int pressed;
    
    FrmSaveActiveState (&savedstate);
    f = FrmInitForm (AboutForm);
    FrmSetEventHandler (f, AboutFormHandleEvent);
    FrmSetActiveForm (f);
    Draw();

    /* Set up navigation info.  */
    if (FrmGlueNavIsSupported())
	FrmSetNavState (f, kFrmNavStateFlagsObjectFocusMode);
    FrmGlueNavObjectTakeFocus (f, FrmGetObjectIndex (f, AboutOKButton));

    pressed = 0;
    do {
	EvtGetEvent (&event, evtWaitForever);
	if (SysHandleEvent (&event))
	    continue;

	if (FrmDispatchEvent (&event))
	    continue;

	switch (event.eType) {
	    case appStopEvent:
		ENQUEUE (CmdExitApp);
		pressed = -1;
		continue;
	    case ctlSelectEvent:
		switch (event.data.ctlSelect.controlID) {
		    case AboutOKButton:
			pressed = 1;
			continue;
		    default:
			break;
		}
	    default:
		break;
	}

    } while (pressed == 0);
    
    FrmEraseForm (f);
    FrmRestoreActiveState (&savedstate);
    FrmDeleteForm (f);
    EvtFlushPenQueue();
    HardKeyEnable (prev);
}
