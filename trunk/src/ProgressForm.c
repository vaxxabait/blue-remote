/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: ProgressForm.c
 *
 ***********************************************************************/

#define LOGLEVEL LogTrace

#include "Common.h"

#include "BlueRemote_res.h"
#include "AppMenu.h"
#include "ProgressForm.h"
#include "HardKeys.h"

#define g globals

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Number of ticks to wait between successive animation states.  */
#define ROLL_DELAY  10

#define PHASES  5
#define BITMAPS 7

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

static void DrawTitle (void)
    PROGRESSFORM_SECTION;

static void DrawDetail (void)
    PROGRESSFORM_SECTION;

static void DrawDevname (void)
    PROGRESSFORM_SECTION;

static void DrawBitmaps (void)
    PROGRESSFORM_SECTION;

static void DrawMsg (void)
    PROGRESSFORM_SECTION;

static void Redraw (void)
    PROGRESSFORM_SECTION;

static void StartAnimate (void)
    PROGRESSFORM_SECTION;

static UInt32 AnimateTimer (Globals *g, UInt32 count)
    PROGRESSFORM_SECTION;

static void StopAnimate (Boolean wait)
    PROGRESSFORM_SECTION;

static Boolean ProgressFormHandleEvent (EventType* pEvent)
    PROGRESSFORM_SECTION;

static void ProgressInitForm (struct FormDesc *pFormDesc, FormType *pForm)
    PROGRESSFORM_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

static FormDesc progressFormDesc = {
    NULL,
    ProgressForm,
    65535,
    0,
    ProgressInitForm
};

static Timer timer;

static Boolean progressing = false;
static FormType *progressform = NULL;
static FormActiveStateType savedstate;



static Boolean needsRedraw = true;

static Boolean cancelEnable = true;
static Boolean canceled = false;

/* Animation state.  */
static Animation animType = AnimationScanning;
static int direction = 1;    /* 1: Left to right  -1: Right to left */
static int animLength = 4;
static int animNextChange = 4;

#if 0
static BitmapType *phone;
static Boolean phoneVisible = false;
static RectangleType phoneBounds = { { 13, 96 }, { 0, 0 } };

static BitmapType *notebook;
static Boolean notebookVisible = false;
static RectangleType notebookBounds = { { 0, 96 }, { 0, 0 } };
#endif

static BitmapType *bitmaps[PHASES];
static int bitmapState[BITMAPS] = { 0, 1, 0, -1, -1, -1, -1 };

/* Texts.  */
char ptitle[100];
char pdetail[300];
char pmsg[300];

RectangleType titleBounds = { { 8, 19 }, { 140, 20} };
RectangleType detailBounds = { { 8, 40 }, { 140, 54 } };
RectangleType devnameBounds = { { 8, 80 }, { 140, 0 } };
RectangleType bitmapBounds = { { 8, 96 }, { 140, 20 } };
RectangleType msgBounds = { { 8, 120 }, { 140, 15 } };

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


static void
DrawTitle (void)
{
    FontID old;
    if (!progressing) {
        needsRedraw = true;
        return;
    }
    old = FntSetFont (largeBoldFont);
    WinEraseRectangle (&titleBounds, 0);
    DrawText (&titleBounds, JustifyLeft, ptitle, true);
    FntSetFont (old);
}



static void
DrawDetail (void)
{
    FontID old;
    if (!progressing) {
        needsRedraw = true;
        return;
    }
    old = FntSetFont (stdFont);
    WinEraseRectangle (&detailBounds, 0);
    DrawText (&detailBounds, JustifyLeft, pdetail, true);
    FntSetFont (old);
}



static void
DrawDevname (void)
{
    FontID old;
    Device *d;
    return;

    if (!progressing) {
        needsRedraw = true;
        return;
    }
    d = DevListSelected();
    WinEraseRectangle (&devnameBounds, 0);
    if (d) {
        old = FntSetFont (boldFont);
        DrawText (&devnameBounds, JustifyCenter, d->nameStr, true);
        FntSetFont (old);
    }
}



