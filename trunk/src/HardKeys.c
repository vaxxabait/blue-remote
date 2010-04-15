/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: HardKeys.c
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
 **************************************************************************/

#include "Common.h"

#include "HardKeys.h"
#include "Commands.h"

#include <Common/System/HsKeyCodes.h>
#include <Common/System/HsKeyCommon.h>

#define g globals

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Accessor macros to save on typing.  */
#define CHR         event->data.keyDown.chr
#define KEYCODE     event->data.keyDown.keyCode
#define MODIFIERS   event->data.keyDown.modifiers
#define OPTION      (MODIFIERS & optionKeyMask)


#define TREO_NOTIFY_PRIORITY	-16

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

static void EnqueueHardKey (UInt16 hardkey, Action action)
    HARDKEYS_SECTION;

static Int16 TreoSpecials (Boolean override, EventType *event,
			   Action action)
    HARDKEYS_SECTION;

static Int16 TungstenSpecials (Boolean override, EventType *event,
			       Action action)
    HARDKEYS_SECTION;

static Boolean DefaultOverrides (EventType *event, Action action)
    HARDKEYS_SECTION;

static Boolean DefaultHandler (EventType *event, Action action)
    HARDKEYS_SECTION;

/* On the Treo, override some key events at a very lowest level, with
 * a sysNotifyEventDequeuedEvent notification.  This is necessary to
 * prevent the Ringer Volume form and Butler's navigation features
 * from interfering with our fancy hard key handling.  */
static Err TreoNotifyCallback (SysNotifyParamType *p)
    HARDKEYS_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

/* Tru if we should process hard key events at all.  */
static Boolean enabled;

/* True if device has an old-style 5-way navigator,
   such as the Tungsten T2.  */
static Boolean old5way;

/* True if device has a new-style 5-way navigator,
   such as the Tungsten T5 or the Treo 650.  */
static Boolean new5way;

/* True if device has a thumbwheel.  */
static Boolean thumbwheel;

/* True if device has a thumbwheel with a back button.  */
static Boolean thumbwheelBack;

/* True if device has a dedicated keyboard.  */
static Boolean keyboard;

/* Normal keyclick.  */
#define KCN(keycode) GetKeyboardCmd (keycode, ActClick, ModCurrent)
/* Shifted keyclick.  */
#define KCS(keycode) GetKeyboardCmd (keycode, ActClick, ModCurrent | ModShiftOn)
/* Unshifted keyclick.  */
#define KC(keycode) GetKeyboardCmd (keycode, ActClick, ModCurrent | ModShiftOff)

