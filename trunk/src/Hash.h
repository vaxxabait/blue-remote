/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Hash.h
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
#define SECTION HASH_SECTION

#ifdef NORMAL_INCLUDE
typedef MemHandle Hash;
typedef MemPtr HashIter;

#define InvalidHash NULL

#endif  /* NORMAL_INCLUDE */

/* Create a new hash table of size MAX.  */
DEFUN1 (Hash, HashCreate, int max);

/* Delete HASH.  Values are lost.  */
DEFUN1 (void, HashDelete, Hash hash);

/* Put VALUE in HASH at KEY.  Returns the previous
 * value stored at KEY, if any.  */
DEFUN3 (void *, HashPut, Hash hash, UInt32 key, void *value);

/* Remove the value with KEY from the hash, and return it.  */
DEFUN2 (void *, HashRemove, Hash hash, UInt32 key);

/* Get the value in HASH at KEY.
 * If there is no value with key KEY, return NULL.  */
DEFUN2 (void *, HashGet, Hash hash, UInt32 key);

/* Get an iterator to list all elements in this hash.
 * The hash may be modified without problems
 * while iterators are active on it.  The
 * modifications may or may not affect the list
 * of items returned by the iterator.  */
DEFUN1 (HashIter, HashIterGet, Hash hash);

/* Close a hash iterator.  Its pointer will become invalid
 * after this call.  Iterators may be closed before they reach
 * the end of their hash table.  */
DEFUN1 (void, HashIterClose, HashIter hashIter);

/* Step to the next item in the hash table.  Returns false if
 * there are no more entries.  */
DEFUN1 (Boolean, HashIterNext, HashIter hashIter);

/* Get the key at the current position of an iterator.  */
DEFUN1 (UInt32, HashIterGetKey, HashIter hashIter);

/* Get the value at the current position of an iterator.  */
DEFUN1 (void *, HashIterGetValue, HashIter hashIter);
