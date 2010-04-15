/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Keyboard.c
 *
 ***********************************************************************/

#define LOGLEVEL LogTrace

#include "Common.h"

#include "BlueRemote_res.h"
#include "Main.h"
#include "AppMenu.h"
#include "UITools.h"
#include "Hash.h"
#include "Commands.h"
#include "KeyboardForm.h"
#include "Pref.h"
#include "HIDProtocol.h"
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

/* Describes a selectable user interface element.  */
typedef struct Button {
    /* The screen that this command appears on.  */
    Screen screen;

    /* The object ID.  */
    UInt16 objectID;

    /* The command to be executed when the user selects the object.  */
    Command cmd;
} Button;

typedef struct {
    /* The keycode corresponding to the modifier.  */
    UInt16 keycode;

    /* The state of the modifier.  0 if unlocked, 1 if locked.
       Selected state is tracked in the control's value.  */
    UInt16 state;

    /* Screen.  */
    Screen screen;

    /* Object ID of the button corresponding to the modifier.  */
    UInt16 objectID;

    /* True if the button is a graphic button.  */
    Boolean graphic;

    /* Normal label.  */
    char *label;

    /* Locked label.  */
    char *lockedLabel;

    /* Normal bitmap.  */
    UInt16 bitmap;

    /* Locked bitmap.  */
    UInt16 lockedBitmap;
} Modifier;

typedef struct {
    /* The keycode corresponding to the LED.  */
    UInt16 keycode;

    /* Screen.  */
    Screen screen;

    /* Object ID of the button corresponding to the LED.  */
    UInt16 objectID;
} Led;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static Button *FindButtonByObjectID (UInt16 objectID)
    KEYBOARDFORM_SECTION;

static void ShowScreen (Screen scr)
    KEYBOARDFORM_SECTION;

static void HideScreen (Screen scr)
    KEYBOARDFORM_SECTION;

static void SwitchScreen (Screen to, Boolean display)
    KEYBOARDFORM_SECTION;

static void UpdateModifierState (Boolean force)
    KEYBOARDFORM_SECTION;

static void UpdateLedState (Boolean force)
    KEYBOARDFORM_SECTION;

static void KbdSwitchScreens (void)
    KEYBOARDFORM_SECTION;

static void KbdSelectAlphaScreenCommand (Command cmd)
    KEYBOARDFORM_SECTION;

static void KbdSelectNumericScreenCommand (Command cmd)
    KEYBOARDFORM_SECTION;

static void KbdSelectCursorScreenCommand (Command cmd)
    KEYBOARDFORM_SECTION;

static void KbdSelectFunctionScreenCommand (Command cmd)
    KEYBOARDFORM_SECTION;

static Boolean HandleButtonSelectEvent (EventType *pEvent)
    KEYBOARDFORM_SECTION;

static void KeyboardInitForm (FormDesc *pFormDesc, FormType *pForm)
    KEYBOARDFORM_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

static FormDesc keyboardFormDesc = {
    NULL,
    KeyboardForm,
    1,
    0,
    KeyboardInitForm
};