/* Commands corresponding to ASCII virtual chars entered by Graffiti.  */
static Command virtualChars[0x80] = {
    CmdNull,                     /* chrNull */
    CmdNull,                     /* chrStartOfHeading */
    CmdNull,                     /* chrStartOfText */
    CmdNull,                     /* chrEndOfText */
    CmdNull,                     /* chrEndOfTransmission */
    CmdNull,                     /* chrEnquiry */
    CmdNull,                     /* chrAcknowledge */
    CmdNull,                     /* chrBell */
    KCN (KeyCodeBackspace),      /* chrBackspace */
    KCN (KeyCodeTab),            /* chrTab (chrHorizontalTabulation) */
    KCN (KeyCodeReturn),         /* chrLineFeed */
    KCN (KeyCodePageUp),         /* chrPageUp (chrVerticalTabulation) */
    KCN (KeyCodePageDown),       /* chrPageDown (chrFormFeed) */
    CmdNull,                     /* chrCarriageReturn */
    CmdNull,                     /* chrShiftOut */
    CmdNull,                     /* chrShiftIn */
    CmdNull,                     /* chrDataLinkEscape */
    CmdNull,                     /* chrDeviceControlOne */
    CmdNull,                     /* chrDeviceControlTwo */
    CmdNull,                     /* chrDeviceControlThree */
    CmdNull,                     /* chrOtaSecure (chrDeviceControlFour) */
    CmdNull,                     /* chrOta (chrNegativeAcknowledge) */
    CmdNull,                     /* chrCommandStroke (chrSynchronousIdle) */
    CmdNull,                     /* chrShortcutStroke (chrEndOfTransmissionBlock) */
    CmdNull,                     /* chrEllipsis (chrCancel) */
    KCN (KeyCodeSpace),          /* chrNumericSpace (chrEndOfMedium) */
    CmdNull,                     /* chrCardIcon (chrSubstitute) */
    KCN (KeyCodeLeftArrow),      /* chrLeftArrow (chrEscape) */
    KCN (KeyCodeRightArrow),     /* chrRightArrow (chrFileSeparator) */
    KCN (KeyCodeUpArrow),        /* chrUpArrow (chrGroupSeparator) */
    KCN (KeyCodeDownArrow),      /* chrDownArrow (chrRecordSeparator) */
    CmdNull,                     /* chrUnitSeparator */
    KCN (KeyCodeSpace),          /* chrSpace */
    KCS (KeyCode1),              /* chrExclamationMark */
    KCS (KeyCodeSingleQuote),    /* chrQuotationMark */
    KCS (KeyCode3),              /* chrNumberSign */
    KCS (KeyCode4),              /* chrDollarSign */
    KCS (KeyCode5),              /* chrPercentSign */
    KCS (KeyCode7),              /* chrAmpersand */
    KC (KeyCodeSingleQuote),     /* chrApostrophe */
    KCS (KeyCode9),              /* chrLeftParenthesis */
    KCS (KeyCode0),              /* chrRightParenthesis */
    KCS (KeyCode8),              /* chrAsterisk */
    KCS (KeyCodeEquals),         /* chrPlusSign */
    KC (KeyCodeComma),           /* chrComma */
    KC (KeyCodeDash),            /* chrHyphenMinus */
    KC (KeyCodePeriod),          /* chrFullStop */
    KC (KeyCodeSlash),           /* chrSolidus */
    KC (KeyCode0),               /* chrDigitZero */
    KC (KeyCode1),               /* chrDigitOne */
    KC (KeyCode2),               /* chrDigitTwo */
    KC (KeyCode3),               /* chrDigitThree */
    KC (KeyCode4),               /* chrDigitFour */
    KC (KeyCode5),               /* chrDigitFive */
    KC (KeyCode6),               /* chrDigitSix */
    KC (KeyCode7),               /* chrDigitSeven */
    KC (KeyCode8),               /* chrDigitEight */
    KC (KeyCode9),               /* chrDigitNine */
    KCS (KeyCodeSemiColon),      /* chrColon */
    KC (KeyCodeSemiColon),       /* chrSemicolon */
    KCS (KeyCodeComma),          /* chrLessThanSign */
    KC (KeyCodeEquals),          /* chrEqualsSign */
    KCS (KeyCodePeriod),         /* chrGreaterThanSign */
    KCS (KeyCodeSlash),          /* chrQuestionMark */
    KCS (KeyCode2),              /* chrCommercialAt */
    KCS (KeyCodeA),              /* chrCapital_A */
    KCS (KeyCodeB),              /* chrCapital_B */
    KCS (KeyCodeC),              /* chrCapital_C */
    KCS (KeyCodeD),              /* chrCapital_D */
    KCS (KeyCodeE),              /* chrCapital_E */
    KCS (KeyCodeF),              /* chrCapital_F */
    KCS (KeyCodeG),              /* chrCapital_G */
    KCS (KeyCodeH),              /* chrCapital_H */
    KCS (KeyCodeI),              /* chrCapital_I */
    KCS (KeyCodeJ),              /* chrCapital_J */
    KCS (KeyCodeK),              /* chrCapital_K */
    KCS (KeyCodeL),              /* chrCapital_L */
    KCS (KeyCodeM),              /* chrCapital_M */
    KCS (KeyCodeN),              /* chrCapital_N */
    KCS (KeyCodeO),              /* chrCapital_O */
    KCS (KeyCodeP),              /* chrCapital_P */
    KCS (KeyCodeQ),              /* chrCapital_Q */
    KCS (KeyCodeR),              /* chrCapital_R */
    KCS (KeyCodeS),              /* chrCapital_S */
    KCS (KeyCodeT),              /* chrCapital_T */
    KCS (KeyCodeU),              /* chrCapital_U */
    KCS (KeyCodeV),              /* chrCapital_V */
    KCS (KeyCodeW),              /* chrCapital_W */
    KCS (KeyCodeX),              /* chrCapital_X */
    KCS (KeyCodeY),              /* chrCapital_Y */
    KCS (KeyCodeZ),              /* chrCapital_Z */
    KC (KeyCodeOpenBracket),     /* chrLeftSquareBracket */
    KC (KeyCodeBackslash),       /* chrReverseSolidus */
    KC (KeyCodeCloseBracket),    /* chrRightSquareBracket */
    KCS (KeyCode6),              /* chrCircumflexAccent */
    KCS (KeyCodeDash),           /* chrLowLine */
    KC (KeyCodeBacktick),        /* chrGraveAccent */
    KC (KeyCodeA),               /* chrSmall_A */
    KC (KeyCodeB),               /* chrSmall_B */
    KC (KeyCodeC),               /* chrSmall_C */
    KC (KeyCodeD),               /* chrSmall_D */
    KC (KeyCodeE),               /* chrSmall_E */
    KC (KeyCodeF),               /* chrSmall_F */
    KC (KeyCodeG),               /* chrSmall_G */
    KC (KeyCodeH),               /* chrSmall_H */
    KC (KeyCodeI),               /* chrSmall_I */
    KC (KeyCodeJ),               /* chrSmall_J */
    KC (KeyCodeK),               /* chrSmall_K */
    KC (KeyCodeL),               /* chrSmall_L */
    KC (KeyCodeM),               /* chrSmall_M */
    KC (KeyCodeN),               /* chrSmall_N */
    KC (KeyCodeO),               /* chrSmall_O */
    KC (KeyCodeP),               /* chrSmall_P */
    KC (KeyCodeQ),               /* chrSmall_Q */
    KC (KeyCodeR),               /* chrSmall_R */
    KC (KeyCodeS),               /* chrSmall_S */
    KC (KeyCodeT),               /* chrSmall_T */
    KC (KeyCodeU),               /* chrSmall_U */
    KC (KeyCodeV),               /* chrSmall_V */
    KC (KeyCodeW),               /* chrSmall_W */
    KC (KeyCodeX),               /* chrSmall_X */
    KC (KeyCodeY),               /* chrSmall_Y */
    KC (KeyCodeZ),               /* chrSmall_Z */
    KCS (KeyCodeOpenBracket),    /* chrLeftCurlyBracket */
    KCS (KeyCodeBackslash),      /* chrVerticalLine */
    KCS (KeyCodeCloseBracket),   /* chrRightCurlyBracket */
    KCS (KeyCodeBacktick),       /* chrTilde */
    CmdNull,                     /* chrDelete */
};

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static void
EnqueueHardKey (UInt16 hardkey, Action action)
{
    if (hardkey != 0) {
        TRACE ("Hardkey %x %s", hardkey, HIDPActionStr (action));
        ENQUEUE (GetHardKeyCmd (action, hardkey));
    }
}



