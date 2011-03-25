/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Mouse.c
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

#define LOGLEVEL LogDebug

#include "Common.h"

#include "BlueRemote_res.h"
#include "MouseForm.h"
#include "Main.h"
#include "AppMenu.h"
#include "Hash.h"
#include "Commands.h"
#include "UITools.h"
#include "Bluetooth.h"
#include "Pref.h"
#include "HIDProtocol.h"
#include "HardKeys.h"

#define g globals

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Frequency of mouse pad commands, in Hertz.  */
#define MOUSE_PAD_FREQUENCY 50

/* Maximum number of milliseconds the pen can be held down for the
 * stroke to be considered a tap. */
#define MOUSE_TAP_MAX_DELAY     500UL

/* Maximum number of pixels the pen can move for the stroke to be
 * considered a tap.  */
#define MOUSE_TAP_MAX_MOVEMENT  10

#define HK_AUTOREPEAT_DELAY 	30
#define HK_AUTOREPEAT_FAST	10
#define HK_AUTOREPEAT_SLOW	30
#define HK_ACCEL_STEP  5
#define HK_ACCEL_LEVELS 4

#define BUTTON_GROUP_ID	10

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

/* Describes a selectable user interface element.  */
typedef struct Button {
    /* The object ID.  */
    UInt16 objectID;

    /* The command to be executed when the user selects the object.  */
    Command cmd;
} Button;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static Button *FindButtonByObjectID (UInt16 objectID)
    MOUSEFORM_SECTION;

static void UpdateButtonAfterClick (void)
    MOUSEFORM_SECTION;

static UInt16 MouseButtonIDToNumber (UInt16 button)
    MOUSEFORM_SECTION;

static void DrawMousePad (void)
    MOUSEFORM_SECTION;

static void MousePadUpdateState (Boolean draw, int screenX, int screenY)
    MOUSEFORM_SECTION;

static void MousePadDoMovement (Boolean force)
    MOUSEFORM_SECTION;

static Boolean MousePadHandler (struct FormGadgetTypeInCallback *gadget,
                                UInt16 cmd, void *param)
    MOUSEFORM_SECTION;

static Boolean HandleButtonSelectEvent (EventType *e)
    MOUSEFORM_SECTION;

static void MousePadDoClick (void)
    MOUSEFORM_SECTION;

static void DragMode (Boolean enabled)
    MOUSEFORM_SECTION;

static void DragStart (void)
    MOUSEFORM_SECTION;

static void DragStop (void)
    MOUSEFORM_SECTION;

static Boolean MouseFormHandleEvent (EventType* e)
    MOUSEFORM_SECTION;

static void MouseInitForm (FormDesc *d, FormType *f)
    MOUSEFORM_SECTION;

static UInt32 HardKeyTimer (Globals *g, UInt32 userdata)
    MOUSEFORM_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

static FormDesc mouseFormDesc = {
    NULL,
    MouseForm,
    0,
    0,
    MouseInitForm
};

static Button buttons[] = {
    { MouseRollUpRepeatButton, CmdWheelUp },
    { MouseRollDownRepeatButton, CmdWheelDown },
    { 0, CmdNull }
};

static Hash buttonsByObjectID;

/* Currently selected mouse button.  */
static UInt16 selectedButton = MouseLeftPushButton;

/* True if taps start dragging instead of clicking once.  */
static Boolean dragmode;

/* True if currently dragging.  */
static Boolean drag;

/* Bounds of the mouse pad gadget.  */
static RectangleType padBounds = { { 20, 37 }, { 115, 94 }};

/* The center of the mouse pad gadget.  */
static int centerX;
static int centerY;

/* True if the mouse pad is currently tracking pen movement.  */
static Boolean trackPen = false;

/* If trackPen is true, then these are the pen coordinates of the
 * original penDown event.  */
static Int16 origX;
static Int16 origY;

/* If trackPen is true, then these are the current pen coordinates.  */
static Int16 screenX;
static Int16 screenY;

/* If trackPen is true, then these are the coordinates of the
 * previously drawn cursor.  */
static Int16 drawnX;
static Int16 drawnY;

