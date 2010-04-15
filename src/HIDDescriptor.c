/**************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: HIDDescriptor.c
 *
 **************************************************************************/

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "HIDDescriptor.h"
#undef PRIVATE_INCLUDE

UInt8 *
f_HIDAddToBufSigned (Globals *g, UInt8 *buf, UInt8 tag, UInt8 type,
                     Int32 data, Boolean forcedata)
{
    UInt32 udata = (UInt32) data;

    if (udata == 0 && !forcedata) {
        *buf++ = HIDPrefix (tag, type, HIDSize0);
    }
    else if (0xFFFFFF80 <= udata || udata <= 0x0000007F) {
        *buf++ = HIDPrefix (tag, type, HIDSize1);
        *buf++ = (UInt8) udata;
    }
    else if (0xFFFF8000 <= udata || udata <= 0x00007FFF) {
        *buf++ = HIDPrefix (tag, type, HIDSize2);
        udata = HIDHToNS ((UInt16) udata);
        *((UInt16 *)buf) = (UInt16) udata;
        buf += 2;
    }
    else {
        *buf++ = HIDPrefix (tag, type, HIDSize4);
        udata = HIDHToNL (udata);
        *((UInt32 *)buf) = udata;
        buf += 4;
    }
    return buf;
}

UInt8 *
f_HIDAddToBufUnsigned (Globals *g, UInt8 *buf, UInt8 tag, UInt8 type,
                       UInt32 data, Boolean forcedata)
{
    if (data == 0 && !forcedata) {
        *buf++ = HIDPrefix (tag, type, HIDSize0);
    }
    else if (data <= 0x000000FF) {
        *buf++ = HIDPrefix (tag, type, HIDSize1);
        *buf++ = (UInt8) data;
    }
    else if (data <= 0x0000FFFF) {
        *buf++ = HIDPrefix (tag, type, HIDSize2);
        data = HIDHToNS ((UInt16) data);
        *((UInt16 *)buf) = (UInt16) data;
        buf += 2;
    }
    else {
        *buf++ = HIDPrefix (tag, type, HIDSize4);
        data = HIDHToNL (data);
        *((UInt32 *)buf) = data;
        buf += 4;
    }
    return buf;
}



UInt8 *
f_HIDDescriptor (Globals *g, UInt8 *b)
{
    /* FIRST REPORT: Keyboard */

    b = HIDUsagePage (b, 0x01); /* Generic Desktop */
    b = HIDUsage (b, 0x06);     /* Keyboard */
    b = HIDCollection (b, HIDCollectionApplication); {
        b = HIDReportID (b, 1);

        /* Modifier state: 8 bits.  */
        b = HIDReportSize (b, 1);
        b = HIDReportCount (b, 8);
        b = HIDUsagePage (b, 0x07); /* Keycodes */
        b = HIDUsageMinimum (b, 224); /* Left Control */
        b = HIDUsageMaximum (b, 231); /* Right GUI */
        b = HIDLogicalMinimum (b, 0);
        b = HIDLogicalMaximum (b, 1);
        b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);

        /* Reserved: 1 byte.  */
        b = HIDReportCount (b, 1);
        b = HIDReportSize (b, 8);
        b = HIDInput (b, HIDConstant);

        /* LED state: 5 bits.  */
        b = HIDReportCount (b, 5);
        b = HIDReportSize (b, 1);
        b = HIDUsagePage (b, 0x08); /* LEDs */
        b = HIDUsageMinimum (b, 1); /* NumLock */
        b = HIDUsageMaximum (b, 5); /* Kana */
        b = HIDOutput (b, HIDData | HIDVariable | HIDAbsolute);

        /* Padding: 3 bits.  */
        b = HIDReportCount (b, 1);
        b = HIDReportSize (b, 3);
        b = HIDOutput (b, HIDConstant);

        /* Keypress state: 6 bytes.  */
        b = HIDReportCount (b, 6);
        b = HIDReportSize (b, 8);
        b = HIDLogicalMinimum (b, 0);
        b = HIDLogicalMaximum (b, 255);
        b = HIDUsagePage (b, 0x07); /* Keycodes */
        b = HIDUsageMinimum (b, 0);
        b = HIDUsageMaximum (b, 255);
        b = HIDInput (b, HIDData | HIDArray);
    }
    b = HIDEndCollection (b);

    /* SECOND REPORT: Mouse */

    b = HIDUsagePage (b, 0x01); /* Generic Desktop */
    b = HIDUsage (b, 0x02);     /* Mouse */
    b = HIDCollection (b, HIDCollectionApplication); {
        b = HIDReportID (b, 2);

        b = HIDUsage (b, 0x01); /* Pointer */
        b = HIDCollection (b, HIDCollectionPhysical); {

            /* First byte: Button state.  */
            b = HIDReportCount (b, 3);
            b = HIDReportSize (b, 1);
            b = HIDUsagePage (b, 0x09); /* Buttons */
            b = HIDUsageMinimum (b, 1);
            b = HIDUsageMaximum (b, 3);
            b = HIDLogicalMinimum (b, 0);
            b = HIDLogicalMaximum (b, 1);
            b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);

            /* Padding.  */
            b = HIDReportCount (b, 5);
            b = HIDReportSize (b, 1);
            b = HIDInput (b, HIDConstant | HIDVariable | HIDAbsolute);

            /* Second byte: coordinates.  */
            b = HIDReportCount (b, 3);
            b = HIDReportSize (b, 8);
            b = HIDUsagePage (b, 0x01); /* Generic Desktop */
            b = HIDUsage (b, 0x30);         /* X */
            b = HIDUsage (b, 0x31);         /* Y */
            b = HIDUsage (b, 0x38);         /* Wheel */
            b = HIDLogicalMinimum (b, -127);
            b = HIDLogicalMaximum (b, 127);
            b = HIDInput (b, HIDData | HIDVariable | HIDRelative);
        }
        b = HIDEndCollection (b);
    }
    b = HIDEndCollection (b);
    