/* A NULL terminated list of commands.  */
static Button buttons[] = {
    /* Screen selection buttons.  */
    { AllScreens, KeyboardAlphaPushButton, CmdKbdSelectAlphaScreen },
    { AllScreens, KeyboardNumericPushButton, CmdKbdSelectNumericScreen },
    { AllScreens, KeyboardCursorPushButton, CmdKbdSelectCursorScreen },
    { AllScreens, KeyboardFunctionPushButton, CmdKbdSelectFunctionScreen },

    /* Frequently used buttons.  */
    { AllScreens, KeyboardReturnButton, GetKeyClickCmd (KeyCodeReturn) },
    { AllScreens, KeyboardBackspaceButton, GetKeyClickCmd (KeyCodeBackspace) },
    { AllScreens, KeyboardEscapeButton, GetKeyClickCmd (KeyCodeEscape) },
    { AllScreens, KeyboardTabButton, GetKeyClickCmd (KeyCodeTab) },

    /* Modifiers.  */
    { AllScreens, KeyboardControlPushButton, GetKeyClickCmd (KeyCodeLeftControl) },
    { AllScreens, KeyboardShiftPushButton, GetKeyClickCmd (KeyCodeLeftShift) },
    { AllScreens, KeyboardAltPushButton, GetKeyClickCmd (KeyCodeLeftAlt) },
    { AllScreens, KeyboardWinPushButton, GetKeyClickCmd (KeyCodeLeftGUI) },

    /* Buttons on the Alpha screen.  */
    { AlphaScreen, KeyboardBacktickButton, GetKeyClickCmd (KeyCodeBacktick) },
    { AlphaScreen, KeyboardBackslashButton, GetKeyClickCmd (KeyCodeBackslash) },
    { AlphaScreen, KeyboardOneButton, GetKeyClickCmd (KeyCode1) },
    { AlphaScreen, KeyboardTwoButton, GetKeyClickCmd (KeyCode2) },
    { AlphaScreen, KeyboardThreeButton, GetKeyClickCmd (KeyCode3) },
    { AlphaScreen, KeyboardFourButton, GetKeyClickCmd (KeyCode4) },
    { AlphaScreen, KeyboardFiveButton, GetKeyClickCmd (KeyCode5) },
    { AlphaScreen, KeyboardSixButton, GetKeyClickCmd (KeyCode6) },
    { AlphaScreen, KeyboardSevenButton, GetKeyClickCmd (KeyCode7) },
    { AlphaScreen, KeyboardEightButton, GetKeyClickCmd (KeyCode8) },
    { AlphaScreen, KeyboardNineButton, GetKeyClickCmd (KeyCode9) },
    { AlphaScreen, KeyboardZeroButton, GetKeyClickCmd (KeyCode0) },
    { AlphaScreen, KeyboardDashButton, GetKeyClickCmd (KeyCodeDash) },
    { AlphaScreen, KeyboardEqualsButton, GetKeyClickCmd (KeyCodeEquals) },

    { AlphaScreen, KeyboardQButton, GetKeyClickCmd (KeyCodeQ) },
    { AlphaScreen, KeyboardWButton, GetKeyClickCmd (KeyCodeW) },
    { AlphaScreen, KeyboardEButton, GetKeyClickCmd (KeyCodeE) },
    { AlphaScreen, KeyboardRButton, GetKeyClickCmd (KeyCodeR) },
    { AlphaScreen, KeyboardTButton, GetKeyClickCmd (KeyCodeT) },
    { AlphaScreen, KeyboardYButton, GetKeyClickCmd (KeyCodeY) },
    { AlphaScreen, KeyboardUButton, GetKeyClickCmd (KeyCodeU) },
    { AlphaScreen, KeyboardIButton, GetKeyClickCmd (KeyCodeI) },
    { AlphaScreen, KeyboardOButton, GetKeyClickCmd (KeyCodeO) },
    { AlphaScreen, KeyboardPButton, GetKeyClickCmd (KeyCodeP) },
    { AlphaScreen, KeyboardOpenBracketButton, GetKeyClickCmd (KeyCodeOpenBracket) },
    { AlphaScreen, KeyboardCloseBracketButton, GetKeyClickCmd (KeyCodeCloseBracket) },

    { AlphaScreen, KeyboardAButton, GetKeyClickCmd (KeyCodeA) },
    { AlphaScreen, KeyboardSButton, GetKeyClickCmd (KeyCodeS) },
    { AlphaScreen, KeyboardDButton, GetKeyClickCmd (KeyCodeD) },
    { AlphaScreen, KeyboardFButton, GetKeyClickCmd (KeyCodeF) },
    { AlphaScreen, KeyboardGButton, GetKeyClickCmd (KeyCodeG) },
    { AlphaScreen, KeyboardHButton, GetKeyClickCmd (KeyCodeH) },
    { AlphaScreen, KeyboardJButton, GetKeyClickCmd (KeyCodeJ) },
    { AlphaScreen, KeyboardKButton, GetKeyClickCmd (KeyCodeK) },
    { AlphaScreen, KeyboardLButton, GetKeyClickCmd (KeyCodeL) },
    { AlphaScreen, KeyboardSemiColonButton, GetKeyClickCmd (KeyCodeSemiColon) },
    { AlphaScreen, KeyboardSingleQuoteButton, GetKeyClickCmd (KeyCodeSingleQuote) },

    { AlphaScreen, KeyboardZButton, GetKeyClickCmd (KeyCodeZ) },
    { AlphaScreen, KeyboardXButton, GetKeyClickCmd (KeyCodeX) },
    { AlphaScreen, KeyboardCButton, GetKeyClickCmd (KeyCodeC) },
    { AlphaScreen, KeyboardVButton, GetKeyClickCmd (KeyCodeV) },
    { AlphaScreen, KeyboardBButton, GetKeyClickCmd (KeyCodeB) },
    { AlphaScreen, KeyboardNButton, GetKeyClickCmd (KeyCodeN) },
    { AlphaScreen, KeyboardMButton, GetKeyClickCmd (KeyCodeM) },
    { AlphaScreen, KeyboardCommaButton, GetKeyClickCmd (KeyCodeComma) },
    { AlphaScreen, KeyboardPeriodButton, GetKeyClickCmd (KeyCodePeriod) },
    { AlphaScreen, KeyboardSlashButton, GetKeyClickCmd (KeyCodeSlash) },

    { AlphaScreen, KeyboardAlphaSpaceButton, GetKeyClickCmd (KeyCodeSpace) },
    { AlphaScreen, KeyboardAlphaLeftAltPushButton, GetKeyClickCmd (KeyCodeLeftAlt) },
    { AlphaScreen, KeyboardAlphaRightAltPushButton, GetKeyClickCmd (KeyCodeRightAlt) },



    /* Buttons on the Numeric screen.  */
    { NumericScreen, KeyboardNumericZeroButton, GetKeyClickCmd (KeyCodeKeypad0) },
    { NumericScreen, KeyboardNumericOneButton, GetKeyClickCmd (KeyCodeKeypad1) },
    { NumericScreen, KeyboardNumericTwoButton, GetKeyClickCmd (KeyCodeKeypad2) },
    { NumericScreen, KeyboardNumericThreeButton, GetKeyClickCmd (KeyCodeKeypad3) },
    { NumericScreen, KeyboardNumericFourButton, GetKeyClickCmd (KeyCodeKeypad4) },
    { NumericScreen, KeyboardNumericFiveButton, GetKeyClickCmd (KeyCodeKeypad5) },
    { NumericScreen, KeyboardNumericSixButton, GetKeyClickCmd (KeyCodeKeypad6) },
    { NumericScreen, KeyboardNumericSevenButton, GetKeyClickCmd (KeyCodeKeypad7) },
    { NumericScreen, KeyboardNumericEightButton, GetKeyClickCmd (KeyCodeKeypad8) },
    { NumericScreen, KeyboardNumericNineButton, GetKeyClickCmd (KeyCodeKeypad9) },
    { NumericScreen, KeyboardNumLockPushButton, GetKeyClickCmd (KeyCodeKeypadNumLock) },
    { NumericScreen, KeyboardNumericSlashButton, GetKeyClickCmd (KeyCodeKeypadSlash) },
    { NumericScreen, KeyboardNumericAsteriskButton, GetKeyClickCmd (KeyCodeKeypadAsterisk) },
    { NumericScreen, KeyboardNumericDashButton, GetKeyClickCmd (KeyCodeKeypadDash) },
    { NumericScreen, KeyboardNumericPlusButton, GetKeyClickCmd (KeyCodeKeypadPlus) },
    { NumericScreen, KeyboardNumericPeriodButton, GetKeyClickCmd (KeyCodeKeypadPeriod) },
    { NumericScreen, KeyboardNumericEnterButton, GetKeyClickCmd (KeyCodeKeypadEnter) },

    /* Buttons on the Cursor screen.  */
    { CursorScreen, KeyboardUpArrowRepeatButton, GetKeyClickCmd (KeyCodeUpArrow) },
    { CursorScreen, KeyboardDownArrowRepeatButton, GetKeyClickCmd (KeyCodeDownArrow) },
    { CursorScreen, KeyboardLeftArrowRepeatButton, GetKeyClickCmd (KeyCodeLeftArrow) },
    { CursorScreen, KeyboardRightArrowRepeatButton, GetKeyClickCmd (KeyCodeRightArrow) },
    { CursorScreen, KeyboardPageUpRepeatButton, GetKeyClickCmd (KeyCodePageUp) },
    { CursorScreen, KeyboardPageDownRepeatButton, GetKeyClickCmd (KeyCodePageDown) },
    { CursorScreen, KeyboardHomeRepeatButton, GetKeyClickCmd (KeyCodeHome) },
    { CursorScreen, KeyboardEndRepeatButton, GetKeyClickCmd (KeyCodeEnd) },

    /* Buttons on the Function screen.  */
    { FunctionScreen, KeyboardF1Button, GetKeyClickCmd (KeyCodeF1) },
    { FunctionScreen, KeyboardF2Button, GetKeyClickCmd (KeyCodeF2) },
    { FunctionScreen, KeyboardF3Button, GetKeyClickCmd (KeyCodeF3) },
    { FunctionScreen, KeyboardF4Button, GetKeyClickCmd (KeyCodeF4) },
    { FunctionScreen, KeyboardF5Button, GetKeyClickCmd (KeyCodeF5) },
    { FunctionScreen, KeyboardF6Button, GetKeyClickCmd (KeyCodeF6) },
    { FunctionScreen, KeyboardF7Button, GetKeyClickCmd (KeyCodeF7) },
    { FunctionScreen, KeyboardF8Button, GetKeyClickCmd (KeyCodeF8) },
    { FunctionScreen, KeyboardF9Button, GetKeyClickCmd (KeyCodeF9) },
    { FunctionScreen, KeyboardF10Button, GetKeyClickCmd (KeyCodeF10) },
    { FunctionScreen, KeyboardF11Button, GetKeyClickCmd (KeyCodeF11) },
    { FunctionScreen, KeyboardF12Button, GetKeyClickCmd (KeyCodeF12) },

    { FunctionScreen, KeyboardFunctionNumLockPushButton, GetKeyClickCmd (KeyCodeKeypadNumLock) },
    { FunctionScreen, KeyboardCapsLockPushButton, GetKeyClickCmd (KeyCodeCapsLock) },
    { FunctionScreen, KeyboardScrollLockPushButton, GetKeyClickCmd (KeyCodeScrollLock) },

    { FunctionScreen, KeyboardPrintScreenButton, GetKeyClickCmd (KeyCodePrintScreen) },
    { FunctionScreen, KeyboardTaskButton, GetKeyClickCmd (KeyCodeApplication) },
    { FunctionScreen, KeyboardPauseButton, GetKeyClickCmd (KeyCodePause) },

    { FunctionScreen, KeyboardInsertButton, GetKeyClickCmd (KeyCodeInsert) },
    { FunctionScreen, KeyboardHomeButton, GetKeyClickCmd (KeyCodeHome) },
    { FunctionScreen, KeyboardPageUpButton, GetKeyClickCmd (KeyCodePageUp) },

    { FunctionScreen, KeyboardDeleteButton, GetKeyClickCmd (KeyCodeDelete) },
    { FunctionScreen, KeyboardEndButton, GetKeyClickCmd (KeyCodeEnd) },
    { FunctionScreen, KeyboardPageDownButton, GetKeyClickCmd (KeyCodePageDown) },

    /* Guard.  */
    { InvalidScreen, 0, CmdNull },
};

