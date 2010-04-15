/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Main.h
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
#define SECTION MAIN_SECTION

#ifdef NORMAL_INCLUDE
typedef struct FormDesc {
    /* Pointer to the next form in the formList.  */
    struct FormDesc *next;

    /* The ID of this form.  */
    UInt16 formID;

    /* The index of this form in the form selector popup list, or 65535 if not listed.  */
    UInt16 popupIndex;

    /* The menu item that should select this form, or 0 if none. */
    UInt16 menuCommandID;

    /* Function to initialize this form.  */
    void (*Init) (struct FormDesc *pFormDesc, FormType *pForm);
} FormDesc;

extern Globals *globals;

#elif defined (GLOBALS_INCLUDE)

struct FormDesc *formList;
#define g_formList  g->formList

struct FormDesc *currentForm;
#define g_currentForm   g->currentForm

Int32 eventTimeout;
#define g_eventTimeout  g->eventTimeout

#endif

DEFUN1 (void, RegisterForm, struct FormDesc *form);
DEFUN1 (void, SwitchToForm, UInt16 formID);
DEFUN1 (void, FormDrawn, UInt16 formID);
