/**************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Hash.c
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

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Hash.h"
#undef PRIVATE_INCLUDE


/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Fibonacci hashing */
#define HASH_MAGIC 2654435769U

#define HashEntryEmpty(entry) ((entry).state == 0)
#define HashEntryDeleted(entry) ((entry).state == 2)
#define HashEntryFull(entry) ((entry).state == 1)
#define HashEntryMatches(entry,key) (HashEntryFull(entry) && (entry).key == (key))

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct HashEntry {
    /* Key.  */
    UInt32 key;

    /* Value. */
    void *value;

    /* State:
     * 0 if empty;
     * 1 if full;
     * 2 if deleted. */
    UInt8 state;
} HashEntry;

typedef struct {
    /* The size of this hash table.  */
    int max;

    /* The order of this hash table.  2^order = size.  */
    int order;

    /* The number of elements in this hash table.  */
    int size;

    /* The table.  */
    MemHandle entries;
} HashImpl;

typedef struct {
    /* The hash of this iterator.  */
    HashImpl *hashImpl;

    /* The current index of this iterator.  */
    int index;

    /* Direct pointer to data in hash->entries.  */
    HashEntry *entries;
} HashIterImpl;


/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/


static UInt32 HashFn (Globals *g, HashImpl *hashImpl, UInt32 key)
    HASH_SECTION;

static UInt32 HashFindEntry (Globals *g, HashImpl *hashImpl, UInt32 key)
    HASH_SECTION;


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


static UInt32
HashFn (Globals *g, HashImpl *hashImpl, UInt32 key)
{
    UInt32 result = (key * HASH_MAGIC) >> (32 - hashImpl->order);
    PANICIF (result >= hashImpl->max, "Broken hash function");
    return result;
}



