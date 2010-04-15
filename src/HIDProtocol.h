/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: HIDProtocol.h
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
#define SECTION HIDPROTOCOL_SECTION

#ifdef NORMAL_INCLUDE

/* LED bits.  */
#define LEDNumLock          0x01
#define LEDCapsLock         0x02
#define LEDScrollLock       0x04

#define LED_KEYCODE_TO_BIT(keycode)				\
    (keycode == KeyCodeKeypadNumLock ? LEDNumLock		\
     : (keycode == KeyCodeCapsLock ? LEDCapsLock		\
        : (keycode == KeyCodeScrollLock ? LEDScrollLock		\
           : (PANIC ("Invalid led keycode %x", keycode), 0))))

/* If set, use the current modifier state.
   Otherwise  */
#define ModCurrent      0x0100
/* Modifier bits if ModCurrent is unset.  This bitmask overrides
 * any current modifier state.  This is useful to bind UI buttons to
 * specific keyboard combinations like Alt-F4 etc.  */
/* These match the modifier bitmask positions for the HID keyboard
 * boot protocol.  */
#define ModLeftControl  0x01
#define ModLeftShift    0x02
#define ModLeftAlt      0x04
#define ModLeftGUI      0x08
#define ModRightControl 0x10
#define ModRightShift   0x20
#define ModRightAlt     0x40
#define ModRightGUI     0x80

#define MODIFIER_KEYCODE_TO_BIT(keycode)				\
    ((keycode >= KeyCodeLeftControl && keycode <= KeyCodeRightGUI)	\
     ? (1 << (keycode - KeyCodeLeftControl))				\
     : (PANIC ("Invalid modifier keycode %x", keycode), 0))

#define ModControlMask  0x11
#define ModShiftMask    0x22
#define ModAltMask      0x44
#define ModGUIMask      0x88
#define ModMask         0xFF


/* Modifier bits if CmdModCurrent is set.  Normally these are all
   unset, but they may be used to selectively override the current
   modifier state.  This mechanism is useful with virtual characters
   drawn with Graffiti.  For example, an exclamation point vchr could
   be converted to a command like this, assuming a standard US
   keyboard layout:

   GetKeyboardCmd (CmdKeyCode1, ModCurrent | ModShiftOn)

   This ignores the current shift state, but applies any
   Control/Alt/GUI modifiers, so that combinations like Alt-! will
   work as expected. */
#define ModShiftOn      (1 << 1)
#define ModShiftOff     (1 << 2)
#define ModRightAltOn   (1 << 3)
#define ModRightAltOff  (1 << 4)


typedef enum {
    ModReleased,
    ModPressed,
    ModSticky,
    ModStickyPressed,
    ModLocked,
    ModLockedPressed,
} ModState;

/* Type codes for keyboard commands.  */
typedef enum {
    ActPress = 0,
    ActRelease = 1,
    ActClick = 2,
    ActDoubleClick = 3
} Action;


/* Available keycodes.  Usage Page 0x07 from the USB HID Usage Tables.  */