/* If nonzero, contains the display contents behind the currently
 * visible cursor.  */
static WinHandle savedWin = 0;

/* The time of the penDown event.  */
static UInt32 penDownTime;
static UInt32 sensitivity = 25;

/* True if we have already sent a movement command.  */
static Boolean moved = false;

/* The last time a movement command was sent.  */
static UInt32 lastCommandTime;

/* Original event timeout that was set before the mouse pad became
 * active.  */
static Int32 savedEventTimeout;


static Timer hardkeyTimer;
static UInt16 hardkey;
static UInt16 hardkeyIncrement;
static UInt32 hardkeyCount;
static UInt32 hardkeyRepeat;

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



static UInt16
MouseButtonIDToNumber (UInt16 button)
{
    switch (selectedButton) {
        case MouseLeftPushButton:
            return 1;
            break;
        case MouseRightPushButton:
            return 2;
            break;
        case MouseMiddlePushButton:
            return 3;
            break;
        default:
            ErrFatalDisplay ("Invalid selected button");
            return 0;
    }
}



static void
UpdateButtonAfterClick (void)
{
    /* Automatically return to left button after click.  */
    if (selectedButton != MouseLeftPushButton)
	FrmSetControlGroupSelection (FrmGetActiveForm(),
				     BUTTON_GROUP_ID,
				     MouseLeftPushButton);
    selectedButton = MouseLeftPushButton;
}



static void
DrawMousePad (void)
{
    //TRACE ("Drawing gadget");
    WinPaintRectangleFrame (roundFrame, &padBounds);
    WinDrawLine (centerX, centerY - 5, centerX, centerY + 5);
    WinDrawLine (centerX - 5, centerY, centerX + 5, centerY);
}



static void
MousePadUpdateState (Boolean draw, int screenX, int screenY)
{
    RectangleType win = { { screenX - 5, screenY - 5 }, { 10, 10 } };
    Err err = errNone;

    /* Delete the previous pointer, if necessary.  */
    if (savedWin != 0) {
        WinRestoreBits (savedWin, drawnX - 5, drawnY - 5);
    }

    if (!draw) {
        drawnX = centerX;
        drawnY = centerY;
        savedWin = 0;
        return;
    }

    savedWin = WinSaveBits (&win, &err);
    if (err != errNone)
        ErrFatalDisplay ("Can't store pointer background");
    WinDrawRectangle (&win, 5);
    win.topLeft.x++;
    win.topLeft.y++;
    win.extent.x -= 2;
    win.extent.y -= 2;
    WinEraseRectangle (&win, 4);
    drawnX = screenX;
    drawnY = screenY;
}



static void
MousePadDoMovement (Boolean force)
{
    Int32 distX;
    Int32 distY;
    Int32 moveX, moveY;
    int i;
    UInt32 time;
    UInt32 speedsqr;            /* Speed squared */

    i = 0;

    distX = screenX - origX;
    distY = screenY - origY;

    moveX = distX;
    moveY = distY;

    time = TimGetTicks() - lastCommandTime;
    if (time == 0)
        time = 1;

    speedsqr = (distY * distY + distX * distX) / (time * time);

#if 0
    if (speedsqr >= 2 * sensitivity) {
        moveX *= 4;
        moveY *= 4;
    }
    else
#endif
	if (speedsqr >= sensitivity) {
	    moveX *= 2;
	    moveY *= 2;
	}

    origX = screenX;
    origY = screenY;

    if (moveX != 0 || moveY != 0)
        HIDPMouseMove (moveX, moveY);

    lastCommandTime = TimGetTicks();
    moved = true;
}



