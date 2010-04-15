/**************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: AppMenu.c
 *
 *************************************************************************/

#include "Common.h"

#include "AppMenu.h"
#include "BlueRemote_res.h"
#include "Main.h"
#include "Bluetooth.h"
#include "Pref.h"
#include "Commands.h"
#include "HIDProtocol.h"
#include "AboutForm.h"

#define g globals

/* Perform menu command COMMAND.  */
Boolean
AppMenuDoCommand (UInt16 command)
{
    Boolean handled = false;
    FormDesc *fd;

    /* See if menu command is a form switch.  */
    for (fd = g_formList; fd; fd = fd->next) {
        if (fd->menuCommandID == command) {
            SwitchToForm (fd->formID);
            return true;
        }
    }

    /* Handle the rest of the items.  */
    switch (command) {
	case CommonOptionsPreferences:
	    PrefShowSetupForm();
	    return true;
	    
	case CommonOptionsRegister:
	    DRMRegisterForm();
	    return true;

	case CommonOptionsAbout:
	    AboutShow();
	    return true;

#if 0
	case CommonOptionsSwitchclass:
	    SPREF (bluetoothClass) = (SPREF (bluetoothClass) + 1) % 5;
	    SPREF (bluetoothSDP) = (SPREF (bluetoothSDP) + 1) % 5;
	    return true;
#endif

	case CommonConnectionAddDevice:
	    BTSelectDevice();
	    return true;

	case CommonConnectionConnect:
	    ENQUEUE (CmdInitiateConnect);
	    return true;

	case CommonConnectionDisconnect:
	    ENQUEUE (CmdInitiateDisconnect);
	    return true;

	default:
	    break;
    }

    return handled;
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */
