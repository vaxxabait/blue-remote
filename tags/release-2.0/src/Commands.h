/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Commands.h
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
#define SECTION COMMANDS_SECTION

#ifdef NORMAL_INCLUDE

/* A command is simply a 32-bit integer value.  */
typedef UInt32 Command;

/* A command handler is used to execute a command.  */
typedef void (*CmdHandler) (Command cmd);

void f_CmdInit (Globals *g) SECTION;

#endif	/* NORMAL_INCLUDE */

DEFUN0 (void, CmdClose);
DEFUN1 (void, CmdExecute, Command cmd);
DEFUN2 (void, CmdRegisterHandler, Command cmd, CmdHandler handler);
DEFUN1 (Err, CmdGet, EventType *event);

/* Command queuing.  */
DEFUN1 (void, CmdQueue, Command cmd);

#ifdef NORMAL_INCLUDE

#define ENQUEUE	CmdQueue
#define EVENT_COMMAND(event) (((CommandEventType*)(event))->data.cmd)

/* The command event number.  */
#define commandEvent firstUserEvent

typedef struct {
    eventsEnum eType;	/* Always set to CommandEvent.  */
    Boolean penDown;	/* 0 */
    UInt8 tapCount;		/* 0 */
    Int16 screenX;		/* 0 */
    Int16 screenY;		/* 0 */
    union {
	struct _GenericEventType generic;
	Command cmd;
    } data;
} CommandEventType;


#define _CmdHighTypeBit		(1UL << 31)
#define _CmdLowTypeBit		(1UL << 30)
#define _CmdTypeMask		(_CmdHighTypeBit | _CmdLowTypeBit)

#define _CmdMouseAbsoluteTag	_CmdHighTypeBit
#define	_CmdMouseRelativeTag	(_CmdHighTypeBit | _CmdLowTypeBit)
#define _CmdKeyboardTag			_CmdLowTypeBit
#define _CmdButtonTag			(_CmdLowTypeBit | (1UL << 29))
#define _CmdMiscTag				0UL


/* Mouse movement to a position specified with absolute
 * coordinates. */
/* +--------+-------+--------+--------+
 * |10yyyyyy|yyyyyyy|yyxxxxxx|xxxxxxxx|
 * +--------+-------+--------+--------+ */
#define GetMouseAbsoluteCmd(x, y)		\
    (_CmdMouseAbsoluteTag			\
     | (((y) & 0x7fffUL) << 15) 		\
     | ((x) & 0x7fffUL))

#define CmdMouseAbsoluteX(cmd) ((cmd) & 0x7ffffUL)
#define CmdMouseAbsoluteY(cmd) (((cmd) >> 15) & 0x7ffffUL)

#define CmdIsMouseAbsolute(cmd) (((cmd) & 0xC0000000UL) == 0x80000000UL)


/* Mouse movement to a position specified with relative
 * coordinates.  */
/* +--------+-------+--------+--------+
 * |11yyyyyy|yyyyyyy|yyxxxxxx|xxxxxxxx|
 * +--------+-------+--------+--------+ */
#define GetMouseRelativeCmd(x, y)		\
    (_CmdMouseRelativeTag			\
     | ((((UInt32)(y)) & 0x7fffUL) << 15) 	\
     | (((UInt32)(x)) & 0x7fffUL))

#define _SignExtend15Bit(val)	((signed long)((val) & (1UL << 14)	\
                                               ? (val) | 0xffff8000UL	\
                                               : (val) & 0x00007fffUL))

#define CmdMouseRelativeX(cmd) _SignExtend15Bit(cmd)
#define CmdMouseRelativeY(cmd) _SignExtend15Bit((cmd) >> 15)

#define CmdIsMouseRelative(cmd) (((cmd) & 0xC0000000UL) == 0xC0000000UL)

/* Keyboard command.  */
/* +--------+-------+--------+--------+
 * |010tt00m|mmmmmmm|cccccccc|cccccccc|
 * +--------+-------+--------+--------+ */

#define GetKeyboardCmd(code, action, modifiers)	\
    (_CmdKeyboardTag				\
     | (((action) & 0x3UL) << 27)		\
     | (((modifiers) & 0x01ffUL) << 16)		\
     | ((code) & 0xffffUL))

