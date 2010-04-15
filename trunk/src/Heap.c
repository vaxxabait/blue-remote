/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Heap.c
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
 **************************************************************************/

#define LOGLEVEL LogDebug

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Heap.h"
#undef PRIVATE_INCLUDE

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define UP      ((i - 1) >> 1)
#define LEFT    ((i << 1) + 1)
#define RIGHT   ((i << 1) + 2)

#define DATA    (ASSERT (i < impl->size), impl->data[i])
#define UPD     (i > 0 ? impl->data[UP] : 0)
#define LEFTD   (LEFT < impl->size ? impl->data[LEFT] : 0)
#define RIGHTD  (RIGHT < impl->size ? impl->data[RIGHT] : 0)

#define CMP(a,b)    ((a) == (b) ? 0				\
                     : (((a) == 0 ? 1				\
                         : ((b) == 0 ? -1			\
                            : impl->cmp (g, (a), (b))))))

#define SWAP(j)                                         \
    ({                                                  \
        MemHandle t = impl->data[i];                    \
        UInt32 k = (j);                                 \
        ASSERT (i < impl->size);                        \
        ASSERT (k < impl->size);                        \
        TRACE ("Swapping #%lu (%lx) with #%lu (%lx)",   \
               i, (UInt32) impl->data[i],               \
               k, (UInt32) impl->data[k]);              \
        impl->data[i] = impl->data[k];                  \
        impl->data[k] = t;                              \
        i = k;                                          \
        ASSERT (DATA == t);                             \
    })



/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct HeapImpl {
    char *name;
    Comparator cmp;
    UInt32 maxsize;
    UInt32 size;
    MemHandle data[0];
} HeapImpl;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static void Adjust (Globals *g, HeapImpl *impl, UInt32 i)
    HEAP_SECTION;

static void FloatDown (Globals *g, HeapImpl *impl, UInt32 i)
    HEAP_SECTION;

static void RaiseUp (Globals *g, HeapImpl *impl, UInt32 i)
    HEAP_SECTION;

static void DumpHeap_ (Globals *g, HeapImpl *impl,
                       char *buf, UInt32 i, int level)
    HEAP_SECTION;

static void DumpHeap (Globals *g, HeapImpl *impl)
    HEAP_SECTION;

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
Adjust (Globals *g, HeapImpl *impl, UInt32 i)
{
    if (impl->size == 0) {
        ASSERT (i == 0);
        return;
    }
    if (UPD && CMP (DATA, UPD) < 0)
        RaiseUp (g, impl, i);
    else
        FloatDown (g, impl, i);
}


static void
FloatDown (Globals *g, HeapImpl *impl, UInt32 i)
{
    TRACE ("FloatDown %s[%lx]", impl->name, i);
    ASSERT (i < impl->size);
    while (LEFTD) {
        Boolean l = LEFTD && CMP (DATA, LEFTD) > 0;
        Boolean r = RIGHTD && CMP (DATA, RIGHTD) > 0;
        if (l && !r)
            SWAP (LEFT);
        else if (!l && r)
            SWAP (RIGHT);
        else if (l && r) {
            if (CMP (LEFTD, RIGHTD) < 0)
                SWAP (LEFT);
            else
                SWAP (RIGHT);
        }
        else
            break;
    }
}


static void
RaiseUp (Globals *g, HeapImpl *impl, UInt32 i)
{
    TRACE ("RaiseUp %s[%lx]", impl->name, i);
    ASSERT (i < impl->size);
    while (UPD && CMP (DATA, UPD) < 0)
        SWAP (UP);
}

#if LOGLEVEL <= LogTrace
static void
DumpHeap_ (Globals *g, HeapImpl *impl, char *buf, UInt32 i, int level)
{
    int j;
    char *p = buf;
    for (j = 0; j < level; j++) {
        *p++ = ' ';
        *p++ = ' ';
        *p++ = ' ';
    }
    if (i < impl->size) {
        StrPrintF (p, "%lx", (UInt32) impl->data[i]);
        TRACE ("%s", buf);
        if (LEFT) {
            ASSERT (CMP (DATA, LEFTD) <= 0);
            ASSERT (CMP (DATA, RIGHTD) <= 0);
            DumpHeap_ (g, impl, buf, LEFT, level + 1);
            DumpHeap_ (g, impl, buf, RIGHT, level + 1);
        }
    }
    else {
        *p++ = '.';
        *p++ = 0;
        TRACE ("%s", buf);
    }
}