static Int16
TreoSpecials (Boolean override, EventType *event, Action action)
{
    if (!override)
        /* We override all keys on the Treo.  */
        return 0;

    if (!KEYCODE)
        /* Ignore virtual characters not generated by a hardkey. */
        return 0;

    /* Some of these characters (hsChrOpt*) conflict with other
       devices, so let's process all of them here.  */
    switch (CHR) {
	case vchrPageUp:
	case vchrRockerUp:
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyboardCmd (KeyCodePageUp, ActClick,
					 ModCurrent));
	    else
		EnqueueHardKey (HardKeyUp, action);
	    return 1;

	case vchrPageDown:
	case vchrRockerDown:
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyboardCmd (KeyCodePageDown, ActClick,
					 ModCurrent));
	    else
		EnqueueHardKey (HardKeyDown, action);
	    return 1;

	case vchrRockerLeft:
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyboardCmd (KeyCodeHome, ActClick,
					 ModCurrent));
	    else
		EnqueueHardKey (HardKeyLeft, action);
	    return 1;

	case vchrRockerRight:
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyboardCmd (KeyCodeEnd, ActClick, ModCurrent));
	    else
		EnqueueHardKey (HardKeyRight, action);
	    return 1;

	case vchrRockerCenter:      /* Treo 600 */
	case vchrHardRockerCenter:  /* Treo 650 and later */
	    EnqueueHardKey (HardKeyCenter, action);
	    return 1;

	case hsChrOptHard1:         /* Treo 600, 650, 700p: Phone */
	case vchrHard1:
	    EnqueueHardKey (HardKeyHard1, action);
	    return 1;

	case hsChrOptHard2:         /* Treo 600, 650, 700p: Calendar */
	case vchrHard2:
	    EnqueueHardKey (HardKeyHard2, action);
	    return 1;

	case hsChrOptHard3:         /* Treo 600, 650, 700p: Mail */
	case vchrHard3:
	    EnqueueHardKey (HardKeyHard3, action);
	    return 1;

	case hsChrOptHard4:
	case vchrHard4:             /* Treo 600: Screen
				       Treo 650: Power/End
				       Treo 700p: Home */
	    /* Don't handle these keys.  */
	    return -1;

	case vchrHard11:            /* Treo 700p: Send */
	case hsChrOptHard11:
	    EnqueueHardKey (HardKeySend, action);
	    return 1;

	case vchrHardPower:
	case hsChrOptHardPower:     /* Treo 600: Dedicated power button
				       Trep 700p: Dedicated power button */
	case vchrLaunch:            /* Treo 600: Home key
				       Treo 650: Home key
				       Treo 700p: Option + HardKey4 */
	case vchrMenu:              /* Treo 600, 650, 700p: Menu key */

	    /* Never handle these.  */
	    return -1;

	case hsChrSymbol:
	    EnqueueHardKey (HardKeyAlt, action);
	    return 1;

	case hsChrModifierKey:
	    switch (KEYCODE) {
		case keyLeftShift:
		    EnqueueHardKey (HardKeyLeftShift, action);
		    return 1;

		case keyRightShift:
		    EnqueueHardKey (HardKeyRightShift, action);
		    return 1;

		case keyLeftAlt:        /* Option key */
		    //EnqueueHardKey (HardKeyOption, action);
		    /* Don't handle this; let it do its normal job.  */
		    return -1;

		case hsKeySymbol:       /* Alt key */
		    EnqueueHardKey (HardKeyAlt, action);
		    return 1;

		case keyMenu:
		case keyLaunch:
		default:
		    /* Never redefine these keys.  */
		    return -1;
	    }

	case ' ':
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyClickCmd (KeyCodeTab));
	    else
		EnqueueHardKey (HardKeySpace, action);
	    return 1;

	case '\n':
	    EnqueueHardKey (HardKeyEnter, action);
	    return 1;

	case '\b':
	    if (OPTION && action != ActRelease)
		ENQUEUE (GetKeyClickCmd (KeyCodeEscape));
	    else
		EnqueueHardKey (HardKeyBackspace, action);
	    return 1;

	case '.':
	    if (OPTION && action != ActRelease)
		return -1;
	    EnqueueHardKey (HardKeyPeriod, action);
	    return 1;

	case '0':
	    if (OPTION && action != ActRelease)
		return -1;
	    EnqueueHardKey (HardKeyZero, action);
	    return 1;

	default:
	    return 0;
    }
}



