/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: SHA1.h
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