typedef enum {
    KeyCodeNull                     = 0x0000, /* Not a physical key */

    KeyCodeErrorRollOver            = 0x0001, /* Not a physical key */
    KeyCodePOSTFail                 = 0x0002, /* Not a physical key */
    KeyCodeErrorUndefined           = 0x0003, /* Not a physical key */

    KeyCodeA                        = 0x0004,
    KeyCodeB                        = 0x0005,
    KeyCodeC                        = 0x0006,
    KeyCodeD                        = 0x0007,
    KeyCodeE                        = 0x0008,
    KeyCodeF                        = 0x0009,
    KeyCodeG                        = 0x000A,
    KeyCodeH                        = 0x000B,
    KeyCodeI                        = 0x000C,
    KeyCodeJ                        = 0x000D,
    KeyCodeK                        = 0x000E,
    KeyCodeL                        = 0x000F,
    KeyCodeM                        = 0x0010,
    KeyCodeN                        = 0x0011,
    KeyCodeO                        = 0x0012,
    KeyCodeP                        = 0x0013,
    KeyCodeQ                        = 0x0014,
    KeyCodeR                        = 0x0015,
    KeyCodeS                        = 0x0016,
    KeyCodeT                        = 0x0017,
    KeyCodeU                        = 0x0018,
    KeyCodeV                        = 0x0019,
    KeyCodeW                        = 0x001A,
    KeyCodeX                        = 0x001B,
    KeyCodeY                        = 0x001C,
    KeyCodeZ                        = 0x001D,

    KeyCode1                        = 0x001E,
    KeyCode2                        = 0x001F,
    KeyCode3                        = 0x0020,
    KeyCode4                        = 0x0021,
    KeyCode5                        = 0x0022,
    KeyCode6                        = 0x0023,
    KeyCode7                        = 0x0024,
    KeyCode8                        = 0x0025,
    KeyCode9                        = 0x0026,
    KeyCode0                        = 0x0027,

    KeyCodeReturn                   = 0x0028,
    KeyCodeEscape                   = 0x0029,
    KeyCodeBackspace                = 0x002A,
    KeyCodeTab                      = 0x002B,
    KeyCodeSpace                    = 0x002C,

    KeyCodeDash                     = 0x002D,
    KeyCodeEquals                   = 0x002E,
    KeyCodeOpenBracket              = 0x002F,
    KeyCodeCloseBracket             = 0x0030,
    KeyCodeBackslash                = 0x0031,
    KeyCodeNonUSPound               = 0x0032,
    KeyCodeSemiColon                = 0x0033,
    KeyCodeSingleQuote              = 0x0034,
    KeyCodeBacktick                 = 0x0035,
    KeyCodeComma                    = 0x0036,
    KeyCodePeriod                   = 0x0037,
    KeyCodeSlash                    = 0x0038,
    KeyCodeCapsLock                 = 0x0039,

    KeyCodeF1                       = 0x003A,
    KeyCodeF2                       = 0x003B,
    KeyCodeF3                       = 0x003C,
    KeyCodeF4                       = 0x003D,
    KeyCodeF5                       = 0x003E,
    KeyCodeF6                       = 0x003F,
    KeyCodeF7                       = 0x0040,
    KeyCodeF8                       = 0x0041,
    KeyCodeF9                       = 0x0042,
    KeyCodeF10                      = 0x0043,
    KeyCodeF11                      = 0x0044,
    KeyCodeF12                      = 0x0045,

    KeyCodePrintScreen              = 0x0046,
    KeyCodeScrollLock               = 0x0047,
    KeyCodePause                    = 0x0048,
    KeyCodeInsert                   = 0x0049,
    KeyCodeHome                     = 0x004A,
    KeyCodePageUp                   = 0x004B,
    KeyCodeDelete                   = 0x004C,
    KeyCodeEnd                      = 0x004D,
    KeyCodePageDown                 = 0x004E,

    KeyCodeRightArrow               = 0x004F,
    KeyCodeLeftArrow                = 0x0050,
    KeyCodeDownArrow                = 0x0051,
    KeyCodeUpArrow                  = 0x0052,
    KeyCodeKeypadNumLock            = 0x0053,
    KeyCodeKeypadSlash              = 0x0054,
    KeyCodeKeypadAsterisk           = 0x0055,
    KeyCodeKeypadDash               = 0x0056,
    KeyCodeKeypadPlus               = 0x0057,
    KeyCodeKeypadEnter              = 0x0058,
    KeyCodeKeypad1                  = 0x0059,
    KeyCodeKeypad2                  = 0x005A,
    KeyCodeKeypad3                  = 0x005B,
    KeyCodeKeypad4                  = 0x005C,
    KeyCodeKeypad5                  = 0x005D,
    KeyCodeKeypad6                  = 0x005E,
    KeyCodeKeypad7                  = 0x005F,
    KeyCodeKeypad8                  = 0x0060,
    KeyCodeKeypad9                  = 0x0061,
    KeyCodeKeypad0                  = 0x0062,
    KeyCodeKeypadPeriod             = 0x0063,

    KeyCodeNonUSBackslash           = 0x0064,  /* Near left shift */
    KeyCodeApplication              = 0x0065,  /* Win95 win & Compose */
    KeyCodePower                    = 0x0066,  /* Not a physical key */

    KeyCodeKeypadEquals             = 0x0067,

    KeyCodeF13                      = 0x0068,
    KeyCodeF14                      = 0x0069,
    KeyCodeF15                      = 0x006A,
    KeyCodeF16                      = 0x006B,
    KeyCodeF17                      = 0x006C,
    KeyCodeF18                      = 0x006D,
    KeyCodeF19                      = 0x006E,
    KeyCodeF20                      = 0x006F,
    KeyCodeF21                      = 0x0070,
    KeyCodeF22                      = 0x0071,
    KeyCodeF23                      = 0x0072,
    KeyCodeF24                      = 0x0073,

    KeyCodeExecute                  = 0x0074,
    KeyCodeHelp                     = 0x0075,
    KeyCodeMenu                     = 0x0076,
    KeyCodeSelect                   = 0x0077,
    KeyCodeStop                     = 0x0078,
    KeyCodeAgain                    = 0x0079,
    KeyCodeUndo                     = 0x007A,
    KeyCodeCut                      = 0x007B,
    KeyCodeCopy                     = 0x007C,
    KeyCodePaste                    = 0x007D,
    KeyCodeFind                     = 0x007E,
    KeyCodeMute                     = 0x007F,

    KeyCodeVolumeUp                 = 0x0080,
    KeyCodeVolumeDown               = 0x0081,

    KeyCodeLockingCapsLock          = 0x0082, /* Legacy, don't use */
    KeyCodeLockingNumLock           = 0x0083, /* Legacy, don't use */
    KeyCodeLockingScrollLock        = 0x0084, /* Legacy, don't use */

    KeyCodeKeypadComma              = 0x0085,
    KeyCodeKeypadEqualSign          = 0x0086, /* AS/400 */

    KeyCodeInternational1           = 0x0087,
    KeyCodeInternational2           = 0x0088,
    KeyCodeInternational3           = 0x0089,
    KeyCodeInternational4           = 0x008A,
    KeyCodeInternational5           = 0x008B,
    KeyCodeInternational6           = 0x008C,
    KeyCodeInternational7           = 0x008D,
    KeyCodeInternational8           = 0x008E,
    KeyCodeInternational9           = 0x008F,

    KeyCodeLANG1                    = 0x0090,
    KeyCodeLANG2                    = 0x0091,
    KeyCodeLANG3                    = 0x0092,
    KeyCodeLANG4                    = 0x0093,
    KeyCodeLANG5                    = 0x0094,
    KeyCodeLANG6                    = 0x0095,
    KeyCodeLANG7                    = 0x0096,
    KeyCodeLANG8                    = 0x0097,
    KeyCodeLANG9                    = 0x0098,

    KeyCodeAlternateErase           = 0x0099,
    KeyCodeSysReq                   = 0x009A,

    KeyCodeCancel                   = 0x009B,
    KeyCodeClear                    = 0x009C,
    KeyCodePrior                    = 0x009D,
    KeyCodeReturn2                  = 0x009E,
    KeyCodeSeparator                = 0x009F,
    KeyCodeOut                      = 0x00A0,
    KeyCodeOper                     = 0x00A1,
    KeyCodeClearAgain               = 0x00A2,
    KeyCodeCrSelProps               = 0x00A3,
    KeyCodeExSel                    = 0x00A4,

#if 0  /*  Reserved keycodes */
    KeyCode                         = 0x00A5,
    KeyCode                         = 0x00A6,
    KeyCode                         = 0x00A7,
    KeyCode                         = 0x00A8,
    KeyCode                         = 0x00A9,
    KeyCode                         = 0x00AA,
    KeyCode                         = 0x00AB,
    KeyCode                         = 0x00AC,
    KeyCode                         = 0x00AD,
    KeyCode                         = 0x00AE,
    KeyCode                         = 0x00AF,
#endif

    KeyCodeKeypad00                 = 0x00B0,
    KeyCodeKeypad000                = 0x00B1,
    KeyCodeThousandsSeparator       = 0x00B2,
    KeyCodeDecimalSeparator         = 0x00B3,
    KeyCodeCurrencyUnit             = 0x00B4,
    KeyCodeCurrencySubUnit          = 0x00B5,
    KeyCodeKeypadLeftParen          = 0x00B6,
    KeyCodeKeypadRightParen         = 0x00B7,
    KeyCodeKeypadLeftBrace          = 0x00B8,
    KeyCodeKeypadRightBrace         = 0x00B9,
    KeyCodeKeypadTab                = 0x00BA,
    KeyCodeKeypadBackspace          = 0x00BB,
    KeyCodeKeypadA                  = 0x00BC,
    KeyCodeKeypadB                  = 0x00BD,
    KeyCodeKeypadC                  = 0x00BE,
    KeyCodeKeypadD                  = 0x00BF,
    KeyCodeKeypadE                  = 0x00C0,
    KeyCodeKeypadF                  = 0x00C1,
    KeyCodeKeypadXOR                = 0x00C2,
    KeyCodeKeypadCaret              = 0x00C3,
    KeyCodeKeypadPercent            = 0x00C4,
    KeyCodeKeypadLessThan           = 0x00C5,
    KeyCodeKeypadGreaterThan        = 0x00C6,
    KeyCodeKeypadAmpersand          = 0x00C7,
    KeyCodeKeypadDoubleAmpersand    = 0x00C8,
    KeyCodeKeypadPipe               = 0x00C9,
    KeyCodeKeypadDoublePipe         = 0x00CA,
    KeyCodeKeypadColon              = 0x00CB,
    KeyCodeKeypadHashmark           = 0x00CC,
    KeyCodeKeypadSpace              = 0x00CD,
    KeyCodeKeypadAt                 = 0x00CE,
    KeyCodeKeypadExclamationPoint   = 0x00CF,

    KeyCodeKeypadMemoryStore        = 0x00D0,
    KeyCodeKeypadMemoryRecall       = 0x00D1,
    KeyCodeKeypadMemoryClear        = 0x00D2,
    KeyCodeKeypadMemoryAdd          = 0x00D3,
    KeyCodeKeypadMemorySubtract     = 0x00D4,
    KeyCodeKeypadMemoryMultiply     = 0x00D5,
    KeyCodeKeypadMemoryDivide       = 0x00D6,
    KeyCodeKeypadPlusMinus          = 0x00D7,
    KeyCodeKeypadClear              = 0x00D8,
    KeyCodeKeypadClearEntry         = 0x00D9,
    KeyCodeKeypadBinary             = 0x00DA,
    KeyCodeKeypadOctal              = 0x00DB,
    KeyCodeKeypadDecimal            = 0x00DC,
    KeyCodeKeypadHexadecimal        = 0x00DD,
#if 0 /* Reserved */
    KeyCodeKeypad                   = 0x00DE,
    KeyCodeKeypad                   = 0x00DF,
#endif
    KeyCodeLeftControl              = 0x00E0,
    KeyCodeLeftShift                = 0x00E1,
    KeyCodeLeftAlt                  = 0x00E2,
    KeyCodeLeftGUI                  = 0x00E3,
    KeyCodeRightControl             = 0x00E4,
    KeyCodeRightShift               = 0x00E5,
    KeyCodeRightAlt                 = 0x00E6,
    KeyCodeRightGUI                 = 0x00E7,
} KeyCode;

void f_HIDPInit (Globals *g) SECTION;

#endif

DEFUN0 (void, HIDPClose);

DEFUN0 (MemHandle, HIDPProcessMessages);

DEFUN3 (MemHandle, HIDPKeyboard, Action action, KeyCode keycode, UInt16 modifiers);
DEFUN2 (MemHandle, HIDPMouseMove, Int16 x, Int16 y);
DEFUN3 (MemHandle, HIDPButton, Action action, UInt16 button, UInt16 modifiers);
DEFUN1 (MemHandle, HIDPWheelMove, Int16 dir);

DEFUN0 (UInt8, HIDPLeds);
DEFUN0 (UInt16, HIDPModifiers);
DEFUN1 (ModState, HIDPModifierState, KeyCode keycode);

DEFUN1 (char *, HIDPModStateStr, UInt8 state);
DEFUN1 (char *, HIDPActionStr, Action action);

/* Send state changes to host, if necessary.  If FORCE, then send
 * the current report even if it's the same as the previous one
 * sent. */
DEFUN1 (MemHandle, HIDPSendReport, Boolean force);

/* Reset the HID state, i.e., release all buttons.  */
DEFUN0 (MemHandle, HIDPReset);
