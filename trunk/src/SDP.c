/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: SDP.c
 *
 ***********************************************************************/

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "SDP.h"
#undef PRIVATE_INCLUDE

Boolean
f_SDPVerifyRecord (Globals *g, UInt8 *start, UInt8 *end)
{
    UInt8 *recend, *recstart;

    while (start < end) {
        UInt8 tag = *start;
        switch (tag & SDPTypeMask) {
	    case SDPTypeNil:
		if (tag != (SDPTypeNil | SDPDataSize1))
		    return false;
		start += 1;
		continue;

	    case SDPTypeUInt:
	    case SDPTypeSInt:
	    case SDPTypeUUID:
	    case SDPTypeBoolean:
		switch (tag & SDPDataSizeMask) {
		    case SDPDataSize1: start += 2; break;
		    case SDPDataSize2: start += 3; break;
		    case SDPDataSize4: start += 5; break;
		    case SDPDataSize8: start += 9; break;
		    case SDPDataSize16: start += 17; break;
		    default:
			return false;
		}
		continue;

	    case SDPTypeString:
	    case SDPTypeURL:
	    case SDPTypeSequence:
	    case SDPTypeAlternative:
		switch (tag & SDPDataSizeMask) {
		    case SDPDataSizeLong1:
			recend = start + 2 + start[1];
			recstart = start + 2;
			break;
		    case SDPDataSizeLong2:
			recend = start + 3
			    + BtLibSdpNToHS (*((UInt16 *)(start + 1)));
			recstart = start + 3;
			break;
		    case SDPDataSizeLong4:
			recend = start + 5
			    + BtLibSdpNToHS (*((UInt32 *)(start + 1)));
			recstart = start + 5;
			break;
		    default:
			return false;
		}
		if (recend > end)
		    return false;
		if ((tag & SDPTypeMask) != SDPTypeString
		    && !SDPVerifyRecord (recstart, recend))
		    return false;
		start = recend;
		continue;

	    default:
		return false;
        }
    }
    return true;
}

UInt8 *
f_SDPUInt16 (Globals *g, UInt8 *buf, UInt16 data)
{
    *buf++ = SDPTypeUInt | SDPDataSize2;
    data = BtLibSdpHToNS (data);
    *((UInt16 *)buf) = data;
    buf += 2;
    return buf;
}


UInt8 *
f_SDPAddShortToBuf (Globals *g, UInt8 *buf, UInt8 type, UInt32 data)
{
    if (data & 0xFFFF0000l) {
        *buf++ = type | SDPDataSize4;
        data = BtLibSdpHToNL (data);
        *((UInt32 *)buf) = data;
        buf += 4;
    }
    else if (data & 0x0000FF00l) {
        *buf++ = type | SDPDataSize2;
        data = BtLibSdpHToNS ((UInt16)data);
        *((UInt16 *)buf) = (UInt16) data;
        buf += 2;
    }
    else {
        *buf++ = type | SDPDataSize1;
        *buf++ = (UInt8) data;
    }
    return buf;
}

UInt8 *
f_SDPAddLongToBuf (Globals *g, UInt8 *buf, UInt8 type,
		   UInt8 *data, UInt32 datasize)
{
    if (datasize <= 0xFF) {
        *buf++ = type | SDPDataSizeLong1;
        *buf++ = (UInt8) datasize;
    }
    else if (datasize <= 0xFFFF) {
        *buf++ = type | SDPDataSizeLong2;
        datasize = BtLibSdpHToNS ((UInt16)datasize);
        *((UInt16 *)buf) = (UInt16) datasize;
        buf += 2;
    }
    else {
        *buf++ = type | SDPDataSizeLong4;
        datasize = BtLibSdpHToNL (datasize);
        *((UInt32 *)buf) = datasize;
        buf += 4;
    }
    MemMove (buf, data, datasize);
    buf += datasize;
    return buf;
}

UInt8 *
f_SDPSInt (Globals *g, UInt8 *buf, Int32 data)
{
    if (data < -0x10000 || data > 0xFFFF) {
        *buf++ = SDPTypeSInt | SDPDataSize4;
        data = BtLibSdpHToNL (data);
        *((Int32 *)buf) = data;
        buf += 4;
    }
    else if (data < -0x100 || data > 0xFF) {
        *buf++ = SDPTypeSInt | SDPDataSize2;
        *((Int16 *)buf) = (Int16) data;
        buf += 2;
    }
    else {
        *buf++ = SDPTypeSInt | SDPDataSize1;
        *buf++ = (Int8) data;
    }
    return buf;
}

UInt8 *
f_SDPUUID (Globals *g, UInt8 *buf, BtLibSdpUuidType *uuid)
{
    switch (uuid->size) {
	case btLibUuidSize128:
	    *buf++ = SDPTypeUUID | SDPDataSize16;
	    MemMove (buf, uuid->UUID, 16);
	    buf += 16;
	    return buf;
	case btLibUuidSize32:
	    *buf++ = SDPTypeUUID | SDPDataSize4;
	    MemMove (buf, uuid->UUID, 4);
	    buf += 4;
	    return buf;
	case btLibUuidSize16:
	    *buf++ = SDPTypeUUID | SDPDataSize2;
	    MemMove (buf, uuid->UUID, 2);
	    buf += 2;
	    return buf;
	default:
	    ErrFatalDisplay ("Invalid UUID size");
	    return buf;
    }
}

UInt8 *
f_SDPSeriesStart (Globals *g, UInt8 *buf, UInt8 type)
{
    *buf++ = type & SDPTypeMask; /* Size filled in later. */
    *buf++ = 0;                  /* To be filled in later. */
    return buf;
}

UInt8 *
f_SDPSeriesEnd (Globals *g, UInt8 *start, UInt8 *end, UInt8 type)
{
    UInt32 length = end - start - 2;
    UInt8 *tag = start;
    UInt8 *size = start + 1;
    DUMP (LogTrace, "SDP Series", start, end - start);
    PANICIF (start > end, "Invalid sequence");
    PANICIF ((*tag & SDPTypeMask) != type, "Invalid sequence %2x/%2x",
             type, *tag & SDPTypeMask);
    if (length <= 0xFF) {
        *tag |= SDPDataSizeLong1;
        *size = (UInt8) length;
        return end;
    }
    else if (end - start <= 0xFFFF) {
        *tag |= SDPDataSizeLong2;
        MemMove (start + 3, start + 2, length);
        *((UInt16*)size) = BtLibSdpHToNS ((UInt16) length);
        return end + 1;
    }
    else {
        *tag |= SDPDataSizeLong4;
        MemMove (start + 5, start + 2, length);
        *((UInt32*)size) = BtLibSdpHToNL (length);
        return end + 3;}
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

