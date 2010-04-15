/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: SHA1.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION SHA1_SECTION

#ifdef NORMAL_INCLUDE
typedef struct SHA1State SHA1State;
#endif

/* Create a new SHA1 instance.  */
DEFUN0 (SHA1State *, SHA1Create);

/* Append LENGTH bytes of DATA to the SHA1 instance STATE.  */
DEFUN3 (void, SHA1Append, SHA1State *state, UInt8 *data, UInt32 length);

/* Delete the SHA1 instance STATE and return a copy of the final
 * digest value in DIGEST, which should be a 20-byte buffer.  */
DEFUN2 (void, SHA1Finish, SHA1State *state, UInt8 *digest);
