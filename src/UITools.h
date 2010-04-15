/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: UITools.h
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