static Hash buttonsByObjectID = 0;

/* Zero-terminated list of modifiers.  */
static Modifier modifiers[] = {
    /* Modifier buttons on all screens.  */
    { KeyCodeLeftControl, 0,
      AllScreens, KeyboardControlPushButton,
      false, "Control", "CtrlLk", 0, 0 },
    { KeyCodeLeftShift, 0,
      AllScreens, KeyboardShiftPushButton,
      false, "Shift", "ShftLk", 0, 0 },
    { KeyCodeLeftAlt, 0,
      AllScreens, KeyboardAltPushButton,
      false, "Alt", "AltLk", 0, 0 },
    { KeyCodeLeftGUI, 0,
      AllScreens, KeyboardWinPushButton,
      true, NULL, NULL, WinBitmap, WinLkBitmap },

    /* Modifiers without a button.  */
    { KeyCodeRightControl, 0,
      NoScreen, 0,
      false, "Control", "CtrlLk", 0, 0 },
    { KeyCodeRightShift, 0,
      NoScreen, 0,
      false, "Shift", "ShftLk", 0, 0 },
    { KeyCodeRightAlt, 0,
      NoScreen, 0,
      false, "Alt", "AltLk", 0, 0 },
    { KeyCodeRightGUI, 0,
      NoScreen, 0,
      true, NULL, NULL, WinBitmap, WinLkBitmap },

    /* Extra buttons on specific screens.  */
    { KeyCodeLeftAlt, 0,
      AlphaScreen, KeyboardAlphaLeftAltPushButton,
      false, "Alt", "ALk", 0, 0 },
    { KeyCodeRightAlt, 0,
      AlphaScreen, KeyboardAlphaRightAltPushButton,
      false, "Alt", "ALk", 0, 0 },

    { 0, 0, InvalidScreen, 0, false, NULL, NULL, 0, 0 }
};