static void
DrawBitmaps (void)
{
    int i;
    UInt16 x = bitmapBounds.topLeft.x;
    UInt16 y = bitmapBounds.topLeft.y;

    if (!progressing) {
        needsRedraw = true;
        return;
    }

#if 0
    if (phoneVisible)
        WinDrawBitmap (phone, x, y);
    else
        WinEraseRectangle (&phoneBounds, 0);

    x += phoneBounds.extent.x + 2;
    y += 2;                     /* The phone antenna sticks out. */
#endif

    for (i = 0; i < BITMAPS; i++) {
        PANICIF (bitmapState[i] >= PHASES, "Corrupt bitmap state");
        if (bitmapState[i] >= 0)
            WinDrawBitmap (bitmaps[bitmapState[i]], x, y);
        else {
            RectangleType r = { { x, y }, { 20, 20 } };
            WinEraseRectangle (&r, 0);
        }
        x += 20;
    }
#if 0
    x += 2;
    if (notebookVisible)
        WinDrawBitmap (notebook, x, y);
    else
        WinEraseRectangle (&notebookBounds, 0);
#endif

}



static void
DrawMsg (void)
{
    FontID old;
    if (!progressing) {
        needsRedraw = true;
        return;
    }
    old = FntSetFont (stdFont);
    WinEraseRectangle (&msgBounds, 0);
    DrawText (&msgBounds, JustifyCenter, pmsg, true);
    FntSetFont (old);
}



static void
Redraw (void)
{
    if (!progressing) {
        needsRedraw = true;
        return;
    }
    DrawTitle();
    DrawDetail();
    DrawDevname();
    DrawBitmaps();
    DrawMsg();
    needsRedraw = false;
}


static void
StartAnimate (void)
{
    animNextChange = animLength;
    switch (animType) {
	case AnimationScanning:
	{
	    int i;
	    for (i = 0; i < BITMAPS; i++) {
		bitmapState[i] = -1;
	    }
	    bitmapState[BITMAPS / 2 - 1] = 0;
	    bitmapState[BITMAPS / 2] = 1;
	    bitmapState[BITMAPS / 2 + 1] = 0;
	    animLength = 4;
	    animNextChange = animLength - BITMAPS / 2 + 1;
	    direction = 1;
	    break;
	}
	case AnimationChannelForward:
	case AnimationChannelBackward:
	{
	    int i, j = 0, dir = 1;
	    for (i = 0; i < BITMAPS; i++, j += dir) {
		bitmapState[i] = j;
		if (j == 2)
		    dir = -1;
		if (j == -1)
		    dir = 1;
	    }
	    animLength = 0;
	    direction = (animType == AnimationChannelForward ? 1 : -1);
	    break;
	}
	case AnimationPulsing:
	    animLength = 0;
	    direction = 1;
	    break;
    }
    TimerReschedule (timer, TimGetTicks() + ROLL_DELAY);
}



static UInt32
AnimateTimer (Globals *g, UInt32 userdata)
{
    UInt16 t;
    int i;
    Boolean found;
    ASSERT (progressing);

    switch (animType) {
	case AnimationScanning:
	case AnimationChannelForward:
	case AnimationChannelBackward:
	    if (direction == 1) {
		t = bitmapState[BITMAPS - 1];
		for (i = BITMAPS - 1; i > 0; i--)
		    bitmapState[i] = bitmapState[i - 1];
		bitmapState[0] = t;
	    }
	    else {
		t = bitmapState[0];
		for (i = 0; i < BITMAPS - 1; i++)
		    bitmapState[i] = bitmapState[i + 1];
		bitmapState[BITMAPS - 1] = t;
	    }
	    break;
	case AnimationPulsing:
	    found = false;
	    if (direction == 1) {
		for (i = 0; i < BITMAPS; i++) {
		    if (bitmapState[i] < PHASES - 1)  {
			bitmapState[i]++;
			found = true;
		    }
		}
	    }
	    else {
		for (i = 0; i < BITMAPS; i++) {
		    if (bitmapState[i] > 0)  {
			bitmapState[i]--;
			found = true;
		    }
		}
	    }
	    if (!found && animLength == 0)
		direction *= -1;
	    break;
    }

    if (animLength > 0) {
        animNextChange--;
        if (animNextChange <= 0) {
            direction *= -1;
            animNextChange = animLength;
        }
    }
    DrawBitmaps();
    return TimGetTicks() + ROLL_DELAY;
}