#define GetKeyPressCmd(code) GetKeyboardCmd (code, ActPress, ModCurrent)
#define GetKeyReleaseCmd(code) GetKeyboardCmd (code, ActRelease, ModCurrent)
#define GetKeyClickCmd(code) GetKeyboardCmd (code, ActClick, ModCurrent)

#define CmdKeyboardKeycode(cmd) ((cmd) & 0xffffUL)
#define CmdKeyboardModifiers(cmd) (((cmd) >> 16) & 0x01ffUL)
#define CmdKeyboardAction(cmd) (((cmd) >> 27) & 0x3UL)

#define CmdIsKeyboard(cmd) (((cmd) & 0xE0000000UL) == 0x40000000UL)

/* Mouse button.  */
/* +--------+-------+--------+--------+
 * |011tt00m|mmmmmmm|bbbbbbbb|bbbbbbbb|
 * +--------+-------+--------+--------+ */

/* See above for type codes and modifier bits.  */

#define GetButtonCmd(number, action, modifiers)	\
    (_CmdButtonTag				\
     | (((action) & 0x3UL) << 27)		\
     | (((modifiers) & 0x01FFUL) << 16)		\
     | ((number) & 0xFFFFUL))

#define GetButtonPressCmd(number) GetButtonCmd ((number), ActPress, ModCurrent)
#define GetButtonReleaseCmd(number) GetButtonCmd ((number), ActRelease, ModCurrent)
#define GetButtonClickCmd(number) GetButtonCmd ((number), ActClick, ModCurrent)
#define GetButtonDoubleClickCmd(number) GetButtonCmd ((number), ActDoubleClick, ModCurrent)

#define CmdButtonNumber(cmd) ((cmd) & 0xffffUL)
#define CmdButtonModifiers(cmd) (((cmd) >> 16) & 0x01ffUL)
#define CmdButtonAction(cmd) (((cmd) >> 27) & 0x3UL)

#define CmdIsButton(cmd) (((cmd) & 0xE0000000UL) == 0x60000000UL)

/* Miscellaneous command.  */
/* +--------+-------+--------+--------+
 * |00tttttt|ddddddd|dddddddd|dddddddd|
 * +--------+-------+--------+--------+ */

#define CmdMiscTypeInternal		0 /* BlueRemote system events */
#define CmdMiscTypeHardKey		1 /* Hardkey events */
#define CmdMiscTypeAsyncResult		2 /* Asynchronous results */
#define CmdMiscTypeOther		3 /* Other input events */

#define GetMiscCmd(type, data)			\
    (_CmdMiscTag				\
     | (((type) & 0x3fUL) << 24)		\
     | ((data) & 0x00ffffffUL))

#define CmdMiscType(cmd) (((cmd) >> 24) & 0x3fUL)
#define CmdMiscData(cmd) ((cmd) & 0x00ffffffUL)

#define CmdIsMisc(cmd) (((cmd) & 0xC0000000UL) == 0x00000000UL)

/* Internal commands are handled entirely in the handheld.  */
#define GetInternalCmd(data) GetMiscCmd (CmdMiscTypeInternal, data)
#define CmdIsInternal(cmd) (CmdIsMisc(cmd) && CmdMiscType(cmd) == CmdMiscTypeInternal)

/* Hard Key commands correspond to presses and releases of hard keys.  */
#define GetHardKeyCmd(action, key)				\
    GetMiscCmd (CmdMiscTypeHardKey,				\
		((((UInt32)(action) & 0x00000003UL) << 16)	\
		 | ((UInt32)(key) & 0x0000FFFFUL)))
#define CmdHardKeyAction(cmd) (CmdMiscData (cmd) >> 16)
#define CmdHardKeyCode(cmd) (cmd & 0xFFFF)
#define CmdIsHardKey(cmd) (CmdIsMisc(cmd) && CmdMiscType(cmd) == CmdMiscTypeHardKey)

/* Async result commands are what WaitForResult waits for.  */
#define GetAsyncResult(kind, err)			\
    GetMiscCmd (CmdMiscTypeAsyncResult,			\
		(((UInt32)(kind) & 0x000000FFUL) << 16)	\
		| ((UInt32)(err) & 0x0000FFFFUL))
