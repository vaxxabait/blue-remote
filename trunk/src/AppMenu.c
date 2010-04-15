/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: AppMenu.c
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
