/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Timer.c
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

#define LOGLEVEL LogDebug

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Timer.h"
#undef PRIVATE_INCLUDE

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Make sure this number of ticks elapses between successive triggers
 * of the same timer.  */
#define TIMER_RESOLUTION    5

#define MAX_TIMERS      10

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct TimerImpl {
    char *name;
    UInt32 time;
    TimerTrigger trigger;
    UInt32 userdata;
} TimerImpl;

typedef struct TimerGlobals {
    Heap heap;
    MsgQueuePtr reinstatequeue;
    UInt32 nexttrigger;
} TimerGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static int TriggerCompare (Globals *g, MemHandle a, MemHandle b)
    TIMER_SECTION;

static void UpdateNextTrigger (Globals *g)
    TIMER_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_heap              g->timer->heap
#define g_nexttrigger       g->timer->nexttrigger
#define g_reinstatequeue    g->timer->reinstatequeue

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/



static int
TriggerCompare (Globals *g, MemHandle a, MemHandle b)
{
    TimerImpl *t1 = MemHandleLock (a);
    TimerImpl *t2 = MemHandleLock (b);
    int result;
    if (t1->time == t2->time)
        result = 0;
    else if (t1->time == 0)
        result = 1;
    else if (t2->time == 0)
        result = -1;
    else
        result = (t1->time > t2->time ? 1 : -1);
    TRACE ("TriggerCompare (%s (%lx), %s (%lx)) = %d",
           t1->name, t1->time,
           t2->name, t2->time, result);
    MemPtrUnlock (t1);
    MemPtrUnlock (t2);
    return result;
}



static void
UpdateNextTrigger (Globals *g)
{
    MemHandle h;
    TimerImpl *t;
    h = HeapGet (g_heap);
    if (h == 0) {
        g_nexttrigger = 0;
    }
    else {
        t = MemHandleLock (h);
        g_nexttrigger = t->time;
        MemPtrUnlock (t);
    }
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/



void
f_TimerInit (Globals *g)
{
    if (g->timer)
        f_TimerClose (g);
    g->timer = CheckedMemPtrNew (sizeof (TimerGlobals), false);
    g_nexttrigger = 0;
    g_reinstatequeue = MsgNewQueue ("Timer reinstate", MAX_TIMERS,
                                    false, true, NULL);
    g_heap = HeapNew ("Timer", MAX_TIMERS, TriggerCompare);
    DEBUG ("TimerInit");
}



void
f_TimerClose (Globals *g)
{
    if (!g->timer)
        return;
    DEBUG ("Closing Timer");
    if (g_heap) {
        ASSERT (HeapGet (g_heap) == 0);
        HeapFree (g_heap);
        g_heap = 0;
    }
    if (g_reinstatequeue) {
        MsgFreeQueue (g_reinstatequeue);
        g_reinstatequeue = 0;
    }
    g_nexttrigger = 0;
}



UInt32
f_TimerTimeout (Globals *g)
{
    UInt32 ticks = TimGetTicks();
    if (!g->timer || !g_nexttrigger)
        return evtWaitForever;
    else if (g_nexttrigger > ticks)
        return g_nexttrigger - ticks;
    else
        return 0;
}



void
f_TimerTick (Globals *g)
{
    MemHandle h;
    TimerImpl *t;
    /* Save the ticks count once and trigger timers that waited for
     * this one only.  Later timers will get a chance to run at the
     * next TimerTick invocation.  */
    UInt32 ticks = TimGetTicks();
    if (!g->timer)
        return;

    if (g_nexttrigger == 0 || g_nexttrigger > ticks)
        return;

    ASSERT (ticks);
    ASSERT (!MsgAvailable (g_reinstatequeue));

    while (g_nexttrigger && g_nexttrigger <= ticks) {
        /* Trigger the next timer.  */
        h = HeapRemove (g_heap);
        ASSERT (h);
        MsgAddHandle (g_reinstatequeue, h);

        UpdateNextTrigger (g);
    }

    while (MsgAvailable (g_reinstatequeue)) {
        h = MsgGetAndProcess (g_reinstatequeue);
        t = MemHandleLock (h);
        TRACE ("Triggering %s timer at %lx", t->name, ticks);
        ASSERT (t->time);
        ASSERT (t->time <= ticks);
        ASSERT (t->trigger);
        t->time = t->trigger (g, t->userdata);
        if (t->time) {
            t->time = MAX (t->time, TimGetTicks() + TIMER_RESOLUTION);
            TRACE ("Reinstating %s timer to run at %lx",
		   t->name, t->time);
        }
        else {
            TRACE ("Canceling %s timer", t->name);
        }
        MemPtrUnlock (t);
        HeapAdd (g_heap, h);
    }

    UpdateNextTrigger (g);
}



Timer
f_TimerNew (Globals *g, char *name, UInt32 ticks,
            TimerTrigger trigger, UInt32 userdata)
{
    MemHandle h;
    TimerImpl *t;
    if (!g->timer)
        return 0;
    h = CheckedMemHandleNew (sizeof (TimerImpl), true);
    ASSERT (name);
    ASSERT (trigger);
    DEBUG ("Adding %s timer (%lx) to run at %lx",
	   name, (UInt32) h, ticks);

    t = MemHandleLock (h);
    t->name = name;
    t->time = ticks;
    t->trigger = trigger;
    t->userdata = userdata;
    MemPtrUnlock (t);
    HeapAdd (g_heap, h);

    UpdateNextTrigger (g);
    return h;
}


void
f_TimerFree (Globals *g, Timer timer)
{
    if (!g->timer)
        return;
    ASSERT (timer);
    HeapDelete (g_heap, timer);
    MemHandleFree (timer);
    UpdateNextTrigger (g);
}



void
f_TimerCancel (Globals *g, Timer timer)
{
    f_TimerReschedule (g, timer, 0);
}



void
f_TimerReschedule (Globals *g, Timer timer, UInt32 ticks)
{
    TimerImpl *t;
    if (!g->timer)
        return;
    ASSERT (timer);
    t = MemHandleLock (timer);
    TRACE ("%s timer rescheduled to run at %lx", t->name, ticks);
    t->time = ticks;
    MemPtrUnlock (t);
    HeapChange (g_heap, timer);
    UpdateNextTrigger (g);
}
