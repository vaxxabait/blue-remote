/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Mouse.h
 *
 ***********************************************************************/

#ifndef MOUSE_H
#define MOUSE_H

Boolean MouseFormOverrideEvent (EventType *pEvent)
    MOUSEFORM_SECTION;

void MouseInit (void) MOUSEFORM_SECTION;
void MouseClose (void) MOUSEFORM_SECTION;

#endif

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

