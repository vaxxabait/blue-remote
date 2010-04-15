/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Heap.h
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