#define CmdAsyncKind(cmd) (CmdMiscData (cmd) >> 16)
#define CmdAsyncErr(cmd) (cmd & 0xFFFF)
#define CmdIsAsyncResult(cmd) (CmdIsMisc (cmd) && CmdMiscType(cmd) == CmdMiscTypeAsyncResult)

typedef enum {
    AsyncNone = 0,	   /* Never sent; used to implement delays. */
    AsyncRadio = 1,
    AsyncAccessibleMode = 2,
    AsyncACLConnect = 3,
    AsyncACLDisconnect = 4,
    AsyncAuthenticate = 5,
    AsyncEncrypt = 6,
    AsyncConnect = 7,
    AsyncSend = 8,
} AsyncKind;


/* "Other" commands are miscellaneous input events.  */
#define GetOtherCmd(code) GetMiscCmd (CmdMiscTypeOther, code)
#define CmdIsOther(cmd) (CmdIsMisc(cmd) && CmdMiscType(cmd) == CmdMiscTypeOther)
#define CmdOtherCode(cmd) CmdMiscData (cmd)

#define CmdWheelUp		GetOtherCmd(1)
#define CmdWheelDown 		GetOtherCmd(2)

/********************************************************/
/* Internal commands                                    */
/********************************************************/

/* No command.  */
#define CmdNull 			GetInternalCmd (0)

/* Exit the handheld application.  Sent after an unrecoverable error
 * or when the user switches applications.  */
#define CmdExitApp			GetInternalCmd (1)

/* User pressed Cancel on the progress dialog.  */
#define CmdProgressCanceled		GetInternalCmd (2)

/* Initiate a connect to the currently selected device.  */
#define CmdInitiateConnect		GetInternalCmd (20)

/* Wait for a connection from a new remote device.  */
#define CmdWaitConnection		GetInternalCmd (21)

/* Initiate a normal disconnect from the current remote device.  */
#define CmdInitiateDisconnect		GetInternalCmd (22)

/* Sent after the Bluetooth connection fails.  */
#define CmdConnectionError		GetInternalCmd (23)

/* Sent after a fatal Bluetooth radio problem.  */
#define CmdRadioError			GetInternalCmd (24)

/* The remote device has sent us a VIRTUAL_CABLE_UNPLUG request.  */
#define CmdUnplugReceived		GetInternalCmd (25)

/* Successfully paired with a remote device.  */
#define CmdPaired			GetInternalCmd (26)



/* Redraw the current form.  */
#define CmdRedraw			GetInternalCmd (70)

/* Reset the current HID state on forms.  */
#define CmdResetHIDState		GetInternalCmd (71)

/* Sent when the LED state changed.  */
#define CmdLedsChanged			GetInternalCmd (72)

/* Sent whenever some modifier states changed.  */
#define CmdModifiersChanged		GetInternalCmd (73)

/* Sent whenever the DevList or the selected device changes.  */
#define CmdDeviceListChanged		GetInternalCmd (74)

/* Show the "Welcome to BlueRemote" form.  */
#define CmdShowWelcome			GetInternalCmd (75)

/* Switch to another remote control form.  */
#define CmdSwitchForms 			GetInternalCmd (100)
#define CmdMouseForm 			GetInternalCmd (101)
#define CmdKeyboardForm 		GetInternalCmd (102)

/* Switch to another screen in the current remote control form.  */
#define CmdSwitchScreens 		GetInternalCmd (103)

/* Switch between screens of the Keyboard form.  */
#define CmdKbdSelectAlphaScreen		GetInternalCmd (200)
#define CmdKbdSelectNumericScreen	GetInternalCmd (201)
#define CmdKbdSelectCursorScreen	GetInternalCmd (202)
#define CmdKbdSelectFunctionScreen	GetInternalCmd (203)

/* Special buttons on the Mouse form.  */
#define CmdMouseSelectLeftButton	GetInternalCmd (300)
#define CmdMouseSelectRightButton	GetInternalCmd (301)
#define CmdMouseSelectMiddleButton	GetInternalCmd (302)
#define CmdMouseDrag			GetInternalCmd (303)


#endif	/* NORMAL_INCLUDE */