/* Zero-terminated list of LEDs.  */
static Led leds[] = {
    { KeyCodeCapsLock, FunctionScreen, KeyboardCapsLockPushButton },
    { KeyCodeKeypadNumLock, NumericScreen, KeyboardNumLockPushButton },
    { KeyCodeKeypadNumLock, FunctionScreen, KeyboardFunctionNumLockPushButton },
    { KeyCodeScrollLock, FunctionScreen, KeyboardScrollLockPushButton },
    { 0, 0, 0 }
};

/* A rectangle covering all subscreens.
 * Used for speeding up hiding the current screen.  */
#if QUICK_HIDE_SCREEN
static RectangleType screenRectangle = { { 0, 29 }, { 160, 101 } };
#endif

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static Button *
FindButtonByObjectID (UInt16 objectID)
{
#if 0
    Button *b;
    for (b = buttons; b->screen != InvalidScreen; b++) {
        if (b->objectID == objectID)
            return b;
    }
    return NULL;
#else
    return HashGet (buttonsByObjectID, objectID);
#endif
}


/* Show SCR on the display and enable its controls.  */
static void
ShowScreen (Screen scr)
{
    Button *b;
    FormType *pForm = FrmGetActiveForm();

    TRACE ("ShowScreen %d", scr);
    
    if (FrmGetActiveFormID() != KeyboardForm)
	return;

    for (b = buttons; b->screen != InvalidScreen; b++) {
        UInt16 idx;
        if (b->screen != scr)
            continue;

        idx = FrmGetObjectIndex (pForm, b->objectID);

        if (idx != frmInvalidObjectId) {
	    FrmShowObject (pForm, idx);
        }
    }
}

