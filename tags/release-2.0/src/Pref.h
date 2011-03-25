/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Pref.h
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

#undef SECTION
#define SECTION APPPREF_SECTION

#ifdef NORMAL_INCLUDE

/* Application saved preference ID and version number.  */
#define prefSavedID             0x00
#define prefSavedVersionNum     0x01

/* Application unsaved preference ID and version number.  */
#define prefUnsavedID           0x00
#define prefUnsavedVersionNum   0x01

typedef struct SavedPreferences {
    /* Version number, set to prefSavedVersionNum.  */
    Int16 version;

    /* True if we should automatically reconnect to the selected
     * device on startup.  */
    Boolean s_autoConnect;
#define SPREF_DEFAULT_AUTOCONNECT true
    
    Boolean s_hardkeyOverride;
#define SPREF_DEFAULT_HARDKEY_OVERRIDE true
    
    Boolean s_disableAutoOff;
#define SPREF_DEFAULT_DISABLE_AUTO_OFF true

    Boolean s_masquarade;
#define SPREF_DEFAULT_MASQUARADE true
    
    BTDeviceClassType s_deviceSubclass;
#define SPREF_DEFAULT_DEVICE_SUBCLASS classKeyboard

    /* The index of the currently selected device in the DevList.  */
    UInt16 s_device;
#define SPREF_DEFAULT_DEVICE 0

} SavedPreferences;

typedef struct UnsavedPreferences {
    /* Version number, set to prefUnsavedVersionNum.  */
    Int16 version;

    /* The object ID of the current remote control form ID.  */
    UInt16 u_currentRemoteFormID;
#define UPREF_DEFAULT_CURRENT_REMOTE_FORMID KeyboardForm

    /* The screen that is currently selected on the Keyboard form.  */
    Int16 u_currentScreen;
#define UPREF_DEFAULT_CURRENT_SCREEN AlphaScreen
    
} UnsavedPreferences;

#define SPREF(name) g->savedPref->s_##name
#define UPREF(name) g->unsavedPref->u_##name

void f_PrefInit (Globals *g);

#endif  /* GLOBALS_INCLUDE */

DEFUN0 (void, PrefClose);
DEFUN0 (void, PrefSave);
DEFUN0 (void, PrefShowSetupForm);
