/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: HardKeys.h
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

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