#if 0
    /* THIRD REPORT: Game Pad */
    b = HIDUsagePage (b, 0x01); /* Generic Desktop */
    b = HIDUsage (b, 0x05);     /* Game Pad */
    b = HIDCollection (b, HIDCollectionApplication); {
        b = HIDReportID (b, 3);

        b = HIDUsage (b, 0x01); /* Pointer */
        b = HIDCollection (b, HIDCollectionPhysical); {
            /* First byte: D-Pad.  */
            b = HIDReportSize (b, 1);
            b = HIDReportCount (b, 4);
            b = HIDLogicalMinimum (b, 0);
            b = HIDLogicalMaximum (b, 1);
            b = HIDPhysicalMinimum (b, 0);
            b = HIDPhysicalMaximum (b, 1);
            b = HIDUsage (b, 0x90); /* D-Pad Up */
            b = HIDUsage (b, 0x91); /* D-Pad Down */
            b = HIDUsage (b, 0x93); /* D-Pad Left */
            b = HIDUsage (b, 0x92); /* D-Pad Right */
            b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);

            /* Padding.  */
            b = HIDReportSize (b, 1);
            b = HIDReportCount (b, 4);
            b = HIDInput (b, HIDConstant);

            /* Second byte */

            /* Buttons Start & Select.  */
            b = HIDReportSize (b, 1);
            b = HIDReportCount (b, 2);
            b = HIDUsagePage (b, 0x01); /* General Desktop */
            b = HIDUsage (b, 0x3D);     /* Start */
            b = HIDUsage (b, 0x3E);     /* Select */
            b = HIDUsageMaximum (b, 4);
            b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);

            /* Padding.  */
            b = HIDReportSize (b, 1);
            b = HIDReportCount (b, 2);
            b = HIDInput (b, HIDConstant);

            /* Buttons A, B, X, Y.  */
            b = HIDReportSize (b, 1);
            b = HIDReportCount (b, 4);
            b = HIDUsagePage (b, 0x09); /* Buttons */
            b = HIDUsageMinimum (b, 1);
            b = HIDUsageMaximum (b, 4);
            b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);
        }
        b = HIDEndCollection (b);
    }
    b = HIDEndCollection (b);

    /* FOURTH REPORT: Consumer Control */
    b = HIDUsagePage (b, 0x0C); /* Consumer */
    b = HIDUsage (b, 0x01);     /* Consumer Control */
    b = HIDCollection (b, HIDCollectionApplication); {
        b = HIDReportCount (b, 1);
        b = HIDReportSize (b, 8);
        b = HIDUsagePage (b, 0x0C); /* Consumer Control */
        b = HIDUsage (b, 0xE0);     /* Volume (LC) */
        b = HIDLogicalMinimum (b, 0);
        b = HIDLogicalMaximum (b, 100);
        b = HIDInput (b, HIDData | HIDVariable | HIDAbsolute);
    }
    b = HIDEndCollection (b);

    /* FIFTH REPORT: Battery strength feature.  */
    b = HIDReportID (b, 5);
    b = HIDReportCount (b, 1);
    b = HIDReportSize (b, 8);
    b = HIDUsagePage (b, 0x06);  /* Generic Device Controls */
    b = HIDUsage (b, 0x20);      /* Battery Strength */
    b = HIDLogicalMinimum (b, 0);
    b = HIDLogicalMaximum (b, 100);
    b = HIDFeature (b, HIDVariable | HIDNonLinear | HIDNoPreferred);
#endif
    return b;
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

