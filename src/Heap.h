/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Heap.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION HEAP_SECTION

#ifdef NORMAL_INCLUDE

typedef MemHandle Heap;
typedef int (*Comparator) (struct Globals *g,
                           MemHandle a,
                           MemHandle b);

#endif /* NORMAL_INCLUDE */

/* Create a new heap of size MAXSIZE, name NAME and comparator CMP.  */
DEFUN3 (Heap, HeapNew, char *name, UInt32 maxsize, Comparator cmp);

/* Delete a heap.  It must be empty.  */
DEFUN1 (void, HeapFree, Heap heap);

/* Return the name of heap HEAP.  */
DEFUN1 (char *, HeapName, Heap heap);

/* Add H to HEAP.  Returns false if the heap is full.  */
DEFUN2 (void, HeapAdd, Heap heap, MemHandle h);

/* Get the smallest object in HEAP.  The object is not removed.
   Returns 0 if the heap is empty.   */
DEFUN1 (MemHandle, HeapGet, Heap heap);

/* Get and remove the smallest object in HEAP.
   Returns 0 if the heap is empty.  */
DEFUN1 (MemHandle, HeapRemove, Heap heap);

/* Deletes the given handle from the heap.  Returns true if found.
   The caller is responsible for freeing H, if necessary.
   This is an O(n) operation at the moment.  */
DEFUN2 (void, HeapDelete, Heap heap, MemHandle h);

/* Notifies the heap that the given handle has changed.
   Returns true if the handle was found.
   This is an O(n) operation at the moment.  */
DEFUN2 (void, HeapChange, Heap heap, MemHandle h);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

