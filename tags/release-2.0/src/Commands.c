/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Commands.c
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

#define LOGLEVEL LogTrace

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Commands.h"
#undef PRIVATE_INCLUDE

#include "KeyboardForm.h"
#include "HardKeys.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* The size of the command event queue.  */
#define COMMAND_QUEUE_SIZE  16

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct CmdGlobals {
    /* The command queue.  */
    MsgQueuePtr queue;

    /* Repository of command handlers.  */
    Hash handlers;
} CmdGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static void ExecuteHardKey (Globals *g, Action action, UInt16 key)
    COMMANDS_SECTION;

static void ExecuteOther (Globals *g, UInt32 cmd)
    COMMANDS_SECTION;

static void ExecuteMisc (Globals *g, UInt8 type, UInt32 data)
    COMMANDS_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_queue     c->queue
#define g_handlers  c->handlers

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static void
ExecuteHardKey (Globals *g, Action action, UInt16 key)
{
    TRACE ("Hard Key %x %s", key, HIDPActionStr (action));
    if (key >= HardKeyA && key <= HardKeyZ) {
        HIDPKeyboard (action, KeyCodeA + key - HardKeyA, ModCurrent);
        return;
    }
    switch (key) {
	case HardKeySpace:
	    HIDPKeyboard (action, KeyCodeSpace, ModCurrent);
	    break;
	case HardKeyEnter:
	    HIDPKeyboard (action, KeyCodeReturn, ModCurrent);
	    break;
	case HardKeyBackspace:
	    HIDPKeyboard (action, KeyCodeBackspace, ModCurrent);
	    break;
	case HardKeyUp:
	    HIDPKeyboard (action, KeyCodeUpArrow, ModCurrent);
	    break;
	case HardKeyDown:
	    HIDPKeyboard (action, KeyCodeDownArrow, ModCurrent);
	    break;
	case HardKeyLeft:
	    HIDPKeyboard (action, KeyCodeLeftArrow, ModCurrent);
	    break;
	case HardKeyRight:
	    HIDPKeyboard (action, KeyCodeRightArrow, ModCurrent);
	    break;
	case HardKeyCenter:
	    HIDPKeyboard (action, KeyCodeReturn, ModCurrent);
	    break;
	case HardKeyHard1:
	    if (action == ActPress || action == ActClick)
		ENQUEUE (CmdSwitchForms);
	    break;
	case HardKeyHard2:
	    if (action == ActPress || action == ActClick)
		ENQUEUE (CmdSwitchScreens);
	    break;
	case HardKeyHard3:
	    HIDPButton (action, 3, ModCurrent);
	    break;
	case HardKeyHard4:
	    // TODO
	    break;
	case HardKeySideUp:
	    if (action == ActPress || action == ActClick)
		HIDPWheelMove (1);
	    break;
	case HardKeySideDown:
	    if (action == ActPress || action == ActClick)
		HIDPWheelMove (-1);
	    break;
	case HardKeySideSelect:
	    HIDPButton (action, 1, ModCurrent);
	    break;

	case HardKeyLeftShift:
	case HardKeyRightShift:
	    HIDPKeyboard (action, KeyCodeLeftShift, ModCurrent);
	    break;

	case HardKeyAlt:
	    HIDPKeyboard (action, KeyCodeLeftAlt, ModCurrent);
	    break;

	case HardKeyZero:
	    HIDPKeyboard (action, KeyCodeLeftControl, ModCurrent);
	    break;

	case HardKeyPeriod:
	    HIDPKeyboard (action, KeyCodeLeftGUI, ModCurrent);
	    break;

	default:
	    break;
    }
}


static void
ExecuteOther (Globals *g, UInt32 cmd)
{
    TRACE ("Other command %ld", CmdOtherCode (cmd));
    switch (cmd) {
	case CmdWheelUp:
	    HIDPWheelMove (1);
	    break;
	case CmdWheelDown:
	    HIDPWheelMove (-1);
	    break;
	default:
	    LOG ("Unknown other command");
	    break;
    }
}


