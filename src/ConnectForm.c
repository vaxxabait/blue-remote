/**************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: ConnectForm.c
 *
 *************************************************************************/

#include "Common.h"

#include <FrmGlue.h>

#include "BlueRemote_res.h"
#include "AppMenu.h"
#include "ConnectForm.h"
#include "HardKeys.h"

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

static void DrawListEntry (Int16 itemNum,
                           RectangleType *bounds, Char **itemsText)
    CONNECTFORM_SECTION;

static void UpdateDeviceList (Boolean draw)
    CONNECTFORM_SECTION;

static Boolean ConnectFormHandleEvent (EventType* pEvent)
    CONNECTFORM_SECTION;

static void ConnectInitForm (struct FormDesc *pFormDesc, FormType *pForm)
    CONNECTFORM_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

static FormDesc connectFormDesc = {
    NULL,
    ConnectForm,
    65535,
    0,
    ConnectInitForm
};

UInt32 modnum = 0;

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static void
DrawListEntry (Int16 itemNum, RectangleType *bounds, Char **itemsText)
{
    Globals *g = (Globals *)itemsText;
    Device *d;
    if (g == 0) return;

    if (DevListLength() > 0) {
        d = DevListGet (itemNum);
        if (!d) {
            LOG ("DrawListEntry could not find item %d", itemNum);
            return;
        }
        DEBUG ("DrawListEntry %d: %s %s",
	       itemNum, d->nameStr, d->addressStr);
	
        WinDrawTruncChars (d->nameStr, StrLen (d->nameStr),
                           bounds->topLeft.x, bounds->topLeft.y,
                           bounds->extent.x);
    }
}


static void
UpdateDeviceList (Boolean draw)
{
    FormType *pForm = FrmGetActiveForm();
    UInt16 idx;
    ListType *l;
    if (!pForm)
        return;

    idx = FrmGetObjectIndex (pForm, ConnectDeviceList);
    if (idx == frmInvalidObjectId)
        return;
    l = FrmGetObjectPtr (pForm, idx);

    LstSetDrawFunction (l, DrawListEntry);
    LstSetListChoices (l, (Char **)g, DevListLength());
    LstSetTopItem (l, 0);
    if (SPREF (device) == noListSelection)
        LstSetSelection (l, noListSelection);
    else
        LstSetSelection (l, SPREF (device));
    if (draw)
        LstDrawList (l);
    modnum = DevListModNum();
}

static Boolean
ConnectFormHandleEvent (EventType* e)
{
    FormType *pForm = FrmGetActiveForm();

    switch (e->eType) {
	case keyDownEvent:
	    if (!FrmGlueNavIsSupported()
		&& IsFiveWayNavEvent (e)
		&& NavKeyPressed (e, Select)) {
		/* Select key.  */
		if (DevListSelected())
		    ENQUEUE (CmdInitiateConnect);
		else
		    SndPlaySystemSound (sndError);
		return true;
	    }
	    else if (e->data.keyDown.chr == vchrPageUp
		     || (IsFiveWayNavEvent (e)
			 && NavKeyPressed (e, Up))) {
		/* Up key.  */
		if (DevListSelected()) {
		    if (SPREF (device) > 0) {
			SPREF (device)--;
			UpdateDeviceList (true);
		    }
		    else
			SndPlaySystemSound (sndError);
		}
		else if (DevListLength() > 0) {
		    SPREF (device) = 0;
		    UpdateDeviceList (true);
		}
		return true;
	    }
	    else if (e->data.keyDown.chr == vchrPageDown
		     || (IsFiveWayNavEvent (e)
			 && NavKeyPressed (e, Down))) {
		/* Down key.  */
		if (DevListSelected()) {
		    if (SPREF (device) < DevListLength() - 1) {
			SPREF (device)++;
			UpdateDeviceList (true);
		    }
		    else
			SndPlaySystemSound (sndError);
		}
		else if (DevListLength() > 0) {
		    SPREF (device) = 0;
		    UpdateDeviceList (true);
		}
		return true;
	    }
	    return false;
	    
	case menuOpenEvent:
	    MenuShowItem (CommonConnectionAddDevice);
	    MenuShowItem (CommonConnectionConnect);
	    MenuHideItem (CommonConnectionDisconnect);
	    return false;

	case menuEvent:
	    return AppMenuDoCommand (e->data.menu.itemID);

	case frmOpenEvent:
	    UpdateDeviceList (false);
	    FrmDrawForm (pForm);
	    FormDrawn (ConnectForm);
	    HardKeyEnable (false);
	    if (DevListLength() == 0) {
		ENQUEUE (CmdShowWelcome);
	    }
	    return true;

	case lstSelectEvent:
	    DevListSetSelected
		(DevListGet (e->data.lstSelect.selection));
	    return true;

	case ctlSelectEvent:
	{
	    UInt16 id = e->data.ctlSelect.controlID;
	    switch (id) {
		case ConnectConnectButton:
		    ENQUEUE (CmdInitiateConnect);
		    return true;

		case ConnectNewButton:
		    SPREF (device) = noListSelection;
		    UpdateDeviceList (true);
		    ENQUEUE (CmdWaitConnection);
		    return true;

		case ConnectDetailsButton:
		    DevListDetails();
		    return true;

		default:
		    return false;
	    }
	}

	case commandEvent:
	{
	    Command cmd = EVENT_COMMAND (e);
	    switch (cmd) {

		case CmdDeviceListChanged:
		    if (modnum != DevListModNum())
			UpdateDeviceList (true);
		    return false;

		default:
		    /* Ignore.  */
		    return false;
	    }
	}

	default:
	    return false;
    }
}

static void
ConnectInitForm (struct FormDesc *pFormDesc, FormType *pForm)
{
    /* Set the event handler.  */
    FrmSetEventHandler (pForm, ConnectFormHandleEvent);
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
ConnectInit (void)
{
    RegisterForm (&connectFormDesc);
}

void
ConnectClose (void)
{
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

