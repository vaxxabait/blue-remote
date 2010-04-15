/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: ProgressForm.h
 *
 **********************************************************************/

#undef SECTION
#define SECTION PROGRESSFORM_SECTION

#ifdef NORMAL_INCLUDE
typedef enum {
    AnimationScanning,
    AnimationChannelForward,
    AnimationChannelBackward,
    AnimationPulsing,
} Animation;

typedef void (*ProgressCallback) (Globals *g);

void f_ProgressInit (Globals *g);
#endif

DEFUN0 (void, ProgressClose);

/* Display a progress dialog with the given message.  */
DEFUNP (1, 2, void, ProgressStart, char *title);

/* Close the currently displayed progress dialog.  */
DEFUN1 (void, ProgressStop, Boolean wait);

/* Update progress dialog text and status.
   Returns true if the user pressed cancel.  */
DEFUNP (2, 3, Boolean, ProgressUpdate, UInt8 severity, char *format);

DEFUN3 (Boolean, ProgressVUpdate, UInt8 severity, char *format, _Palm_va_list args);

DEFUNP (1, 2, void, ProgressSetDetail, char *detail);
DEFUN2 (void, ProgressSetDetailV, char *detail, _Palm_va_list args);

DEFUN1 (void, ProgressSelectAnimation, Animation a);

/* True if a progress dialog is being displayed.  */
DEFUN0 (Boolean, Progressing);

/* True if the user pressed cancel in the current progress dialog.  */
DEFUN0 (Boolean, ProgressCanceled);

/* Call PrgHandleEvent on the progress object.  */
DEFUN1 (Boolean, ProgressHandleEvent, EventType *event);

/* Enable or disable cancel button.  */
DEFUN1 (Boolean, ProgressEnableCancel, Boolean enable);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

