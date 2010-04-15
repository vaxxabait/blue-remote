/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Hash.h
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

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

