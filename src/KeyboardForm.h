/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Keyboard.h
 *
 ***********************************************************************/

#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef enum {
    InvalidScreen   = -1,
    NoScreen        = 0,
    AlphaScreen     = 1,
    NumericScreen   = 2,
    CursorScreen    = 3,
    FunctionScreen  = 4,
    AllScreens      = 5,
} Screen;

void KeyboardInit (void) KEYBOARDFORM_SECTION;
void KeyboardClose (void) KEYBOARDFORM_SECTION;

#endif

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