static void
ExecuteMisc (Globals *g, UInt8 type, UInt32 data)
{
    TRACE ("type = %hu, data = %lu", type, data);
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


void
f_CmdInit (Globals *g)
{
    CmdGlobals *c;
    if (g->cmd)
        MemPtrFree (g->cmd);
    g->cmd = CheckedMemPtrNew (sizeof (CmdGlobals), true);
    c = g->cmd;

    g_queue = MsgNewQueue ("CmdQueue", 20, true, false, NULL);
    g_handlers = HashCreate (128);
}



void
f_CmdClose (Globals *g)
{
    CmdGlobals *c = g->cmd;
    if (c == 0)
        return;

    if (g_queue != 0)
        MsgFreeQueue (g_queue);

    if (g_handlers != 0)
        HashDelete (g_handlers);

    MemPtrFree (g->cmd);
    g->cmd = 0;
}



void
f_CmdExecute (Globals *g, Command cmd)
{
    CmdHandler handler;
    CmdGlobals *c = g->cmd;
    if (c == 0) return;

    TRACE ("CmdExecute %lu (0x%lx)", cmd, cmd);

    /* Use the registered handler, if possible.  */
    handler = HashGet (g_handlers, cmd);
    if (handler != NULL) {
        handler (cmd);
        return;
    }

    if (CmdIsMouseAbsolute (cmd)) {
        UInt16 x = CmdMouseAbsoluteX (cmd);
        UInt16 y = CmdMouseAbsoluteY (cmd);
        // TODO
        x = x;
        y = y;
    }
    else if (CmdIsMouseRelative (cmd)) {
        HIDPMouseMove (CmdMouseRelativeX (cmd),
                       CmdMouseRelativeY (cmd));
    }
    else if (CmdIsKeyboard (cmd)) {
        HIDPKeyboard (CmdKeyboardAction (cmd),
                      CmdKeyboardKeycode (cmd),
                      CmdKeyboardModifiers (cmd));
    }
    else if (CmdIsButton (cmd)) {
        HIDPButton (CmdButtonAction (cmd),
                    CmdButtonNumber (cmd),
                    CmdButtonModifiers (cmd));
    }
    else if (CmdIsHardKey (cmd)) {
        ExecuteHardKey (g, CmdHardKeyAction (cmd), CmdHardKeyCode (cmd));
    }
    else if (CmdIsOther (cmd)) {
        ExecuteOther (g, cmd);
    }
    else if (CmdIsMisc (cmd)) {
        ExecuteMisc (g, CmdMiscType (cmd),
                     CmdMiscData (cmd));
    }
}



void
f_CmdRegisterHandler (Globals *g, Command cmd, CmdHandler handler)
{
    CmdHandler old;
    CmdGlobals *c = g->cmd;
    if (c == 0) return;

    old = HashPut (g_handlers, cmd, handler);
    if (old)
        ERROR ("Command handler overriden: %lx", cmd);
}



Err
f_CmdGet (Globals *g, EventType *event)
{
    CommandEventType *ce = (CommandEventType *)event;
    CmdGlobals *c = g->cmd;
    if (c == 0) return sysErrNoFreeResource;


    if (!g_queue || !MsgAvailable (g_queue))
        return sysErrNoFreeResource;

    ce->eType = commandEvent;
    ce->penDown = 0;
    ce->tapCount = 0;
    ce->screenX = 0;
    ce->screenY = 0;
    ce->data.cmd = (Command) MsgGet (g_queue);
    MsgProcessed (g_queue, (MemHandle) ce->data.cmd);

    return errNone;
}


void
f_CmdQueue (Globals *g, Command cmd)
{
    CmdGlobals *c = g->cmd;
    if (c == 0) return;
    MsgAddHandle (g_queue, (MemHandle) cmd);
}

