/***********************************************************************
 *
 * Copyright (c) 1999-2004 PalmSource, Inc. All rights reserved.
 *
 * File: MsgQueue.c
 *
 ***********************************************************************/

#define LOGLEVEL LogDebug

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "MsgQueue.h"
#undef PRIVATE_INCLUDE

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* No queue may contain more than this number of messages.  */
#define MAX_QUEUE_SIZE          32000

/* If a queue runs out of space, it gets its new size using this
 * macro.  */
#define QUEUE_GROWTH(size)      MIN (((size) * 3 / 2), MAX_QUEUE_SIZE)


#define INC(val) val = ((val) == q->bufsize - 1 ? 0 : (val) + 1)
#define DEC(val) val = ((val) == 0 ? q->bufsize - 1 : (val) - 1)

#define ADD(a,b) ((a) + (b) >= q->bufsize	\
                  ? (a) + (b) - q->bufsize	\
                  : (a) + (b))

#define SUBTRACT(a,b) ((a) >= (b)			\
                       ? (a) - (b)			\
                       : q->bufsize - (b) + (a))

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

/*
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |  |  |  |PP|PP|PP|PP|QQ|QQ|QQ|QQ|QQ|QQ|QQ|  |  |  |  |  |  |  |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *           ^ process   ^ front              ^ end
 *
 * PP: Client has retrieved data, but still processing it.
 *     These handles may be invalid.
 *
 * QQ: Data still in queue.  Handles should be valid.
 */
typedef struct MsgQueue {
    /*  Array of MemHandles.  */
    MemHandle buf;

    /* Size of the buffer, in number of messages.  */
    UInt16 bufsize;

    /* The name of the queue.  Useful for debugging.  */
    char *name;

    /* Start of Processed data.  */
    UInt16 process;

    /* Start of Queued data.  */
    UInt16 front;

    /* Next free slot.  */
    UInt16 end;

    /* Number of Processed and Queued entries.  */
    UInt16 processing;

    /* Number of Queued data.  */
    UInt16 waiting;

    /* Callback function.  */
    MsgQueueCallback callback;

    /* True if queue is locked.  Used to prevent recursion from
     * messing up the queue. */
    Boolean locked;

    /* True if queue may grow.  */
    Boolean growing;

    /* True if queue contains real handles.  Otherwise slots
       contain 32-bit integer values.  */
    Boolean handles;
} MsgQueue;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static void Callback (Globals *g, MsgQueue *q, Boolean full)
    MSGQUEUE_SECTION;

static void DumpQueue (Globals *g, MsgQueue *q, MemHandle *buf)
    MSGQUEUE_SECTION;

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

static void
Callback (Globals *g, MsgQueue *q, Boolean full)
{
    MsgQueueCallback cb = q->callback;
    if (cb == 0)
        return;

    /* Temporarily disable the callback to prevent recursive calls. */
    q->callback = 0;
    cb (g, q, full);
    q->callback = cb;
}


#if LOGLEVEL <= LogTrace
static void
DumpQueue (Globals *g, MsgQueue *q, MemHandle *buf)
{
    int i;
    DEBUG ("Dumping queue %s (p%d+q%d):", q->name,
           q->processing, q->waiting);
    for (i = 0; i < q->bufsize; i++) {
        DEBUG ("        %lx%s%s%s", (UInt32) buf[i],
               (i == q->process ? " P" : ""),
               (i == q->front ? " F" : ""),
               (i == q->end ? " E" : ""));
    }
}
#else
static inline void
DumpQueue (Globals *g, MsgQueue *q, MemHandle *buf)
{}
#endif

/***********************************************************************
 *
 *  FUNCTION DEFINITIONS
 *
 ***********************************************************************/


MsgQueuePtr
f_MsgNewQueue (Globals *g, char *name, UInt16 size,
               Boolean growing, Boolean handles,
               MsgQueueCallback callback)
{
    MsgQueuePtr q = (MsgQueuePtr) CheckedMemPtrNew (sizeof (MsgQueue),
                                                    true);
    MemHandle *buf;
    q->buf = CheckedMemHandleNew (size * sizeof (MemHandle), true);

    buf = MemHandleLock (q->buf);
    MemSet (buf, size * sizeof (MemHandle), 0);
    MemPtrUnlock (buf);

    q->bufsize = size;
    q->name = name;
    q->process = q->front = q->end = q->processing = q->waiting = 0;
    q->callback = callback;
    q->growing = growing;
    q->handles = handles;
    q->locked = false;
    TRACE ("{%s} MsgNewQueue %lx: %d, %d, %lx",
           name, (UInt32) q, size, growing, (UInt32) callback);
    return q;
}


