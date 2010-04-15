/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: AutoOff.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION AUTOOFF_SECTION

#ifdef NORMAL_INCLUDE
void f_AutoOffInit (Globals *g) SECTION;
#endif

DEFUN0 (void, AutoOffClose);
DEFUN1 (Boolean, AutoOffEnable, Boolean enable);