/* Erase SCR from the display and disable its controls.  */
static void
HideScreen (Screen scr)
{
    Button *b;
    FormType *pForm = FrmGetActiveForm();

    TRACE ("HideScreen %d", scr);

    if (FrmGetActiveFormID() != KeyboardForm)
	return;

    for (b = buttons; b->screen != InvalidScreen; b++) {
        UInt16 idx;
        if (b->screen != scr)
            continue;

        idx = FrmGetObjectIndex (pForm, b->objectID);

        if (idx != frmInvalidObjectId) {
	    FrmHideObject (pForm, idx);
        }
    }
}

/* Switch to another screen.  */
static void
SwitchScreen (Screen to, Boolean display)
{
    FormType *pForm = FrmGetActiveForm();
    UInt16 button = 0;

    TRACE ("SwitchScreen %d (%d)", to, display);
    
    if (FrmGetActiveFormID () != KeyboardForm)
        display = false;

    switch (to) {
        case NumericScreen:
	    button = KeyboardNumericPushButton;
            break;

	case CursorScreen:
            button = KeyboardCursorPushButton;
            break;

	case FunctionScreen:
	    button = KeyboardFunctionPushButton;
            break;

	default:
        case NoScreen:
            to = AlphaScreen;
            /* Fallthrough */
        case AlphaScreen:
	    button = KeyboardAlphaPushButton;
            break;
    }

    ASSERT (button);
    if (display &&
	FrmGetObjectId (pForm, FrmGetControlGroupSelection (pForm, 20))
	!= button)
	FrmSetControlGroupSelection (pForm, 20, button);

    if (display && UPREF (currentScreen) != NoScreen)
        HideScreen (UPREF (currentScreen));    
    UPREF (currentScreen) = to;
    if (display && to != NoScreen) {
        ShowScreen (UPREF (currentScreen));
	UpdateLedState (true);
    }
}



