/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: MsgQueue.h
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
#define SECTION MSGQUEUE_SECTION

#ifdef NORMAL_INCLUDE

typedef struct MsgQueue *MsgQueuePtr;
typedef void (*MsgQueueCallback) (struct Globals *g,
                                  MsgQueuePtr queue,
                                  Boolean full);

#endif  /* NORMAL_INCLUDE */

/* Create a new message queue of size SIZE.
 * If GROWING is true, then the queue can grow if it gets full.
 * If CALLBACK is non-NULL, then it will be called when a new element
 * is put in the queue and when the queue gets full (before growing).  */
DEFUNN (MsgQueuePtr, MsgNewQueue, char *name, UInt16 size, Boolean growing, Boolean handles, MsgQueueCallback callback);

/* Free message queue QUEUE.  */
DEFUN1 (void, MsgFreeQueue, MsgQueuePtr queue);

/* Adds HANDLE to Q as a new message.  The data is not copied; the
   handle is put in the message queue directly.  */
DEFUN2 (MemHandle, MsgAddHandle, MsgQueuePtr q, MemHandle handle);

/* Adds LENGTH bytes at DATA to the Q queue as a new message.
   Makes a copy of the data.  */
DEFUN3 (MemHandle, MsgAddPtr, MsgQueuePtr q, UInt8 *data, UInt16 length);

/* Returns the number of messages in the queue.  */
DEFUN1 (UInt16, MsgAvailable, MsgQueuePtr q);

/* Get the oldest message in the queue.  Returns 0 if there are no
 * messages in the queue.  */
DEFUN1 (MemHandle, MsgGet, MsgQueuePtr q);

/* Get the oldest message in the queue, and process it immediately.
   Returns 0 if there are no messages in the queue.  */
DEFUN1 (MemHandle, MsgGetAndProcess, MsgQueuePtr q);

/* Returns the last message in the queue, but does not delete it.  */
DEFUN1 (MemHandle, MsgLast, MsgQueuePtr q);

/* Return true if H is still in the queue.  */
DEFUN2 (Boolean, MsgIsInQueue, MsgQueuePtr q, MemHandle h);

/* Signal that H has been processed.  H must have been previously
 * removed.  */
DEFUN2 (void, MsgProcessed, MsgQueuePtr q, MemHandle h);

/* Return true if H has been removed from the queue and processed.  */
DEFUN2 (Boolean, MsgIsProcessed, MsgQueuePtr q, MemHandle h);

/* Delete all waiting messages in the queue.  Note that handles that
 * are being processed are not deleted.  Flushed handled are
 * processed immediately by MsgFlush.  */
DEFUN1 (void, MsgFlush, MsgQueuePtr q);