static Boolean
MousePadHandler (struct FormGadgetTypeInCallback *gadget,
                 UInt16 cmd, void *param)
{
    Boolean handled = false;
    switch (cmd) {
        case formGadgetDrawCmd:
            /* Sent to active gadgets any time form is drawn or
	     * redrawn.  */
            //TRACE ("formGadgetDrawCmd");
            DrawMousePad();
            gadget->attr.visible = true;
            handled = true;
            break;

        case formGadgetHandleEventCmd:
        {
            EventType *e = param;
            /* Sent when form receives a gadget event.
             * param points to EventType structure.   */
            switch (e->eType) {
                case frmGadgetEnterEvent:
                    TRACE ("frmGadgetEnterEvent (%d, %d)",
                           e->screenX, e->screenY);
                    /* penDown in gadget's bounds.  */
                    screenX = e->screenX;
                    screenY = e->screenY;
                    origX = screenX;
                    origY = screenY;
                    penDownTime = TimGetTicks();
                    lastCommandTime = penDownTime;
                    savedEventTimeout = g_eventTimeout;
                    g_eventTimeout = 0; //sysTicksPerSecond / MOUSE_PAD_FREQUENCY;
                    MousePadUpdateState (true, e->screenX, e->screenY);

                    /* Don't send a command yet; the stroke may yet
                     * turn out to be a simple tap.  */
                    //MousePadDoMovement (true);

                    trackPen = true;
                    moved = false;
                    handled = true;
                    break;
                case frmGadgetMiscEvent:
                    /* This event is sent by the application
                       when it needs to send info to the gadget. */
                    TRACE ("frmGadgetMiscEvent: unknown event");
                    break;
                default:
                    LOG ("Unknown formGadgetHandleEventCmd");
                    break;
            }
            break;
        }
        case formGadgetDeleteCmd:
            /* Perform any cleanup prior to deletion.  */
            TRACE ("formGadgetDeleteCmd");
            break;
        case formGadgetEraseCmd:
            /* FrmHideObject takes care of this if you
             * return false.  */
            TRACE ("formGadgetEraseCmd");
            handled = false;
            break;
    }
    return handled;
}