static void
StopAnimate (Boolean wait)
{
    int i, j;
    Boolean changed = true;
    if (wait) {
	UInt16 idx = FrmGetObjectIndex (progressform, ProgressCancelButton);
	CtlHideControl (FrmGetObjectPtr (progressform, idx));
	while (changed) {
	    changed = false;
	    for (i = 0; i < BITMAPS; i++) {
		if (bitmapState[i] != PHASES - 1) {
		    bitmapState[i]++;
		    changed = true;
		}
	    }
	    DrawBitmaps();
	    SysTaskDelay (ROLL_DELAY / 2);
	}
	for (j = PHASES - 2; j >= 0; j--) {
	    for (i = 0; i < BITMAPS; i++)
		bitmapState[i] = j;
	    DrawBitmaps();
	    SysTaskDelay (ROLL_DELAY / 2);
	}
	WinEraseRectangle (&bitmapBounds, 0);
	SysTaskDelay (0 * ROLL_DELAY * 2);
    }
    TimerCancel (timer);
}



static Boolean
ProgressFormHandleEvent (EventType* e)
{
    FormType *f = FrmGetActiveForm();

    if (needsRedraw)
        Redraw();

    switch (e->eType) {
	case frmOpenEvent:
	    TRACE ("frmOpenEvent");
	    /* Setup is handled by ProgressStart.  */
	    return true;

	case frmUpdateEvent:
	    FrmDrawForm (f);
	    Redraw();
	    return true;

#if 0
	case winExitEvent:
	    if (e->data.winExit.exitWindow
		== (WindowType *)FrmGetFormPtr (ProgressForm))
		enableDraw = false;
	    return false;

	case winEnterEvent:
	    if (e->data.winEnter.enterWindow
		== (WindowType *)FrmGetFormPtr (ProgressForm)
		&& (e->data.winEnter.enterWindow
		    == (WindowType *)FrmGetFirstForm()))
		enableDraw = true;
	    return false;
#endif

	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		case ProgressCancelButton:
		    CtlHideControl (e->data.ctlSelect.pControl);
		    if (cancelEnable) {
			canceled = true;
			ENQUEUE (CmdProgressCanceled);
			return true;
		    }
		    else {
			return true;
		    }
		default:
		    return false;
	    }

	default:
	    return false;
    }
}



