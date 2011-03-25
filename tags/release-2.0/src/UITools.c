/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: UITools.c
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

#define LOGLEVEL LogTrace

#include "Common.h"

#include <FrmGlue.h>

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "UITools.h"
#undef PRIVATE_INCLUDE

#include "BlueRemote_res.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define PROGRESS_ALWAYS_DELAY 0

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static Boolean PopupFormHandleEvent (EventType *e)
    UITOOLS_SECTION;

static void DrawHeaderPlusText (Globals *g, RectangleType *bounds,
				char *header, char *text)
    UITOOLS_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static Boolean
PopupFormHandleEvent (EventType *e) 
{
    return false;
}


static void
DrawHeaderPlusText (Globals *g, RectangleType *bounds,
		    char *header, char *text)
{
    RectangleType b = *bounds;
    if (header) {
	FontID old = FntSetFont (largeBoldFont);
	b.extent.y = FntLineHeight();
	DrawText (&b, JustifyLeft, header, true);
	b.topLeft.y += FntLineHeight() + 2;
	b.extent.y = bounds->extent.y - FntLineHeight() - 2;
	FntSetFont (old);
    }
    DrawText (&b, JustifyLeft, text, true);
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

/* Handle popSelectEvent from the form selection popup.  */
Boolean
f_HandleFormPopSelectEvent (Globals *g, EventType *pEvent)
{
    FormDesc *fd;

    if (pEvent->eType != popSelectEvent)
        return false;

    /* Has the selection changed at all?  */
    if (pEvent->data.popSelect.selection
        == pEvent->data.popSelect.priorSelection)
        return true;

    /* Find selected form and switch to it.  */
    for (fd = g_formList; fd != NULL; fd = fd->next) {
        if (fd->popupIndex == pEvent->data.popSelect.selection) {
            SwitchToForm (fd->formID);
            return true;
        }
    }
    return false;
}


UInt16
f_DrawText (Globals *g,
            RectangleType *bounds,
            Justification just,
            char *text,
	    Boolean draw)
{
    UInt16 length = StrLen (text); /* Remaining length. */
    UInt16 linelength;             /* Length of this line in bytes. */
    Coord linewidth;               /* Width of this line in pixels. */
    /* Draw the next line at this y coordinate. */
    Coord y = bounds->topLeft.y;
    /* Number of lines that can be safely drawn.  */
    Coord remaininglines = bounds->extent.y / FntLineHeight();
    UInt16 lines = 0;
    UInt16 tmp;
    Boolean fit;

    while (length && remaininglines) {
	/* Skip over whitespace at the beginning of line.  */
	while (*text == ' ' || *text == '\t') {
	    text++;
	    length--;
	}

	linewidth = bounds->extent.x;
        linelength = length;

	
        FntCharsInWidth (text, &linewidth, &linelength, &fit);
        if (linelength == 0) {
            /* Not a single character fitted.  */
	    if (*text == '\n') {
		y += 3;
		text++;
		length--;
		continue;
	    }
	    else break;
	}   

        /* Get the number of characters in full words.  */
        tmp = FldWordWrap (text, bounds->extent.x);
        if (tmp != 0)
            linelength = MIN (tmp, linelength);

        if (remaininglines == 1 && linelength != length) {
            linelength = length; /* Last line. */

	    if (draw)
		WinDrawTruncChars (text, linelength,
				   bounds->topLeft.x, y,
				   bounds->extent.x);
        }
        else if (draw) {
            Int16 width = FntCharsWidth (text, linelength);
            Int16 indent;
            switch (just) {
		case JustifyLeft:
		default:
		    indent = 0;
		    break;
		case JustifyRight:
		    indent = bounds->extent.x - width;
		    break;
		case JustifyCenter:
		    indent = (bounds->extent.x - width) / 2;
		    break;
            }
            WinDrawChars (text, linelength,
                          bounds->topLeft.x + indent, y);
        }
        length -= linelength;
        text += linelength;
        y += FntLineHeight();
        remaininglines--;
        lines++;
    }
    return y - bounds->topLeft.y;
}


int
f_PopupForm (Globals *g, char *title, Boolean fullheight,
	     char *header, char *button1, char *button2,
	     char *format, ...)
{
    _Palm_va_list args;
    FormType *f;
    Coord height;
    Coord width;
    ControlType *b1 = NULL;
    ControlType *b2 = NULL;
    char *text = CheckedMemPtrNew (512, false);
    RectangleType textbounds = { { 8, 19 }, { 140, 119 } };
    FormActiveStateType savedstate;
    EventType event;
    int pressed;
    
    va_start (args, format);
    StrVPrintF (text, format, args);
    va_end (args);

    /* Calculate height of form.  */
    if (fullheight)
	height = 156;
    else
	height = 44 + DrawText (&textbounds, JustifyLeft, text, false);
    if (header) {
	FontID old = FntSetFont (largeBoldFont);
	height += FntLineHeight();
	FntSetFont (old);
    }
    if (height > 156) height = 156;

    /* Create the form.  */
    FrmSaveActiveState (&savedstate);
    f = FrmNewForm (1042, title, 2, 158 - height,
		    156, height, true, 1, 0, 0);
    width = FntCharsWidth (button1, StrLen (button1)) + 16;
    b1 = CtlNewControl ((void **) &f, 1, buttonCtl, button1,
			5, height - 17, width, 12, stdFont, 0, true);
    if (button2) {
	Coord x = 5 + width + 10;
	width = FntCharsWidth (button2, StrLen (button2)) + 16;
	b2 = CtlNewControl ((void **)&f, 2, buttonCtl, button2,
			     x, height - 17, width, 12, stdFont, 0, true);
    }
    FrmSetEventHandler (f, PopupFormHandleEvent);
    FrmSetActiveForm (f);
    FrmDrawForm (f);

    /* Set up navigation info.  */
    if (FrmGlueNavIsSupported()) {
	FrmNavHeaderType header;
	FrmNavOrderEntryType navorder[2];
	header.version = kFrmNavInfoVersion;
	header.numberOfObjects = (button2 ? 2 : 1);
	header.headerSizeInBytes = sizeof (FrmNavHeaderType);
	header.listElementSizeInBytes = sizeof (FrmNavOrderEntryType);
	header.navFlags = kFrmNavHeaderFlagsObjectFocusStartState;
	header.initialObjectIDHint = 1;
	header.jumpToObjectIDHint = 0;
	header.bottomLeftObjectIDHint = 1;
	navorder[0].objectID = 1;
	navorder[0].objectFlags = 0;
	navorder[0].aboveObjectID = 0;
	navorder[0].belowObjectID = 0;
	navorder[1].objectID = 2;
	navorder[1].objectFlags = 0;
	navorder[1].aboveObjectID = 0;
	navorder[1].belowObjectID = 0;
	FrmSetNavState (f, kFrmNavStateFlagsObjectFocusMode);
	FrmSetNavOrder (f, &header, navorder);
    }
    FrmGlueNavObjectTakeFocus (f, FrmGetObjectIndexFromPtr (f, b1));

    /* Draw the text.  */
    DrawHeaderPlusText (g, &textbounds, header, text);
    
    pressed = 0;
    do {
	EvtGetEvent (&event, evtWaitForever);
	if (SysHandleEvent (&event))
	    continue;

	switch (event.eType) {
	    case appStopEvent:
		ENQUEUE (CmdExitApp);
		pressed = -1;
		continue;
	    case ctlSelectEvent:
		switch (event.data.ctlSelect.controlID) {
		    case 1:
			pressed = 1;
			continue;
		    case 2:
			pressed = 2;
			continue;
		    default:
			break;
		}
	    case frmUpdateEvent:
		FrmDrawForm (f);
		DrawHeaderPlusText (g, &textbounds, header, text);
		continue;
		
	    default:
		break;
	}

	if (FrmDispatchEvent (&event))
	    continue;
	
    } while (pressed == 0);

    FrmEraseForm (f);
    FrmRestoreActiveState (&savedstate);
    FrmDeleteForm (f);
    MemPtrFree (text);
    EvtFlushPenQueue();
    return pressed;
}