void
f_MsgFreeQueue (Globals *g, MsgQueuePtr q)
{
    if (!q) return;

    PANICIF (q->locked, "Freeing locked queue");
    q->locked = true;

    TRACE ("{%s} MsgFreeQueue %lx", q->name, (UInt32) q);
    f_MsgFlush (g, q);
    q->process = q->front = q->end = q->processing = q->waiting = 0;
    MemHandleFree (q->buf);
    q->buf = 0;
    q->callback = 0;
    MemPtrFree (q);
}



MemHandle
f_MsgAddHandle (Globals *g, MsgQueuePtr q, MemHandle handle)
{
    MemHandle *buf;
    if (!q) return 0;

    if (q->locked) {
        LOG ("{%s} MsgAddPtr on locked queue %lx",
              q->name, (UInt32) q);
        if (q->handles)
            MemHandleFree (handle);
        return 0;
    }

    q->locked = true;

    TRACE ("{%s} MsgAddHandle %lx <- %lx (size p%d+q%d)",
           q->name, (UInt32) q, (UInt32) handle,
           q->processing, q->waiting);
    buf = MemHandleLock (q->buf);
    DumpQueue (g, q, buf);

    PANICIF (SUBTRACT (q->end, q->process)
             != (q->processing + q->waiting) % q->bufsize,
             "{%s} Broken queue %d/%d/%d (%d/%d/%d)",
             q->name, q->process, q->front, q->end,
             q->processing, q->waiting, q->bufsize);

    PANICIF (SUBTRACT (q->end, q->front) != q->waiting % q->bufsize,
             "{%s} Broken queue %d/%d/%d (%d/%d/%d)",
             q->name, q->process, q->front, q->end,
             q->processing, q->waiting, q->bufsize);

    PANICIF (q->processing + q->waiting != q->bufsize && buf[q->end] != 0,
             "{%s} Broken queue %d/%d/%d (%d/%d/%d)",
             q->name, q->process, q->front, q->end,
             q->processing, q->waiting, q->bufsize);
    MemPtrUnlock (buf);

    /* See if we have run out of space.  */
    PANICIF (q->bufsize == q->processing + q->waiting && q->waiting == 0,
             "{%s} Client does not process items", q->name);

    if (q->bufsize == q->processing + q->waiting)
        Callback (g, q, true);

    if (q->bufsize == q->processing + q->waiting &&
        (!q->growing || q->bufsize == MAX_QUEUE_SIZE)) {
        /* Silently discard message.  */
        DEBUG ("{%s} Reached maximum size, discarding %lx",
               q->name, (UInt32) handle);
        if (q->handles)
            MemHandleFree (handle);
        q->locked = false;
        return handle;
    }

    /* Enlarge buffer if still necessary.  */
    if (q->bufsize == q->processing + q->waiting) {
        UInt16 increment;
        int i = 0;
        int j = q->bufsize;

        q->bufsize = QUEUE_GROWTH (q->bufsize);
        increment = q->bufsize - j;
        PANICIF (increment == 0, "{%s} Zero increment", q->name);

        DEBUG ("{%s} Enlarging message queue %lx to %d",
               q->name, (UInt32)q, q->bufsize);
        CheckedMemHandleResize (q->buf, q->bufsize * sizeof (MemHandle));

        buf = MemHandleLock (q->buf);

        /* We need to rearrange items.  */
        while (i != q->end) {
            buf[j] = buf[i];
            INC (j);
            i++;
        }
        q->end = j;
        while (j != q->process) {
            buf[j] = 0;
            INC (j);
        }
        if (q->front < q->process)
            q->front = SUBTRACT (q->front, increment);

        PANICIF (SUBTRACT (q->end, q->process)
                 != (q->processing + q->waiting) % q->bufsize,
                 "Broken grow algorithm %d/%d/%d (%d/%d/%d)",
                 q->process, q->front, q->end,
                 q->processing, q->waiting, q->bufsize);

        PANICIF (SUBTRACT (q->end, q->front) != q->waiting % q->bufsize,
                 "Broken grow algorithm %d/%d/%d (%d/%d/%d)",
                 q->process, q->front, q->end,
                 q->processing, q->waiting, q->bufsize);

        PANICIF (buf[q->end] != 0,
                 "Broken grow algorithm %d/%d/%d (%d/%d/%d)",
                 q->process, q->front, q->end,
                 q->processing, q->waiting, q->bufsize);

        MemPtrUnlock (buf);
    }

    /* Add new item.  */
    buf = MemHandleLock (q->buf);
    buf[q->end] = handle;

    /* Update pointers.  */
    INC (q->end);
    q->waiting++;

    DumpQueue (g, q, buf);
    MemPtrUnlock (buf);

    Callback (g, q, false);

    q->locked = false;

    return handle;
}



MemHandle
f_MsgAddPtr (Globals *g, MsgQueuePtr q, UInt8 *data, UInt16 length)
{
    if (!q) return 0;

    PANICIF (!q->handles, "MsgAddPtr on value queue");
    if (!q->locked) {
        MemHandle handle = CheckedMemHandleNew (length, true);
        UInt8 *d = MemHandleLock (handle);
        MemMove (d, data, length);
        MemPtrUnlock (d);
        return MsgAddHandle (q, handle);
    }
    else {
        LOG ("MsgAddPtr on locked queue %lx", (UInt32) q);
        return 0;
    }
}



