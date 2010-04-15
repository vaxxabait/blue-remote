/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Heap.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION TIMER_SECTION

#ifdef NORMAL_INCLUDE

typedef UInt32 (*TimerTrigger) (Globals *g, UInt32 userdata);
typedef MemHandle Timer;

void f_TimerInit (Globals *g) TIMER_SECTION;

#endif  /* NORMAL_INCLUDE */

DEFUN0 (void, TimerClose);

DEFUN0 (void, TimerTick);
DEFUN0 (UInt32, TimerTimeout);
DEFUNN (Timer, TimerNew, char *name, UInt32 ticks, TimerTrigger trigger, UInt32 userdata);
DEFUN1 (void, TimerFree, Timer timer);

DEFUN1 (void, TimerCancel, Timer t);
DEFUN2 (void, TimerReschedule, Timer t, UInt32 ticks);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

