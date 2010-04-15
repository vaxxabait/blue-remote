/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Pref.c
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
#include "Pref.h"
#undef PRIVATE_INCLUDE

#include "BlueRemote_res.h"
#include "KeyboardForm.h"

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

/* Reset saved preferences to default values.  */
static void PrefSavedReset (Globals *g)
    APPPREF_SECTION;

/* Reset unsaved preferences to default values.  */
static void PrefUnsavedReset (Globals *g)
    APPPREF_SECTION;

static Boolean PreferencesFormHandleEvent (EventType *e)
    APPPREF_SECTION;
    
static void UpdateAdvancedPreferencesForm (Globals *g, FormType *f)
    APPPREF_SECTION;

static Boolean AdvancedPreferencesFormHandleEvent (EventType *e)
    APPPREF_SECTION;
    
/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_unsavedPref   g->unsavedPref
#define g_savedPref     g->savedPref

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static void
PrefSavedReset (Globals *g)
{
    if (g_savedPref)
        MemPtrFree (g_savedPref);
    g_savedPref = CheckedMemPtrNew (sizeof (struct SavedPreferences),
				    true);

    g_savedPref->version = prefSavedVersionNum;

    SPREF (autoConnect) = SPREF_DEFAULT_AUTOCONNECT;
    SPREF (hardkeyOverride) = SPREF_DEFAULT_HARDKEY_OVERRIDE;
    SPREF (disableAutoOff) = SPREF_DEFAULT_DISABLE_AUTO_OFF;
    SPREF (masquarade) = SPREF_DEFAULT_MASQUARADE;
    SPREF (device) = SPREF_DEFAULT_DEVICE;
    SPREF (deviceSubclass) = SPREF_DEFAULT_DEVICE_SUBCLASS;
}

static void
PrefUnsavedReset (Globals *g)
{
    if (g_unsavedPref)
        MemPtrFree (g_unsavedPref);
    g_unsavedPref = CheckedMemPtrNew (sizeof (struct UnsavedPreferences),
                                      true);

    g_unsavedPref->version = prefUnsavedVersionNum;

    UPREF (currentRemoteFormID) = UPREF_DEFAULT_CURRENT_REMOTE_FORMID;
    UPREF (currentScreen) = UPREF_DEFAULT_CURRENT_SCREEN;
}

static Boolean
PreferencesFormHandleEvent (EventType *e) 
{
    Globals *g = globals;
    
    switch (e->eType) {
	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		case PreferencesAutoconnectCheckbox:
		    SPREF (autoConnect) = e->data.ctlSelect.on;
		    PrefSave();
		    return true;
		case PreferencesHardKeyOverrideCheckbox:
		    SPREF (hardkeyOverride) = e->data.ctlSelect.on;
		    PrefSave();
		    return true;
		case PreferencesDisableAutoOffCheckbox:
		    SPREF (disableAutoOff) = e->data.ctlSelect.on;
		    PrefSave();
		    return true;
		default:
		    return false;
	    }
	default:
	    return false;
    }
}

static void
UpdateAdvancedPreferencesForm (Globals *g, FormType *f)
{
    UInt16 id;
    UInt16 idx;

    switch (SPREF (deviceSubclass)) {
	case classKeyboard:
	    id = AdvancedPreferencesKeyboardPushButton;
	    break;
	case classMouse:
	    id = AdvancedPreferencesMousePushButton;
	    break;
	case classMouseKeyboard:
	    id = AdvancedPreferencesBothPushButton;
	    break;
	default:
	case classCombo:
	    id = AdvancedPreferencesComboPushButton;
	    break;
    }
    FrmSetControlGroupSelection (f, 1, id);
    
    idx = FrmGetObjectIndex (f, AdvancedPreferencesOverrideDeviceIDCheckbox);
    FrmSetControlValue (f, idx, SPREF (masquarade));
}