static void
ProgressInitForm (struct FormDesc *pFormDesc, FormType *f)
{
    /* Set the event handler.  */
    FrmSetEventHandler (f, ProgressFormHandleEvent);
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
f_ProgressInit (Globals *g)
{
    MemHandle h;
    RegisterForm (&progressFormDesc);

#if 0
    h = DmGetResource (bitmapRsc, PhoneBitmap);
    PANICIF (!h, "Resource not found");
    phone = MemHandleLock (h);
    BmpGetDimensions (phone, &phoneBounds.extent.x,
                      &phoneBounds.extent.y, NULL);

    h = DmGetResource (bitmapRsc, NotebookBitmap);
    PANICIF (!h, "Resource not found");
    notebook = MemHandleLock (h);
    BmpGetDimensions (phone, &notebookBounds.extent.x,
                      &notebookBounds.extent.y, NULL);
    notebookBounds.topLeft.x =
        phoneBounds.topLeft.x
        + phoneBounds.extent.x
        + 5 * 20 + 4;
#endif

    h = DmGetResource (bitmapRsc, Progress0Bitmap);
    PANICIF (!h, "Resource not found");
    bitmaps[0] = MemHandleLock (h);
    h = DmGetResource (bitmapRsc, Progress1Bitmap);
    PANICIF (!h, "Resource not found");
    bitmaps[1] = MemHandleLock (h);
    h = DmGetResource (bitmapRsc, Progress2Bitmap);
    PANICIF (!h, "Resource not found");
    bitmaps[2] = MemHandleLock (h);
    h = DmGetResource (bitmapRsc, Progress3Bitmap);
    PANICIF (!h, "Resource not found");
    bitmaps[3] = MemHandleLock (h);
    h = DmGetResource (bitmapRsc, Progress4Bitmap);
    PANICIF (!h, "Resource not found");
    bitmaps[4] = MemHandleLock (h);
#if PHASES != 5
#error Check ProgressInit!
#endif

    timer = TimerNew ("Progress animator", 0, AnimateTimer, 0);
}


void
f_ProgressClose (Globals *g)
{
    MemHandle h;
    int i;

    f_ProgressStop (g, false);

    for (i = 0; i < PHASES; i++) {
        if (bitmaps[i]) {
            h = MemPtrRecoverHandle (bitmaps[i]);
            MemHandleUnlock (h);
            DmReleaseResource (h);
            bitmaps[i] = 0;
        }
    }

#if 0
    if (phone) {
        h = MemPtrRecoverHandle (phone);
        MemHandleUnlock (h);
        DmReleaseResource (h);
        phone = 0;
    }

    if (notebook) {
        h = MemPtrRecoverHandle (notebook);
        MemHandleUnlock (h);
        DmReleaseResource (h);
        notebook = 0;
    }
#endif

    if (timer) {
        TimerFree (timer);
        timer = 0;
    }
}


Boolean
f_ProgressUpdate (Globals *g, UInt8 severity, char *format, ...)
{
    _Palm_va_list args;

    va_start (args, format);
    ProgressVUpdate (severity, format, args);
    va_end (args);

    return canceled;
}

Boolean
f_ProgressVUpdate (Globals *g, UInt8 severity,
                   char *format, _Palm_va_list args)
{
    StrVPrintF (pmsg, format, args);
    if (severity >= LogError) {
        FrmCustomAlert (ErrorAlert, pmsg, NULL, NULL);
    }
    else if (severity >= LogWarn) {
        FrmCustomAlert (WarningAlert, pmsg, NULL, NULL);
    }
    else {
	DrawMsg();
    }
    return canceled;
}

Boolean
f_Progressing (Globals *g)
{
    return progressing;
}

Boolean
f_ProgressCanceled (Globals *g)
{
    return canceled;
}

void
f_ProgressStart (Globals *g, char *title, ...)
{
    _Palm_va_list args;
    UInt16 idx;

    va_start (args, title);
    StrVPrintF (ptitle, title, args);
    va_end (args);

    pdetail[0] = 0;
    pmsg[0] = 0;
    DEBUG ("Progress start: %s", ptitle);

    canceled = false;
    if (!progressing) {
        progressing = true;
	FrmSaveActiveState (&savedstate);
	progressform = FrmInitForm (ProgressForm);
	FrmSetEventHandler (progressform,  ProgressFormHandleEvent);
	FrmSetActiveForm (progressform);
	FrmDrawForm (progressform);
	idx = FrmGetObjectIndex (progressform, ProgressCancelButton);
	CtlShowControl (FrmGetObjectPtr (progressform, idx));
        animType = AnimationScanning;
        StartAnimate();
    }
    Redraw();
    HardKeyEnable (false);
    AutoOffEnable (false);

    /* Discard any queued pen and key events; we don't want them to
       press buttons etc. on the progress form.  */
    EvtFlushPenQueue();
    EvtFlushKeyQueue();
}

void
f_ProgressStop (Globals *g, Boolean wait)
{
    if (!progressing)
        return;

    DEBUG ("Progress stopped");
    StopAnimate (wait);
    progressing = false;
    FrmEraseForm (progressform);
    FrmRestoreActiveState (&savedstate);
    FrmDeleteForm (progressform);
    progressform = NULL;
    AutoOffEnable (true);

    /* Discard any queued pen events; we don't want them to press
       buttons etc. on the restored form.  */
    EvtFlushPenQueue();
}


void
f_ProgressSetDetail (Globals *g, char *detail, ...)
{
    _Palm_va_list args;

    va_start (args, detail);
    f_ProgressSetDetailV (g, detail, args);
    va_end (args);
}

void
f_ProgressSetDetailV (Globals *g, char *detail, _Palm_va_list args)
{
    StrVPrintF (pdetail, detail, args);
    DEBUG ("Progress detail: %s", pdetail);
    DrawDetail();
}


void
f_ProgressSelectAnimation (Globals *g, Animation a)
{
    animType = a;
    StartAnimate();
}

Boolean
f_ProgressHandleEvent (Globals *g, EventType *event)
{
    // TODO
    return SysHandleEvent (event);
}


Boolean
f_ProgressEnableCancel (Globals *g, Boolean enable)
{
    Boolean old = cancelEnable;
    FormType *f;
    UInt16 idx;
    ASSERT (progressing);
    cancelEnable = enable;
    f = FrmGetActiveForm();
    idx = FrmGetObjectIndex (f, ProgressCancelButton);
    ASSERT (idx != frmInvalidObjectId);
    if (cancelEnable)
	FrmShowObject (f, idx);
    else
	FrmHideObject (f, idx);
    return old;
}


/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

