/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: HardKeys.h
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

#ifndef HARDKEYS_H
#define HARDKEYS_H

void HardKeyInit (void)
    HARDKEYS_SECTION;

void HardKeyClose (void)
    HARDKEYS_SECTION;

/* Enable or disable hard key processing.  Returns previous status.  */
Boolean HardKeyEnable (Boolean enable)
    HARDKEYS_SECTION;

Boolean HardKeyProcessEvent (Boolean override, EventType *event)
    HARDKEYS_SECTION;


typedef enum {
    HardKeyUnknown = 0,

    /* Five-way navigation */
    HardKeyUp,              /* Standard Up button */
    HardKeyDown,            /* Standard Down button */
    HardKeyLeft,            /* Standard Left button */
    HardKeyRight,           /* Standard Right button */
    HardKeyCenter,          /* Standard Center button */

    /* Standard hard keys.  */
    HardKeyHard1,
    HardKeyHard2,
    HardKeyHard3,
    HardKeyHard4,

    /* Side keys on Treo */
    HardKeySideUp,          /* Side key (Treo) */
    HardKeySideDown,        /* Side key (Treo) */
    HardKeySideSelect,      /* Side key (Treo) */

    /* QWERTY keyboard on Treo */
    HardKeyBackspace,
    HardKeyOption,
    HardKeyPeriod,
    HardKeyEnter,
    HardKeyLeftShift,
    HardKeySpace,
    HardKeyAlt,
    HardKeyRightShift,

    HardKeyA,
    HardKeyB,
    HardKeyC,
    HardKeyD,
    HardKeyE,
    HardKeyF,
    HardKeyG,
    HardKeyH,
    HardKeyI,
    HardKeyJ,
    HardKeyK,
    HardKeyL,
    HardKeyM,
    HardKeyN,
    HardKeyO,
    HardKeyP,
    HardKeyQ,
    HardKeyR,
    HardKeyS,
    HardKeyT,
    HardKeyU,
    HardKeyV,
    HardKeyW,
    HardKeyX,
    HardKeyY,
    HardKeyZ,

    HardKeySend,                /* Treo 700p, green button */

    HardKeyZero,                /* Treo 600, 650, 700p */

    /* Virtual keys.  */

    HardKeyEscape,

    HardKeyBacktick,

    HardKeyOne,
    HardKeyTwo,
    HardKeyThree,
    HardKeyFour,
    HardKeyFive,
    HardKeySix,
    HardKeySeven,
    HardKeyEight,
    HardKeyNine,

    HardKeyDash,
    HardKeyEquals,

    HardKeyTab,
    HardKeyLeftSquare,
    HardKeyRightSquare,
    HardKeyBackslash,
    HardKeySemicolon,
    HardKeyApostrophe,
    HardKeyComma,
    HardKeySlash,

} HardKeyCode;

#endif