static UInt32
HashFindEntry (Globals *g, HashImpl *hashImpl, UInt32 key)
{
    UInt32 index = HashFn (g, hashImpl, key);
    UInt32 deleted = 0;
    Boolean foundDeleted = false;
    HashEntry *entry;
#if LOGLEVEL <= LogTrace
    int count = 0;
#endif

    entry = MemHandleLock (hashImpl->entries);
    /* Search for existing entry.  */
    while (true) {
#if LOGLEVEL <= LogTrace
        count++;
#endif
        if (HashEntryMatches (entry[index], key)) {
            /* Found it.  */
            break;
        }
        if (HashEntryEmpty (entry[index])) {
            /* Found an empty cell.  Return the first deleted cell
             * found, if any.  Otherwise return this empty cell.  */
            if (foundDeleted)
                index = deleted;
            break;
        }
        if (!foundDeleted && HashEntryDeleted (entry[index])) {
            /* Remember first deleted entry found.  */
            foundDeleted = true;
            deleted = index;
        }
        index++;
        if (index == hashImpl->max)
            index = 0;
    }
    TRACE ("#%lx Key %lu %sfound in %hd tries",
           (UInt32) MemPtrRecoverHandle (hashImpl),
           (UInt32) key,
           (HashEntryFull (entry[index]) ? "" : "not "),
           count);

    MemHandleUnlock (hashImpl->entries);

    return index;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/



Hash
f_HashCreate (Globals *g, int max)
{
    int order;
    Hash hash = CheckedMemHandleNew (sizeof (HashImpl), true);
    HashImpl *hashImpl;
    if (hash == 0) {
        PANICIF (true, "Could not allocate hash");
        return InvalidHash;
    }
    hashImpl = MemHandleLock (hash);

    /* Get order.  */
    for (order = 1; max != 0; order++)
        max >>= 1;
    /* Refuse to create a hash table with less than 16 slots.  */
    if (order < 4)
        order = 4;
    /* Round up max.  */
    max = 1 << order;

    hashImpl->max = max;
    hashImpl->order = order;
    hashImpl->size = 0;
    hashImpl->entries = MemHandleNew (sizeof (HashEntry) * max);
    if (hashImpl->entries == 0) {
        MemHandleFree (hash);
        PANICIF (true, "Hash too large");
        return NULL;
    }

    /* Clear the entries.  */
    {
        void *p = MemHandleLock (hashImpl->entries);
        MemSet (p, sizeof (HashEntry) * max, 0);
        MemHandleUnlock (hashImpl->entries);
    }
    MemHandleUnlock (hash);
    TRACE ("#%lx Created with size %lu", (UInt32) hash, (UInt32) max);
    return hash;
}



void
f_HashDelete (Globals *g, Hash hash)
{
    Err err = 0;
    HashImpl *hashImpl = MemHandleLock (hash);
    err = MemHandleFree (hashImpl->entries);
    PANICIF (err, "Could not free handle");
    hashImpl->entries = 0;
    MemHandleUnlock (hash);
    err = MemHandleFree (hash);
    PANICIF (err, "Could not free hash");
    TRACE ("#%lx Deleted", (UInt32) hash);
}



void *
f_HashPut (Globals *g, Hash hash, UInt32 key, void *value)
{
    UInt32 index;
    HashEntry *entry;
    HashImpl *hashImpl = MemHandleLock (hash);
    void *result = NULL;
    PANICIF (!hashImpl, "Invalid hash");
    PANICIF (hashImpl->size == hashImpl->max - 1, "Hash is full");
    index = HashFindEntry (g, hashImpl, key);
    entry = MemHandleLock (hashImpl->entries);
    if (HashEntryEmpty (entry[index]) || HashEntryDeleted (entry[index])) {
        entry[index].key = key;
        entry[index].value = value;
        entry[index].state = 1;
        result = NULL;
    }
    else {
        PANICIF (!HashEntryMatches (entry[index], key), "Broken hash");
        result = entry[index].value;
        entry[index].value = value;
        entry[index].state = 1;
    }
    MemHandleUnlock (hashImpl->entries);
    MemHandleUnlock (hash);
    return result;
}



void *
f_HashRemove (Globals *g, Hash hash, UInt32 key)
{
    UInt32 index;
    HashEntry *entry;
    HashImpl *hashImpl = MemHandleLock (hash);
    void *result = NULL;
    PANICIF (!hashImpl, "Invalid hash");
    PANICIF (hashImpl->size == hashImpl->max - 1, "Hash is full");
    index = HashFindEntry (g, hashImpl, key);
    entry = MemHandleLock (hashImpl->entries);
    if (HashEntryFull (entry[index])) {
        PANICIF (!HashEntryMatches (entry[index], key), "Broken hash");
        result = entry[index].value;
        entry[index].state = 2;
    }
    MemHandleUnlock (hashImpl->entries);
    MemHandleUnlock (hash);
    return result;
}



void *
f_HashGet (Globals *g, Hash hash, UInt32 key)
{
    UInt32 index;
    HashEntry *entry;
    HashImpl *hashImpl = MemHandleLock (hash);
    void *result = NULL;

    PANICIF (!hashImpl, "Invalid hash");
    index = HashFindEntry (g, hashImpl, key);
    entry = MemHandleLock (hashImpl->entries);
    if (HashEntryMatches (entry[index], key))
        result = entry[index].value;
    else
        result = NULL;
    MemHandleUnlock (hashImpl->entries);
    MemHandleUnlock (hash);
    return result;
}



HashIter
f_HashIterGet (Globals *g, Hash hash)
{
    HashIterImpl *iter = CheckedMemPtrNew (sizeof (HashIterImpl), true);
    HashImpl *hashImpl = MemHandleLock (hash);
    PANICIF (hashImpl == 0, "Invalid hash");
    iter->hashImpl = hashImpl;
    iter->index = -1;
    iter->entries = MemHandleLock (hashImpl->entries);
    PANICIF (iter->entries == 0, "Invalid hash");
    return iter;
}



void
f_HashIterClose (Globals *g, HashIter hashIter)
{
    HashIterImpl *iter = (HashIterImpl *) hashIter;
    MemHandleUnlock (iter->hashImpl->entries);
    iter->entries = NULL;
    MemHandleUnlock (MemPtrRecoverHandle (iter->hashImpl));
    iter->hashImpl = NULL;
    MemPtrFree (iter);
}



Boolean
f_HashIterNext (Globals *g, HashIter hashIter)
{
    HashIterImpl *iter = (HashIterImpl *) hashIter;
    while (++iter->index < iter->hashImpl->max) {
        if (HashEntryFull (iter->entries[iter->index]))
            return true;
    }
    return false;
}



UInt32
f_HashIterGetKey (Globals *g, HashIter hashIter)
{
    HashIterImpl *iter = (HashIterImpl *) hashIter;
    PANICIF (iter->index < 0, "Hash iterator hasn't started yet");
    PANICIF (!HashEntryFull (iter->entries[iter->index]),
	     "Broken hash iterator");
    return iter->entries[iter->index].key;
}



void *
f_HashIterGetValue (Globals *g, HashIter hashIter)
{
    HashIterImpl *iter = (HashIterImpl *) hashIter;
    PANICIF (iter->index < 0, "Hash iterator hasn't started yet");
    PANICIF (!HashEntryFull (iter->entries[iter->index]),
	     "Broken hash iterator");
    return iter->entries[iter->index].value;
}
