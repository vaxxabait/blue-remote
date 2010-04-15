/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: About.c
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


/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