static Boolean
HandleButtonSelectEvent (EventType *pEvent)
{
    Button *b;

    if (pEvent->eType != ctlSelectEvent
	&& pEvent->eType != ctlRepeatEvent)
        return false;

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
MousePadDoClick (void)
{
    if (dragmode) {
	if (drag)
	    DragStop();
	else
	    DragStart();
    }
    else {
        HIDPButton (ActClick,
                    MouseButtonIDToNumber (selectedButton),
                    ModCurrent);
	UpdateButtonAfterClick();
    }
}



static void
DragMode (Boolean enabled)
{
    FormType *pForm = FrmGetActiveForm();
    UInt16 idx = FrmGetObjectIndex (pForm, MouseDragCheckbox);

    DEBUG ("Drag Mode %s", (enabled ? "enabled" : "disabled"));
    FrmSetControlValue (pForm, idx, (enabled ? 1 : 0));
    if (drag && !enabled)
	DragStop();
    dragmode = enabled;
}



static void
DragStart (void)
{
    if (drag)
        return;

    if (!dragmode)
	DragMode (true);

    DEBUG ("DragStart");
    HIDPButton (ActPress,
                MouseButtonIDToNumber (selectedButton),
                ModCurrent);
    drag = true;
}



static void
DragStop (void)
{
    if (!drag)
        return;

    drag = false;
    HIDPButton (ActRelease,
                MouseButtonIDToNumber (selectedButton),
                ModCurrent);
    UpdateButtonAfterClick();
    if (dragmode) 
	DragMode (false);
}



static Boolean
MouseFormHandleEvent (EventType* e)
{
    Boolean handled = false;
    FormType* f = FrmGetActiveForm();

    /* Handle command buttons.  */
    handled = HandleButtonSelectEvent (e);
    if (handled)
        return true;

    switch (e->eType) {
	case menuOpenEvent:
	    MenuHideItem (CommonConnectionAddDevice);
	    MenuHideItem (CommonConnectionConnect);
	    MenuShowItem (CommonConnectionDisconnect);
	    return true;

	case menuEvent:
	    TRACE ("menuEvent");
	    return AppMenuDoCommand (e->data.menu.itemID);

	case frmOpenEvent:
	case frmUpdateEvent:
	{
	    UInt16 idx = FrmGetObjectIndex (f, MousePadGadget);
	    TRACE ("frmOpenEvent/frmUpdateEvent");
	    FrmSetGadgetHandler (f, idx, MousePadHandler);
	    /* Calculate center point.  */
	    centerX = padBounds.topLeft.x + padBounds.extent.x / 2;
	    centerY = padBounds.topLeft.y + padBounds.extent.y / 2;

	    drawnX = centerX;
	    drawnY = centerY;

	    /* Disable the Drag checkbox on a freshly loaded form.  */
	    DragMode (false);

	    /* Select the correct button.  */
	    FrmSetControlGroupSelection (f, BUTTON_GROUP_ID,
					 selectedButton);

	    FrmDrawForm (f);
	    HardKeyEnable (true);
	    FormDrawn (MouseForm);
	    return true;
	}
	case winExitEvent:
	case winEnterEvent:
	    TRACE ("winEnterEvent/winExitEvent");
	    if (e->data.winExit.exitWindow
		== (WindowType *) f)
		HardKeyEnable (false);
	    else if (e->data.winEnter.enterWindow
		     == (WindowType *) f)
		HardKeyEnable (true);
	    return false;

	case popSelectEvent:
	    TRACE ("popSelectEvent");
	    return HandleFormPopSelectEvent (e);

	case ctlSelectEvent:
	{
	    UInt16 id = e->data.ctlSelect.controlID;
	    switch (id) {
		case MouseLeftPushButton:
		case MouseRightPushButton:
		case MouseMiddlePushButton:
		    /* Selecting a different button stops dragging.  */
		    if (drag)
			DragMode (false);
		    selectedButton = id;
		    return true;

		case MouseDragCheckbox:
		{
		    UInt16 idx = FrmGetObjectIndex (f, MouseDragCheckbox);
		    DragMode (FrmGetControlValue (f, idx));
		    return true;
		}
		default:
		    return false;
	    }
	}

	case commandEvent:
	{
	    Command cmd = EVENT_COMMAND (e);
	    if (cmd == CmdSwitchScreens) {
		SwitchToForm (KeyboardForm);
		return true;
	    }
	    else if (CmdIsHardKey (cmd)) {
		UInt16 key = CmdHardKeyCode (cmd);
		Action action = CmdHardKeyAction (cmd);
		switch (key) {
		    case HardKeyUp:
		    case HardKeyDown:
		    case HardKeyLeft:
		    case HardKeyRight:
		    case HardKeySideUp:
		    case HardKeySideDown:
			switch (action) {
			    case ActPress:
				hardkeyIncrement = 4;
				hardkeyCount = 0;
				hardkey = key;
				if (key == HardKeySideUp
				    || key == HardKeySideDown)
				    hardkeyRepeat = HK_AUTOREPEAT_SLOW;
				else
				    hardkeyRepeat = HK_AUTOREPEAT_FAST;
				TimerReschedule (hardkeyTimer,
						 TimGetTicks() +
						 HK_AUTOREPEAT_DELAY);
				HardKeyTimer (g, 0);
				break;
			    case ActClick:
				HardKeyTimer (g, 0);
				break;
			    case ActRelease:
				hardkey = 0;
				TimerCancel (hardkeyTimer);
				break;
			    default:
				break;
			}
			return true;
			
		    case HardKeyCenter:
			HIDPButton (action, 1, ModCurrent);
			return true;

		    case HardKeySideSelect:
			HIDPButton (action, 1, ModCurrent);
			return true;

		    default:
			return false;
		}
	    }
	    return false;
	}
	default:
	    return false;
    }
}



static void
MouseInitForm (FormDesc *d, FormType *f)
{
    /* Preselect the Mouse entry in the popup list.  */
    UInt16 idx = FrmGetObjectIndex (f, MousePopupList);
    ListType *list = FrmGetObjectPtr (f, idx);
    LstSetSelection (list, 0);

    /* Set the event handler.  */
    FrmSetEventHandler(f, MouseFormHandleEvent);

    UPREF (currentRemoteFormID) = MouseForm;
}



static UInt32
HardKeyTimer (Globals *g, UInt32 userdata)
{
    if (Progressing() || g_currentForm->formID != MouseForm)
	return 0;

    hardkeyCount++;
    if ((hardkeyCount % HK_ACCEL_STEP) == 0
	&& (hardkeyCount / HK_ACCEL_STEP) < HK_ACCEL_LEVELS)
	hardkeyIncrement <<= 1;
    
    switch (hardkey) {
	case HardKeyUp:
	    HIDPMouseMove (0, -hardkeyIncrement);
	    break;
	case HardKeyDown:
	    HIDPMouseMove (0, hardkeyIncrement);
	    break;
	case HardKeyLeft:
	    HIDPMouseMove (-hardkeyIncrement, 0);
	    break;
	case HardKeyRight:
	    HIDPMouseMove (hardkeyIncrement, 0);
	    break;
	case HardKeySideUp:
	    HIDPWheelMove (1);
	    break;
	case HardKeySideDown:
	    HIDPWheelMove (-1);
	    break;
	default:
	    return 0;
    }
    return TimGetTicks() + hardkeyRepeat;
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


Boolean
MouseFormOverrideEvent (EventType *e)
{
    if (!trackPen)
        return false;

    switch (e->eType) {
	case penMoveEvent:
	    /* Track pen movement on the mouse pad gadget.  */
	    TRACE ("penMoveEvent (%hu, %hu)",
		   e->screenX, e->screenY);
	    screenX = e->screenX;
	    screenY = e->screenY;

	    /* Update mouse pad display.  */
	    MousePadUpdateState (true, screenX, screenY);

	    if (moved)
		MousePadDoMovement (false);
	    else {
		/* Start moving only if tap conditions have expired.  */
		if (((TimGetTicks() - penDownTime)
		     > (MOUSE_TAP_MAX_DELAY * sysTicksPerSecond / 1000UL))
		    || abs (screenX - origX) > MOUSE_TAP_MAX_MOVEMENT
		    || abs (screenY - origY) > MOUSE_TAP_MAX_MOVEMENT) {
		    MousePadDoMovement (true);
		}
	    }
	    return true;

	case penUpEvent:
	    TRACE ("penUpEvent");
	    MousePadUpdateState (false, e->screenX, e->screenY);
	    trackPen = false;
	    screenX = e->screenX;
	    screenY = e->screenY;
	    g_eventTimeout = savedEventTimeout;
	    TRACE ("Elapsed time: %lu", TimGetTicks() - penDownTime);
	    if (!moved
		&& ((TimGetTicks() - penDownTime)
		    <= (MOUSE_TAP_MAX_DELAY * sysTicksPerSecond / 1000UL))
		&& abs (screenX - origX) <= MOUSE_TAP_MAX_MOVEMENT
		&& abs (screenY - origY) <= MOUSE_TAP_MAX_MOVEMENT)
		MousePadDoClick();
	    return true;

	case nilEvent:
	{
	    Boolean down;
	    EvtGetPen (&screenX, &screenY, &down);
	    if (moved)
		MousePadDoMovement (false);
	    else {
		/* Start moving only if tap conditions have expired.  */
		if (((TimGetTicks() - penDownTime)
		     > (MOUSE_TAP_MAX_DELAY * sysTicksPerSecond / 1000UL))
		    || abs (screenX - origX) > MOUSE_TAP_MAX_MOVEMENT
		    || abs (screenY - origY) > MOUSE_TAP_MAX_MOVEMENT) {
		    MousePadDoMovement (true);
		}
	    }
	    return false;
	}
	default:
	    return false;
    }
}


void
MouseInit (void)
{
    int i;

    RegisterForm (&mouseFormDesc);

    buttonsByObjectID = HashCreate (10);
    for (i = 0; buttons[i].objectID != 0; i++) {
        void *old = HashPut (buttonsByObjectID,
                             buttons[i].objectID,
                             &buttons[i]);
        PANICIF (old != NULL, "Duplicate button");
    }

    hardkeyTimer = TimerNew ("Mouse hardkey handler", 0, HardKeyTimer, 0);
}


void
MouseClose (void)
{
    if (savedWin != 0) {
        WinDeleteWindow (savedWin, false);
        savedWin = 0;
    }
    if (buttonsByObjectID != 0)
        HashDelete (buttonsByObjectID);

    if (hardkeyTimer) {
	TimerFree (hardkeyTimer);
	hardkeyTimer = 0;
    }
}