static Boolean
AdvancedPreferencesFormHandleEvent (EventType *e) 
{
    Globals *g = globals;
    
    switch (e->eType) {
	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		case AdvancedPreferencesKeyboardPushButton:
		    SPREF (deviceSubclass) = classKeyboard;
		    PrefSave();
		    return true;
		case AdvancedPreferencesMousePushButton:
		    SPREF (deviceSubclass) = classMouse;
		    PrefSave();
		    return true;
		case AdvancedPreferencesBothPushButton:
		    SPREF (deviceSubclass) = classMouseKeyboard;
		    PrefSave();
		    return true;
		case AdvancedPreferencesComboPushButton:
		    SPREF (deviceSubclass) = classCombo;
		    PrefSave();
		    return true;
		case AdvancedPreferencesOverrideDeviceIDCheckbox:
		    SPREF (masquarade) = e->data.ctlSelect.on;
		    PrefSave();
		    return true;
		case AdvancedPreferencesResetButton:
		    SPREF (deviceSubclass) = SPREF_DEFAULT_DEVICE_SUBCLASS;
		    SPREF (masquarade) = SPREF_DEFAULT_MASQUARADE;
		    PrefSave();
		    UpdateAdvancedPreferencesForm (g, FrmGetActiveForm());
		    return true;
		default:
		    return false;
	    }
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
f_PrefInit (Globals *g)
{
    UInt16 size;
    Int16 res;

    /* Get saved preferences.  */
    size = 0;
    res = PrefGetAppPreferences (appFileCreator, prefSavedID,
                                 NULL, &size, true);
    if (res == noPreferenceFound) {
        DEBUG ("Saved preferences not found, reinitializing");
        PrefSavedReset (g);
    }
    else {
        if (size < sizeof (struct SavedPreferences))
            size = sizeof (struct SavedPreferences);
        if (g_savedPref)
            MemPtrFree (g_savedPref);
        g_savedPref = CheckedMemPtrNew (size, true);
        res = PrefGetAppPreferences (appFileCreator, prefSavedID,
                                     g_savedPref, &size, true);
        if (res == noPreferenceFound) {
            DEBUG ("Saved preferences not found, reinitializing");
            PrefSavedReset (g);
        }
        else if (res != prefSavedVersionNum) {
            DEBUG ("Version %d found, %d expected",
                   res, prefSavedVersionNum);
            /* TODO */
            PrefSavedReset (g);
        }
        else {
            DEBUG ("Saved preferences loaded");
        }
    }
    PANICIF (g_savedPref == NULL, "Assertion failure");

    /* Get unsaved preferences.  */
    size = 0;
    res = PrefGetAppPreferences (appFileCreator, prefUnsavedID,
                                 NULL, &size, false);
    if (res == noPreferenceFound) {
        DEBUG ("Unsaved preferences not found, reinitializing");
        PrefUnsavedReset (g);
    }
    else {
        if (size < sizeof (struct UnsavedPreferences))
            size = sizeof (struct UnsavedPreferences);
        if (g_unsavedPref)
            MemPtrFree (g_unsavedPref);
        g_unsavedPref = CheckedMemPtrNew (size, true);
        res = PrefGetAppPreferences (appFileCreator, prefUnsavedID,
                                     g_unsavedPref, &size, false);
        if (res == noPreferenceFound) {
            DEBUG ("Unsaved preferences not found, reinitializing");
            PrefUnsavedReset (g);
        }
        else if (res != prefUnsavedVersionNum) {
            DEBUG ("Unsaved preferences version %d found, %d expected",
                   res, prefUnsavedVersionNum);
            /* TODO */
            PrefUnsavedReset (g);
        }
        else {
            DEBUG ("Unsaved preferences loaded");
        }
    }
    PANICIF (g_unsavedPref == NULL, "Assertion failure");

    /* Validate preferences.  */
    switch (UPREF (currentRemoteFormID)) {
        case KeyboardForm:
        case MouseForm:
            /* OK */
            break;
        default:
            UPREF (currentRemoteFormID) = KeyboardForm;
            break;
    }

    DEBUG ("Preferences loaded");
}

void
f_PrefSave (Globals *g)
{
    if (g_savedPref)
        PrefSetAppPreferences (appFileCreator,
                               prefSavedID,
                               prefSavedVersionNum,
                               g_savedPref,
                               sizeof (struct SavedPreferences),
                               true);
    if (g_unsavedPref)
        PrefSetAppPreferences (appFileCreator,
                               prefUnsavedID,
                               prefUnsavedVersionNum,
                               g_unsavedPref,
                               sizeof (struct UnsavedPreferences),
                               false);
    DEBUG ("Preferences saved");
}


void
f_PrefClose (Globals *g)
{
    PrefSave();

    if (g_savedPref)
        MemPtrFree (g_savedPref);
    g_savedPref = NULL;

    if (g_unsavedPref)
        MemPtrFree (g_unsavedPref);
    g_unsavedPref = NULL;

    DEBUG ("Preferences closed");
}

void
f_PrefShowSetupForm (Globals *g) 
{
    FormType *f;
    UInt16 idx;
    UInt16 btn;

    while (true) {
	f = FrmInitForm (PreferencesForm);
	FrmSetEventHandler (f, PreferencesFormHandleEvent);

	idx = FrmGetObjectIndex (f, PreferencesAutoconnectCheckbox);
	FrmSetControlValue (f, idx, SPREF (autoConnect));

	idx = FrmGetObjectIndex (f, PreferencesHardKeyOverrideCheckbox);
	FrmSetControlValue (f, idx, SPREF (hardkeyOverride));

	idx = FrmGetObjectIndex (f, PreferencesDisableAutoOffCheckbox);
	FrmSetControlValue (f, idx, SPREF (disableAutoOff));
    
	btn = FrmDoDialog (f);
	FrmDeleteForm (f);

	if (btn == PreferencesOKButton)
	    break;

	/* Go on to the Advanced Preferences form.  */
	while (true) {
	    f = FrmInitForm (AdvancedPreferencesForm);
	    FrmSetEventHandler (f, AdvancedPreferencesFormHandleEvent);

	    UpdateAdvancedPreferencesForm (g, f);

	    btn = FrmDoDialog (f);
	    FrmDeleteForm (f);

	    if (btn == AdvancedPreferencesOKButton)
		/* Return to simple Preferences form.  */
		break;
	    
	    if (btn == AdvancedPreferencesResetButton) {
		SPREF (deviceSubclass) = SPREF_DEFAULT_DEVICE_SUBCLASS;
		SPREF (masquarade) = SPREF_DEFAULT_MASQUARADE;
	    }
	}
    }
}