static Int16
TungstenSpecials (Boolean override, EventType *event, Action action)
{
    if (override) {
        switch (CHR) {
	    case vchrHard5:         /* Tungsten T/T2 side key (Record) */
		EnqueueHardKey (HardKeySideSelect, action);
		return 1;
	    case vchrLaunch:
	    case vchrMenu:
		/* Never override these keys.  */
		return -1;

	    default:
		return 0;
        }
    }
    else {
        return 0;
    }
}



static Boolean
DefaultOverrides (EventType *event, Action action)
{
    if (old5way && IsFiveWayNavPalmEvent (event)) {
        if (KEYCODE & navChangeUp)
            EnqueueHardKey (HardKeyUp, ((KEYCODE & navBitUp)
                                        ? ActPress : ActRelease));
        if (KEYCODE & navChangeDown)
            EnqueueHardKey (HardKeyDown, ((KEYCODE & navBitDown)
                                          ? ActPress : ActRelease));
        if (KEYCODE & navChangeLeft)
            EnqueueHardKey (HardKeyLeft, ((KEYCODE & navBitLeft)
                                          ? ActPress : ActRelease));
        if (KEYCODE & navChangeRight)
            EnqueueHardKey (HardKeyRight, ((KEYCODE & navBitRight)
                                           ? ActPress : ActRelease));
        if (KEYCODE & navChangeSelect)
            EnqueueHardKey (HardKeyCenter, ((KEYCODE & navBitSelect)
                                            ? ActPress : ActRelease));
        return true;
    }


    switch (CHR) {
	case vchrLaunch:
	case vchrMenu:
	    /* Never override these keys.  */
	    return false;

	case vchrPageUp:
	case vchrRockerUp:
	    EnqueueHardKey (HardKeyUp, action);
	    return true;

	case vchrPageDown:
	case vchrRockerDown:
	    EnqueueHardKey (HardKeyDown, action);
	    return true;

	case vchrRockerLeft:
	    EnqueueHardKey (HardKeyLeft, action);
	    return true;

	case vchrRockerRight:
	    EnqueueHardKey (HardKeyRight, action);
	    return true;

	case vchrRockerCenter:      /* Treo 600 */
	case vchrHardRockerCenter:  /* Treo 650 and later */
	    EnqueueHardKey (HardKeyCenter, action);
	    return true;

	case vchrNavChange:     /*  Palm 5-way navigator state has
				 *  changed.  There may have been
				 *  multiple changes stuffed into the
				 *  same event.  */
	    if (KEYCODE & navChangeUp)
		EnqueueHardKey (HardKeyUp, ((KEYCODE & navBitUp)
					    ? ActPress : ActRelease));
	    if (KEYCODE & navChangeDown)
		EnqueueHardKey (HardKeyDown, ((KEYCODE & navBitDown)
					      ? ActPress : ActRelease));
	    if (KEYCODE & navChangeLeft)
		EnqueueHardKey (HardKeyLeft, ((KEYCODE & navBitLeft)
					      ? ActPress : ActRelease));
	    if (KEYCODE & navChangeRight)
		EnqueueHardKey (HardKeyRight, ((KEYCODE & navBitRight)
					       ? ActPress : ActRelease));
	    if (KEYCODE & navChangeSelect)
		EnqueueHardKey (HardKeyCenter, ((KEYCODE & navBitSelect)
						? ActPress : ActRelease));
	    return true;

	case vchrHard1:
	    EnqueueHardKey (HardKeyHard1, action);
	    return true;

	case vchrHard2:
	    EnqueueHardKey (HardKeyHard2, action);
	    return true;

	case vchrHard3:
	    EnqueueHardKey (HardKeyHard3, action);
	    return true;

	case vchrHard4:
	    EnqueueHardKey (HardKeyHard4, action);
	    return true;

	case hsChrVolumeUp:
	    EnqueueHardKey (HardKeySideUp, action);
	    return true;

	case hsChrVolumeDown:
	    EnqueueHardKey (HardKeySideDown, action);
	    return true;

	case hsChrSide:
	    EnqueueHardKey (HardKeySideSelect, action);
	    return true;

	default:
	    return false;
    }
}