UInt16
f_MsgAvailable (Globals *g, MsgQueuePtr q)
{
    if (!q)
        return 0;
    else
        return q->waiting;
}



Boolean
f_MsgIsInQueue (Globals *g, MsgQueuePtr q, MemHandle h)
{
    /* Note: It is OK to call this on a locked queue!  */
    int i, c;
    MemHandle *buf;

    if (!q)
        return false;
    if (!h)
        return false;

    buf = MemHandleLock (q->buf);
    i = q->front;
    for (c = 0; c < q->waiting; c++) {
        if (buf[i] == h) {
            MemPtrUnlock (buf);
            return true;
        }
        INC (i);
    }
    MemPtrUnlock (buf);
    return false;
}



MemHandle
f_MsgGet (Globals *g, MsgQueuePtr q)
{
    /* Note: It is OK to call this on a locked queue!  */
    MemHandle handle;
    MemHandle *buf;
    if (!q)
        return 0;
    if (q->waiting == 0)
        return 0;

    buf = MemHandleLock (q->buf);
    handle = buf[q->front];
    INC (q->front);
    q->waiting--;
    q->processing++;
    TRACE ("{%s} MsgGet %lx -> %lx (size p%d+q%d)",
           q->name, (UInt32) q, (UInt32) handle,
           q->processing, q->waiting);
    DumpQueue (g, q, buf);
    MemPtrUnlock (buf);

    return handle;
}



MemHandle
f_MsgGetAndProcess (Globals *g, MsgQueuePtr q)
{
    MemHandle result = f_MsgGet (g, q);
    if (result) {
        ASSERT (!f_MsgIsProcessed (g, q, result));
        f_MsgProcessed (g, q, result);
    }
    return result;
}



MemHandle
f_MsgLast (Globals *g, MsgQueuePtr q)
{
    MemHandle *buf;
    MemHandle result;
    if (!q || !g) return 0;

    if (q->waiting == 0)
        return 0;

    buf = MemHandleLock (q->buf);
    result = buf[SUBTRACT (q->end, 1)];
    ASSERT (result != 0);
    MemPtrUnlock (buf);

    return result;
}



void
f_MsgProcessed (Globals *g, MsgQueuePtr q, MemHandle h)
{
    /* Note: It is OK to call this on a locked queue!  */
    int i;
    MemHandle *buf;
    int c;

    if (!q || !h || !g) return;

    TRACE ("{%s} MsgProcessed %lx -> %lx (size p%d+q%d)",
           q->name, (UInt32) q, (UInt32) h,
           q->processing, q->waiting);

    buf = MemHandleLock (q->buf);

    i = q->process;
    for (c = 0; c < q->processing; c++) {
        if (buf[i] == h) {
            break;
        }
        INC (i);
    }

    DumpQueue (g, q, buf);
    PANICIF (c == q->processing,
             "{%s} MsgProcessed on invalid handle %lx",
             q->name, (UInt32) h);

    while (c > 0) {
        int prev = i;
        DEC (prev);
        buf[i] = buf[prev];
        i = prev;
        c--;
    }
    buf[q->process] = 0;
    INC (q->process);
    q->processing--;
    DumpQueue (g, q, buf);
    MemPtrUnlock (buf);
}



Boolean
f_MsgIsProcessed (Globals *g, MsgQueuePtr q, MemHandle h)
{
    /* Note: It is OK to call this on a locked queue!  */
    int i, c;
    MemHandle *buf;

    TRACE ("MsgIsProcessed q=%lx h=%lx", (UInt32)q, (UInt32)h);

    if (!q)
        return true;
    if (!h)
        return true;

    buf = MemHandleLock (q->buf);
    i = q->process;
    for (c = 0; c < q->processing + q->waiting; c++) {
        if (buf[i] == h) {
            MemPtrUnlock (buf);
            return false;
        }
        INC (i);
    }
    MemPtrUnlock (buf);
    return true;
}



void
f_MsgFlush (Globals *g, MsgQueuePtr q)
{
    /* Note: It is OK to call this on a locked queue.
       However, the callback function won't be called in that case.  */
    if (!q) return;
    if (!q->locked && q->waiting)
        Callback (g, q, true);

    if (q->waiting) {
        MemHandle *buf = MemHandleLock (q->buf);
        UInt16 i, c;
        i = q->front;
        for (c = 0; c < q->waiting; c++) {
            if (buf[i] != 0 && q->handles)
                MemHandleFree (buf[i]);
            buf[i] = 0;
            INC (i);
        }
        q->end = q->front;
        DEBUG ("{%s} %d items flushed from queue", q->name, q->waiting);
        q->waiting = 0;
        MemPtrUnlock (buf);}
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

