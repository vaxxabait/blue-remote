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