static void
UpdateModifierState (Boolean force)
{
    FormType *pForm = FrmGetActiveForm();
    UInt16 idx;
    ControlType *ctl = NULL;
    UInt16 mods = HIDPModifiers();
    int i;

    TRACE ("UpdateModifierState");
    for (i = 0; modifiers[i].screen != InvalidScreen; i++) {
        UInt16 bit = MODIFIER_KEYCODE_TO_BIT (modifiers[i].keycode);
        ModState state = HIDPModifierState (modifiers[i].keycode);
        UInt8 enabled = ((mods & bit) ? 1 : 0);

        TRACE ("i=%d, bit=%x, state=%s", i, bit, HIDPModStateStr (state));

        if (modifiers[i].screen == AllScreens
            || modifiers[i].screen == UPREF (currentScreen)) {
            idx = FrmGetObjectIndex (pForm, modifiers[i].objectID);
            if (idx == frmInvalidObjectId) {
                LOG ("Invalid modifier #%d", i);
                continue;
            }
            ctl = FrmGetObjectPtr (pForm, idx);

            switch (state) {
		case ModReleased:
		case ModPressed:
		case ModSticky:
		case ModStickyPressed:
		default:
		    if (force || modifiers[i].state) {
			if (modifiers[i].graphic)
			    CtlSetGraphics (ctl, modifiers[i].bitmap,
					    modifiers[i].bitmap);
			else
			    CtlSetLabel (ctl, modifiers[i].label);
		    }
		    modifiers[i].state = 0;
		    break;

		case ModLocked:
		case ModLockedPressed:
		    if (force || !modifiers[i].state) {
			if (modifiers[i].graphic)
			    CtlSetGraphics (ctl,
					    modifiers[i].lockedBitmap,
					    modifiers[i].lockedBitmap);
			else
			    CtlSetLabel (ctl, modifiers[i].lockedLabel);
		    }
		    modifiers[i].state = 1;
		    break;
            }

            if (force || FrmGetControlValue (pForm, idx) != enabled)
                FrmSetControlValue (pForm, idx, enabled);
        }
    }
}



static void
UpdateLedState (Boolean force)
{
    FormType *pForm = FrmGetActiveForm();
    UInt16 idx;
    UInt16 state = HIDPLeds();
    int i;

    TRACE ("UpdateLedState");
    for (i = 0; leds[i].keycode != 0; i++) {
        UInt16 bit = LED_KEYCODE_TO_BIT (leds[i].keycode);
        UInt16 enabled = (state & bit ? 1 : 0);

        if (leds[i].screen == AllScreens
            || leds[i].screen == UPREF (currentScreen)) {
            idx = FrmGetObjectIndex (pForm, leds[i].objectID);
            if (idx == frmInvalidObjectId) {
                LOG ("Invalid led object #%d", i);
                continue;
            }
            if (force || FrmGetControlValue (pForm, idx) != enabled)
                FrmSetControlValue (pForm, idx, enabled);
        }
    }
}



static Boolean
HandleButtonSelectEvent (EventType *pEvent)
{
    Button *b;

    // TODO: Why does this happen?
    // It happens when I switch forms using the popup
    if (g_currentForm != &keyboardFormDesc)
        return false;

    PANICIF (g_currentForm != &keyboardFormDesc,
             "Unexpected handler call");

    if (pEvent->eType != ctlSelectEvent
	&& pEvent->eType != ctlRepeatEvent)
        return false;

    DEBUG ("Button %d on screen %d tapped",
           pEvent->data.ctlSelect.controlID,
           UPREF (currentScreen));

    b = FindButtonByObjectID (pEvent->data.ctlSelect.controlID);
    if (b == NULL)
        return false;

    ENQUEUE (b->cmd);

    /* Don't handle ctlRepeatEvents: it breaks the repeating button's
     * built-in behavior.  */
    if (pEvent->eType == ctlRepeatEvent)
        return false;

    return true;
}

static void
KbdSwitchScreens (void)
{
    Screen to = AlphaScreen;
    TRACE ("KbdSwitchScreens");
    switch (UPREF (currentScreen)) {
	case AlphaScreen:
	    to = NumericScreen;
	    break;
	case NumericScreen:
	    to = CursorScreen;
	    break;
	case CursorScreen:
	    to = FunctionScreen;
	    break;
	case FunctionScreen:
	default:
	    to = AlphaScreen;
	    break;
    }
    SwitchScreen (to, true);
}

static void
KbdSelectAlphaScreenCommand (Command cmd)
{
    SwitchScreen (AlphaScreen, true);
}

