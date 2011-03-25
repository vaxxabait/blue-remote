/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: SDP.h
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
#define SECTION SDP_SECTION

DEFUN2 (Boolean, SDPVerifyRecord, UInt8 *start, UInt8 *end);

DEFUN2 (UInt8 *, SDPUInt16, UInt8 *buf, UInt16 data);
DEFUN3 (UInt8 *, SDPAddShortToBuf, UInt8 *buf, UInt8 type, UInt32 data);
DEFUNN (UInt8 *, SDPAddLongToBuf, UInt8 *buf, UInt8 type, UInt8 *data, UInt32 datasize);

DEFUN2 (UInt8 *, SDPSeriesStart, UInt8 *buf, UInt8 type);
DEFUN3 (UInt8 *, SDPSeriesEnd, UInt8 *start, UInt8 *end, UInt8 type);

/* Add a Signed Integer element to BUF.  */
DEFUN2 (UInt8 *, SDPSInt, UInt8 *buf, Int32 data);

/* Add a UUID element to BUF.  */
DEFUN2 (UInt8 *, SDPUUID, UInt8 *buf, BtLibSdpUuidType *uuid);

#ifdef NORMAL_INCLUDE

#define SDPTypeNil          0x00    /* Null type */
#define SDPTypeUInt         0x08    /* Unsigned integer */
#define SDPTypeSInt         0x10    /* Signed integer */
#define SDPTypeUUID         0x18    /* UUID */
#define SDPTypeString       0x20    /* String */
#define SDPTypeBoolean      0x28    /* Boolean (0 = False, 1 = True) */
#define SDPTypeSequence     0x30    /* Data element sequence */
#define SDPTypeAlternative  0x38    /* Data element alternative */
#define SDPTypeURL          0x40    /* URL */
#define SDPTypeMask         0xF8

#define SDPDataSize1        0x00 /* 1 byte (or 0 if type == SDPTypeNil) */
#define SDPDataSize2        0x01 /* 2 bytes */
#define SDPDataSize4        0x02 /* 4 bytes */
#define SDPDataSize8        0x03 /* 8 bytes */
#define SDPDataSize16       0x04 /* 16 bytes */
#define SDPDataSizeLong1    0x05 /* 1-byte length field follows */
#define SDPDataSizeLong2    0x06 /* 2-byte length field follows */
#define SDPDataSizeLong4    0x07 /* 4-byte length field follows */
#define SDPDataSizeMask     0x07

/* Add a Nil element to BUF.  */
#define SDPNil(buf) (buf++ = (SDPTypeNil | SDPDataSize1))

/* Add an Unsigned Integer element to BUF.  */
#define SDPUInt(buf, data) SDPAddShortToBuf (buf, SDPTypeUInt, data)

/* Add a short (two-byte) UUID element to BUF.  */
#define SDPShortUUID(buf, data) SDPAddShortToBuf (buf, SDPTypeUUID, BtLibSdpHToNS ((data) & 0xFFFF))

/* Add a String element to BUF.  */
#define SDPString(buf, data) SDPAddLongToBuf (buf, SDPTypeString, data, StrLen (data))

/* Add a Boolean element to BUF.  */
#define SDPBoolean(buf, data) SDPAddShortToBuf (buf, SDPTypeBoolean, (data ? 1 : 0))

/* Add a URL element to BUF.  */
#define SDPURL(buf, data) SDPAddLongToBuf (buf, SDPTypeURL, data, StrLen (data))

/* Start a Sequence element in BUF.  You must store the returned
 * value to be able to close the sequence later.  You must not keep
 * two sequences open in the same buffer unless one of them is
 * embedded in the other.  In the case of embedded sequences, you
 * must close the embedded sequence first.  */
#define SDPSequenceStart(buf)   SDPSeriesStart (buf, SDPTypeSequence)

/* Close the Sequence element starting at START and ending before END.  */
#define SDPSequenceEnd(start, end) SDPSeriesEnd(start, end, SDPTypeSequence)


/* Start an Alternative element in BUF. */
#define SDPAlternativeStart(buf)    SDPSeriesStart (buf, SDPTypeAlternative)

/* Close the Alternative element starting at START and ending before END.  */
#define SDPAlternativeEnd(start, end) SDPSeriesEnd(start, end, SDPTypeAlternative)


/* Start a String element in BUF. */
#define SDPStringStart(buf) SDPSeriesStart (buf, SDPTypeString)

/* Close the Alternative element starting at START and ending before END.  */
#define SDPStringEnd(start, end) SDPSeriesEnd(start, end, SDPTypeString)

#endif  /* NORMAL_INCLUDE */