static void
DumpHeap (Globals *g, HeapImpl *impl)
{
    char buf[128];
    TRACE ("Dumping heap %s (s%lu):", impl->name, impl->size);
    DumpHeap_ (g, impl, buf, 0, 1);
}
#else
static inline void
DumpHeap (Globals *g, HeapImpl *impl)
{}
static inline void
DumpHeap_ (Globals *g, HeapImpl *impl, char *buf, UInt32 i, int level)
{
}
#endif

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


Heap
f_HeapNew (Globals *g, char *name, UInt32 maxsize, Comparator cmp)
{
    Heap heap = CheckedMemHandleNew (sizeof (HeapImpl)
                                     + maxsize * sizeof (MemHandle),
                                     true);
    HeapImpl *impl = MemHandleLock (heap);
    ASSERT (cmp != NULL);
    ASSERT (maxsize > 0);
    if (name)
        impl->name = name;
    else
        impl->name = "<unnamed heap>";
    impl->cmp = cmp;
    impl->maxsize = maxsize;
    impl->size = 0;
    MemSet (impl->data, maxsize * sizeof (MemHandle), 0);
    TRACE ("Created %s heap", impl->name);
    MemPtrUnlock (impl);
    return heap;
}


void
f_HeapFree (Globals *g, Heap heap)
{
    HeapImpl *impl = MemHandleLock (heap);
    TRACE ("Freeing %s heap", impl->name);
    PANICIF (impl->size, "Heap %s is not empty (%ld)",
             impl->name, impl->size);
    MemPtrUnlock (impl);
    MemHandleFree (heap);
}


char *
f_HeapName (Globals *g, Heap heap)
{
    HeapImpl *impl = MemHandleLock (heap);
    char *name = impl->name;
    MemPtrUnlock (impl);
    return name;
}


void
f_HeapAdd (Globals *g, Heap heap, MemHandle h)
{
    HeapImpl *impl = MemHandleLock (heap);
    ASSERT (impl->size < impl->maxsize);
    ASSERT (impl->data[impl->size] == 0);
    impl->data[impl->size++] = h;
    Adjust (g, impl, impl->size - 1);
    TRACE ("HeapAdd %s <- %lx", impl->name, (UInt32) h);
    DumpHeap (g, impl);
    MemPtrUnlock (impl);
    return;
}


MemHandle
f_HeapGet (Globals *g, Heap heap)
{
    HeapImpl *impl = MemHandleLock (heap);
    MemHandle result = (impl->size > 0 ? impl->data[0] : 0);
    TRACE ("HeapGet %s -> %lx", impl->name, (UInt32) result);
    MemPtrUnlock (impl);
    return result;
}


MemHandle
f_HeapRemove (Globals *g, Heap heap)
{
    HeapImpl *impl = MemHandleLock (heap);
    MemHandle result = 0;
    if (impl->size == 0)
        goto exit;
    result = impl->data[0];
    impl->data[0] = 0;
    impl->size--;
    if (impl->size > 0) {
        impl->data[0] = impl->data[impl->size];
        impl->data[impl->size] = 0;
    }
    Adjust (g, impl, 0);
exit:
    TRACE ("HeapRemove %s -> %lx", impl->name, (UInt32) result);
    DumpHeap (g, impl);
    MemPtrUnlock (impl);
    return result;
}


void
f_HeapDelete (Globals *g, Heap heap, MemHandle h)
{
    HeapImpl *impl = MemHandleLock (heap);
    int i;
    ASSERT (impl->size);

    for (i = 0; i < impl->size; i++) {
        if (impl->data[i] == h)
            break;
    }
    ASSERT (i != impl->size);

    impl->data[i] = 0;
    impl->size--;
    if (i != impl->size) {
        impl->data[i] = impl->data[impl->size];
        impl->data[impl->size] = 0;
        Adjust (g, impl, i);
    }

    TRACE ("HeapDelete %s, %lx", impl->name, (UInt32) h);
    DumpHeap (g, impl);
    MemPtrUnlock (impl);
}


void
f_HeapChange (Globals *g, Heap heap, MemHandle h)
{
    HeapImpl *impl = MemHandleLock (heap);
    int i;
    ASSERT (impl->size);
    for (i = 0; i < impl->size; i++) {
        if (impl->data[i] == h)
            break;
    }
    ASSERT (i != impl->size);

    Adjust (g, impl, i);

    TRACE ("HeapChange %s, %lx", impl->name, (UInt32) h);
    DumpHeap (g, impl);
    MemPtrUnlock (impl);
}