static void
KbdSelectNumericScreenCommand (Command cmd)
{
    SwitchScreen (NumericScreen, true);
}

static void
KbdSelectCursorScreenCommand (Command cmd)
{
    SwitchScreen (CursorScreen, true);
}

static void
KbdSelectFunctionScreenCommand (Command cmd)
{
    SwitchScreen (FunctionScreen, true);
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the
 *              "*KeyboardForm" of this application.
 *
 * PARAMETERS:  pEvent  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean
KeyboardFormHandleEvent (EventType* e)
{
    FormType* f = FrmGetActiveForm();
    Boolean handled = false;

    TRACE ("KeyboardFormHandleEvent");
    
    /* Handle command buttons.  */
    handled = HandleButtonSelectEvent (e);
    if (handled)
        return true;

    /* Handle switching between forms by the popup list.  */
    handled = HandleFormPopSelectEvent (e);
    if (handled)
        return true;

    switch (e->eType) {

	case menuOpenEvent:
	    MenuHideItem (CommonConnectionAddDevice);
	    MenuHideItem (CommonConnectionConnect);
	    MenuShowItem (CommonConnectionDisconnect);
	    return false;

	case menuEvent:
	    return AppMenuDoCommand (e->data.menu.itemID);

	case frmOpenEvent:
	case frmUpdateEvent:
	{
	    ListType *list;
	    TRACE ("frmOpenEvent/frmUpdateEvent");

	    /* Preselect the Keyboard entry in the popup list.  */
	    list = FrmGetObjectPtr
		(f, FrmGetObjectIndex (f, KeyboardPopupList));
	    LstSetSelection (list, 1);

	    /* Set the shift and LED state.  */
	    UpdateModifierState (true);
	    UpdateLedState (true);

	    /* Show the correct buttons.  */
	    SwitchScreen (UPREF (currentScreen), true);
	    FrmDrawForm (f);
	    FormDrawn (KeyboardForm);
	    HardKeyEnable (true);
	    return true;	    
	}
	case winExitEvent:
	case winEnterEvent:
	    TRACE ("winEnterEvent/winExitEvent");
	    if (e->data.winExit.exitWindow == (WindowType *) f)
		HardKeyEnable (false);
	    else if (e->data.winEnter.enterWindow == (WindowType *) f)
		HardKeyEnable (true);
	    return false;

	case commandEvent:
	    switch (EVENT_COMMAND (e)) {
		case CmdModifiersChanged:
		    TRACE ("Modifiers changed");
		    UpdateModifierState (false);
		    return true;
		case CmdLedsChanged:
		    TRACE ("LEDs changed");
		    UpdateLedState (false);
		    return true;
		case CmdKeyboardForm:
		case CmdSwitchScreens:
		    TRACE ("Switching screens");
		    KbdSwitchScreens();
		    return true;
		default:
		    return false;
	    }
	default:
	    return false;
    }

}

static void
KeyboardInitForm (FormDesc *pFormDesc, FormType *pForm)
{
    TRACE ("KeyboardInitForm");

    /* Set the event handler.  */
    FrmSetEventHandler (pForm, KeyboardFormHandleEvent);

    UPREF (currentRemoteFormID) = KeyboardForm;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
KeyboardInit (void)
{
    int i;
    RegisterForm (&keyboardFormDesc);

    buttonsByObjectID = HashCreate (100);
    for (i = 0; buttons[i].screen != InvalidScreen; i++) {
        if (buttons[i].objectID != 0) {
            void *old = HashPut (buttonsByObjectID,
                                 buttons[i].objectID,
                                 &buttons[i]);
            PANICIF (old != NULL, "Duplicate button");
        }
    }


    CmdRegisterHandler (CmdKbdSelectAlphaScreen,
			KbdSelectAlphaScreenCommand);
    CmdRegisterHandler (CmdKbdSelectNumericScreen,
			KbdSelectNumericScreenCommand);
    CmdRegisterHandler (CmdKbdSelectCursorScreen,
			KbdSelectCursorScreenCommand);
    CmdRegisterHandler (CmdKbdSelectFunctionScreen,
			KbdSelectFunctionScreenCommand);

}

void
KeyboardClose (void)
{
    if (buttonsByObjectID != 0)
        HashDelete (buttonsByObjectID);
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