static Boolean
DefaultHandler (EventType *event, Action action)
{
    if (KEYCODE) {
        if (CHR >= 'a' && CHR <= 'z') {
            EnqueueHardKey (HardKeyA + CHR - 'a', action);
            return true;
        }
        if (CHR >= 'A' && CHR <= 'Z') {
            EnqueueHardKey (HardKeyA + CHR - 'A', action);
            return true;
        }
        switch (CHR) {
	    case '\b':
		EnqueueHardKey (HardKeyBackspace, action);
		return true;
	    case '\n':
		EnqueueHardKey (HardKeyEnter, action);
		return true;
	    case '\t':
		EnqueueHardKey (HardKeyTab, action);
		return true;
	    case '\033':
		EnqueueHardKey (HardKeyEscape, action);
		return true;
	    case ' ':
		EnqueueHardKey (HardKeySpace, action);
		return true;

	    default:
		/* Convert symbols input from the keyboard to their
		 * corresponding keyboard commands.  */
		if (action != ActRelease &&
		    CHR <= 0x7F && virtualChars[CHR] != CmdNull) {
		    /* Symbols are input on the keyboard with the Option
		     * key.  We can use the virtualChars table to get the
		     * keycode/modifier values to send.
		     *
		     * In theory, we could handle separate presses and
		     * releases.  However, keeping a saved modifier state
		     * across press/release pairs is extremely hard to do
		     * correctly, and without that, shifted symbols would
		     * repeat wrong.  (E.g. pressing " would generate
		     * "''''''''...)
		     *
		     * On top of that, Treo 650 at least forgets the
		     * sticky option state at the moment of keypress, not
		     * release.  This means the release event won't
		     * correspond to its press.  Blech.
		     */
		    ENQUEUE (virtualChars[CHR]);

		    return true;
		}
		else
		    return false;
        }
    }
    else {                      /* No keycode */
        /* Convert Graffiti input characters to their corresponding
         * keyboard commands.  */
        if (CHR <= 0x7F && virtualChars[CHR] != CmdNull) {
            ENQUEUE (virtualChars[CHR]);
            return true;
        }
        else
            return false;
    }
}


