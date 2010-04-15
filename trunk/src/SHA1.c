/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: SHA1.c
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

#include "Common.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define K1 0x5A827999L		/* sqrt (2) * 2^30 */
#define K2 0x6ED9EBA1L		/* sqrt (3) * 2^30 */
#define K3 0x8F1BBCDCL		/* sqrt (5) * 2^30 */
#define K4 0xCA62C1D6L		/* sqrt (10) * 2^30 */

#define f1(x, y, z)	(z ^ (x & (y ^ z)))       /* x ? y : z */
#define f2(x, y, z)	(x ^ y ^ z)	          /* XOR */
#define f3(x, y, z)	((x & y) + (z & (x ^ y))) /* Majority */

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

struct SHA1State {
    UInt32 count;
    UInt8 chunk[64];
    UInt32 digest[5];
};

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

/* Update the current digest by transforming a whole chunk.  */
static void Transform (Globals *g, SHA1State *state)
    SHA1_SECTION;

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
Transform (Globals *g, SHA1State *state) 
{
    UInt32 a, b, c, d, e, t, i;
    UInt32 *W = CheckedMemPtrNew (80 * sizeof (UInt32), true);
    UInt32 *chunk = (UInt32 *)state->chunk;
    ASSERT ((state->count & 0x3F) == 0);

    for (i = 0; i < 16; i++)
	W[i] = BIG2CPU32 (chunk[i]);
    for (i = 0; i < 64; i++)
	W[i + 16] = ROL (W[i+13] ^ W[i+8] ^ W[i+2] ^ W[i], 1);

    a = state->digest[0];
    b = state->digest[1];
    c = state->digest[2];
    d = state->digest[3];
    e = state->digest[4];

    for (i = 0; i < 20; i++) {
	t = f1 (b, c, d) + K1 + ROL (a, 5) + e + W[i];
	e = d; d = c; c = ROL (b, 30); b = a; a = t;
    }

    for (; i < 40; i++) {
	t = f2 (b, c, d) + K2 + ROL (a, 5) + e + W[i];
	e = d; d = c; c = ROL (b, 30); b = a; a = t;
    }

    for (; i < 60; i++) {
	t = f3 (b, c, d) + K3 + ROL (a, 5) + e + W[i];
	e = d; d = c; c = ROL (b, 30); b = a; a = t;
    }

    for (; i < 80; i++) {
	t = f2 (b, c, d) + K4 + ROL (a, 5) + e + W[i];
	e = d; d = c; c = ROL (b, 30); b = a; a = t;
    }

    state->digest[0] += a;
    state->digest[1] += b;
    state->digest[2] += c;
    state->digest[3] += d;
    state->digest[4] += e;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

SHA1State *
f_SHA1Create (Globals *g)
{
    SHA1State *state = CheckedMemPtrNew (sizeof (SHA1State), true);
    state->digest[0] = 0x67452301;
    state->digest[1] = 0xEFCDAB89;
    state->digest[2] = 0x98BADCFE;
    state->digest[3] = 0x10325476;
    state->digest[4] = 0xC3D2E1F0;
    return state;
}

void
f_SHA1Append (Globals *g, SHA1State *state, UInt8 *data, UInt32 length)
{
    UInt32 pos = (state->count & 0x3F);

    if (pos + length >= 64) {
	MemMove (state->chunk + pos, data, 64 - pos);
	state->count += 64 - pos;
	length -= 64 - pos;
	data += 64 - pos;
	Transform (g, state);
	pos = 0;
    }
    
    while (length >= 64) {
	pos = 0;
	MemMove (state->chunk, data, 64);
	state->count += 64;
	length -= 64;
	data += 64;
	Transform (g, state);
    }
    
    MemMove (state->chunk + pos, data, length);
    state->count += length;
}

void
f_SHA1Finish (Globals *g, SHA1State *state, UInt8 *digest)
{
    UInt32 pos = state->count & 0x3F;
    UInt32 padlen = (pos < 56 ? 56 : 120) - pos;
    UInt32 bits = CPU2BIG32 (state->count << 3);
    UInt32 zero = 0;
    UInt32 *d = (UInt32*) digest;
    int i;
    if (padlen > 0) {
	/* This could be improved by having a constant pad
	 * somewhere, but I can't be bothered to define a global
	 * variable for this.  */
	UInt8 *padding = CheckedMemPtrNew (padlen, false);
	padding[0] = 0x80;
	f_SHA1Append (g, state, padding, padlen);
	MemPtrFree (padding);
    }
    f_SHA1Append (g, state, (UInt8 *) &zero, sizeof (UInt32));
    f_SHA1Append (g, state, (UInt8 *) &bits, sizeof (UInt32));
    ASSERT ((state->count & 0x3F) == 0);
    for (i = 0; i < 5; i++)
	d[i] = CPU2BIG32 (state->digest[i]);
    MemPtrFree (state);
}
