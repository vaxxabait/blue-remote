/***********************************************************************
 *
 * Copyright (c) 1999-2004 PalmSource, Inc. All rights reserved.
 *
 * File: UITools.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION UITOOLS_SECTION

#ifdef NORMAL_INCLUDE
typedef enum {
    JustifyLeft,
    JustifyRight,
    JustifyCenter
} Justification;
#endif

DEFUN1 (Boolean, HandleFormPopSelectEvent, EventType *pEvent);

DEFUNN (UInt16, DrawText, RectangleType *bounds, Justification just, char *text, Boolean draw);

DEFUNP (6, 7, int, PopupForm, char *title, Boolean fullheight, char *header, char *button1, char *button2, char *format);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