static Err
TreoNotifyCallback (SysNotifyParamType *p)
{
    EventType *event;

    if (!enabled)
	return 0;
    
    if (p->notifyType != sysNotifyEventDequeuedEvent)
	return 0;

    if (!SPREF (hardkeyOverride))
	return 0;

    /*********************************************************
     * BEWARE! All data in *event is in little-endian order!
     *********************************************************/
    event = (EventType *) p->notifyDetailsP;
    switch (SWAP16 (event->eType)) {
	case keyHoldEvent:
	    TRACE ("++KeyHold c%x, k%x, m%x",
		   SWAP16 (CHR), SWAP16 (KEYCODE), SWAP16 (MODIFIERS));

	    switch (SWAP16 (CHR)) {
		case vchrHardPower:
		case hsChrOptHardPower:
		case vchrLaunch: 
		case vchrMenu:
		    /* Never handle these keys.  */
		    return 0;
	    }
	    
	    switch (SWAP16 (KEYCODE)) {
		default:
		    if (SWAP16 (KEYCODE) < keyA
			|| SWAP16 (KEYCODE) > keyZ)
			break;
		    /* Fallthrough.  */
		case keyVolumeUp:
		case keyVolumeDown:
		case keySide:
		    DEBUG ("Swallowed keyHold event for k%x",
			   SWAP16 (KEYCODE));
		    p->handled = true;
		    break;
	    }
	    break;

	case keyDownEvent:
	case keyUpEvent:
	{
	    Action action;
	    
	    TRACE ("++KeyDown c%x, k%x, m%x",
		   SWAP16 (CHR), SWAP16 (KEYCODE), SWAP16 (MODIFIERS));
	    
	    switch (SWAP16 (CHR)) {
		case vchrHardPower:
		case hsChrOptHardPower:
		case vchrLaunch: 
		case vchrMenu:
		    /* Never handle these keys.  */
		    return 0;
	    }

	    if (SWAP16 (event->eType) == keyDownEvent) {
		if (SWAP16 (MODIFIERS) & willSendUpKeyMask)
		    action = ActPress;
		else
		    action = ActClick;

		/* Button releases seem to be signalled as presses,
		 * but with the poweredOnKeyMask bit set.  */
		if (SWAP16 (MODIFIERS) & poweredOnKeyMask)
		    action = ActRelease;

		/* Button holds seem to be signalled as presses,
		 * but with the autoRepeatKeyMask bit set.  We need to
		 * override these events if we want to prevent Butler
		 * from taking away our events.
		 *
		 * The autoRepeatKeyMask bit is also set in button
		 * releases, so exclude that.  */
		if (action != ActRelease
		    && SWAP16 (MODIFIERS) & autoRepeatKeyMask) {
		    /* Make sure holds are always overridden.  */
		    p->handled = true;
		    /* Note that we still fall through to the switch
		       below. This is a hack to let the volume keys
		       autorepeat.  */
		    break;
		}
	    }
	    else {
		/* This branch is never taken in practice.  */
		action = ActRelease;
	    }
	    
	    switch (SWAP16 (KEYCODE)) {
		case keyVolumeUp:
		    DEBUG ("Overridden side up key");
		    EnqueueHardKey (HardKeySideUp, action);
		    p->handled = true;
		    break;
		    
		case keyVolumeDown:
		    DEBUG ("Overridden side down key");
		    EnqueueHardKey (HardKeySideDown, action);
		    p->handled = true;
		    break;
		    
		case keySide:
		    DEBUG ("Overridden side select key");
		    EnqueueHardKey (HardKeySideSelect, action);
		    p->handled = true;
		    break;
		    
		default:
		    break;
	    }
	    break;
	}
	default:
	    break;
    }
    if (p->handled) {
	event->eType = nilEvent;
	KEYCODE = 0;
	CHR = 0;
	MODIFIERS = 0;
	EvtWakeup();
    }
    return 0;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


void
HardKeyInit (void)
{
    UInt32 value;
    if (!FtrGet (navFtrCreator, navFtrVersion, &value)) {
        DEBUG ("Device has an old-style Palm 5-way Navigator");
        old5way = true;
    }

    if (!FtrGet (sysFileCSystem, sysFtrNumUIHardwareFlags, &value)) {
        if (value & sysFtrNumUIHardwareHas5Way) {
            DEBUG ("Device has a new-style 5-way navigator");
            new5way = true;
        }
        if (value & sysFtrNumUIHardwareHasThumbWheel) {
            DEBUG ("Device has a thumbwheel");
            thumbwheel = true;
        }
        if (value & sysFtrNumUIHardwareHasThumbWheelBack) {
            DEBUG ("Device has a thumbwheel with a back button");
            thumbwheelBack = true;
        }
        if (value & sysFtrNumUIHardwareHasKbd) {
            DEBUG ("Device has a dedicated keyboard");
            keyboard = true;
        }
    }

    switch (ToolsGetDeviceType()) {
	case DevPalmTreo600:
	case DevPalmTreo650:
	case DevPalmTreo700p:
	{
	    UInt16 cardNo;
	    LocalID dbID;
	    
	    SysCurAppDatabase (&cardNo, &dbID);
	    SysNotifyRegister (cardNo, dbID, sysNotifyEventDequeuedEvent,
			       TreoNotifyCallback,
			       TREO_NOTIFY_PRIORITY, g);
	    DEBUG ("Treo event notify callback registered");
	    break;
	}
	default:
	    break;
    }
}


void
HardKeyClose (void)
{
    switch (ToolsGetDeviceType()) {
	case DevPalmTreo600:
	case DevPalmTreo650:
	case DevPalmTreo700p:
	{
	    UInt16 cardNo;
	    LocalID dbID;
	    
	    SysCurAppDatabase (&cardNo, &dbID);
	    SysNotifyUnregister (cardNo, dbID,
				 sysNotifyEventDequeuedEvent,
				 TREO_NOTIFY_PRIORITY);
	    DEBUG ("Treo event notify callback unregistered");
	    break;
	}
	default:
    }
}



Boolean
HardKeyEnable (Boolean enable)
{
    Boolean previous = enabled;
    DEBUG ("Hard keys %s", (enable ? "enabled" : "disabled"));
    enabled = enable;
    return previous;
}



Boolean
HardKeyProcessEvent (Boolean override, EventType *event)
{
    int res = 0;
    Action action;

    if (!enabled)
        return false;

    if (override && !SPREF (hardkeyOverride))
	return false;

    switch (event->eType) {
	case keyDownEvent:
	    DEBUG ("KeyDown c%x, k%x, m%x", CHR, KEYCODE, MODIFIERS);
	    action = ActPress;
	    if (MODIFIERS & autoRepeatKeyMask)
		/* Ignore autorepeated keys, but handle them.  */
		return (MODIFIERS & virtualKeyMask 
			? false /* Unless they are virtual. */
			: true);

	    if (!(MODIFIERS & willSendUpKeyMask))
		/* If there will be no release, then send a release now.  */
		action = ActClick;
	    break;

	case keyUpEvent:
	    DEBUG ("KeyUp c%x, k%x, m%x", CHR, KEYCODE, MODIFIERS);
	    action = ActRelease;
	    break;

	case keyHoldEvent:
	    DEBUG ("KeyHold c%x, k%x, m%x", CHR, KEYCODE, MODIFIERS);
	    /* Ignore holds, but handle them.  */
	    return true;

	default:
	    /* Ignore all other event types.  */
	    return false;
    }

    switch (ToolsGetDeviceType()) {
	case DevPalmTreo600:
	case DevPalmTreo650:
	case DevPalmTreo700p:
	    res = TreoSpecials (override, event, action);
	    break;
	case DevPalmTungstenT:
	case DevPalmTungstenT2:
	case DevPalmTungstenT3:
	case DevPalmTungstenT5:
	    res = TungstenSpecials (override, event, action);
	    break;
	default:
	    res = 0;
    }

    if (res == 1)
        return true;
    else if (res == -1)
        return false;
    else if (override)
        return DefaultOverrides (event, action);
    else
        return DefaultHandler (event, action);
}
