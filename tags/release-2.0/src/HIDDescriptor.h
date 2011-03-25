/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: HIDDescriptor.h
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
#define SECTION HIDDESCRIPTOR_SECTION

DEFUNN (UInt8 *, HIDAddToBufSigned, UInt8 *buf, UInt8 tag, UInt8 type, Int32 data, Boolean forcedata);

DEFUNN (UInt8 *, HIDAddToBufUnsigned, UInt8 *buf, UInt8 tag, UInt8 type, UInt32 data, Boolean forcedata);

/* Generate a HID descriptor describing this device in buffer B.  */
DEFUN1 (UInt8 *, HIDDescriptor, UInt8 *b);


#ifdef NORMAL_INCLUDE

/* HID network order is little-endian, like L2CAP.  */
#define HIDHToNS        BtLibL2CapHToNS
#define HIDHToNL        BtLibL2CapHToNL
#define HIDNToHS        BtLibL2CapNToHS
#define HIDNToHL        BtLibL2CapNToHL

/* Tag values in the HID prefix.  */
/* See type-specific values below.  */
#define HIDTagLong          0xF0    /* Long item. */

#define HIDTagMask          0xF0

/* Type values in the HID prefix.  */
#define HIDTypeMain         0x00
#define HIDTypeGlobal       0x04
#define HIDTypeLocal        0x08
#define HIDTypeLong         0x0C

#define HIDTypeMask         0x0C

/* Size values in the HID prefix.  */
#define HIDSize0        0x00
#define HIDSize1        0x01
#define HIDSize2        0x02
#define HIDSize4        0x03

#define HIDSizeMask     0x03

/* Create a HID prefix byte with the given tag, type and size values.  */
#define HIDPrefix(tag, type, size)  ((tag) | (type) | (size))

/* Constant prefix byte for use with long items.  */
#define HIDPrefixLong   HIDPrefix (HIDTagLong, HIDTypeLong, HIDSize2)


/******************************************************************************
 *
 * Main items.
 *
 *****************************************************************************/

/* Main tags. */
#define HIDTagInput         0x80
#define HIDTagOutput        0x90
#define HIDTagFeature       0xB0
#define HIDTagCollection    0xA0
#define HIDTagEndCollection 0xC0

/* Attributes of input/output/feature items.  OR these together.  */
#define HIDConstant         0x01
#define HIDData             0
#define HIDVariable         0x02
#define HIDArray            0
#define HIDRelative         0x04
#define HIDAbsolute         0
#define HIDWrap             0x08
#define HIDNoWrap           0
#define HIDNonLinear        0x10
#define HIDLinear           0
#define HIDNoPreferred      0x20
#define HIDPreferred        0
#define HIDNullState        0x40
#define HIDNoNullState      0

#define HIDVolatile         0x80    /* Output and Feature only */
#define HIDNonVolatile      0

#define HIDBufferedBytes    0x0100
#define HIDBitField         0

/* Add a HID Input item to buf.  DATA should be constructed by ORing
 * the constants above.  */
#define HIDInput(buf, data)     HIDAddToBufUnsigned (buf, HIDTagInput, HIDTypeMain, data, true)

/* Add a HID Output item to buf.  DATA should be constructed by ORing
 * the constants above.  */
#define HIDOutput(buf, data)    HIDAddToBufUnsigned (buf, HIDTagOutput, HIDTypeMain, data, true)

/* Add a HID Feature item to buf.  DATA should be constructed by ORing
 * the constants above.  */
#define HIDFeature(buf, data)   HIDAddToBufUnsigned (buf, HIDTagFeature, HIDTypeMain, data, true)


/* Collection constants.  Select one.  */
#define HIDCollectionPhysical       0x00    /* Physical (group of axes) */
#define HIDCollectionApplication    0x01    /* Application (mouse, keyboard) */
#define HIDCollectionLogical        0x02    /* Logical (interrelated data)  */
#define HIDCollectionReport         0x03    /* Report */
#define HIDCollectionNamedArray     0x04    /* Named array */
#define HIDCollectionUsageSwitch    0x05    /* Usage switch */
#define HIDCollectionUsageModifier  0x06    /* Usage modifier */

/* Add a Collection item to BUF.  DATA should be one of the constants
 * above.  Collection contents should be added by separate calls
 * later.  */
#define HIDCollection(buf, data)    HIDAddToBufUnsigned (buf, HIDTagCollection, HIDTypeMain, data, true)

/* Add an End Collection item to BUF.  */
#define HIDEndCollection(buf)       HIDAddToBufUnsigned (buf, HIDTagEndCollection, HIDTypeMain, 0, false)


/******************************************************************************
 *
 * Global items.
 *
 *****************************************************************************/

/* Global tags. */
#define HIDTagUsagePage         0x00
#define HIDTagLogicalMinimum    0x10
#define HIDTagLogicalMaximum    0x20
#define HIDTagPhysicalMinimum   0x30
#define HIDTagPhysicalMaximum   0x40
#define HIDTagUnitExponent      0x50
#define HIDTagUnit              0x60
#define HIDTagReportSize        0x70
#define HIDTagReportID          0x80
#define HIDTagReportCount       0x90
#define HIDTagPush              0xA0
#define HIDTagPop               0xB0

/* Add a Usage Page item to BUF.  DATA should be a 16-bit value.  */
#define HIDUsagePage(buf, data) HIDAddToBufUnsigned (buf, HIDTagUsagePage, HIDTypeGlobal, data, true)

/* Add a Logical Minimum item to BUF with DATA.  */
#define HIDLogicalMinimum(buf,data) HIDAddToBufSigned (buf, HIDTagLogicalMinimum, HIDTypeGlobal, data, true)

/* Add a Logical Maximum item to BUF with DATA.  */
#define HIDLogicalMaximum(buf,data) HIDAddToBufSigned (buf, HIDTagLogicalMaximum, HIDTypeGlobal, data, true)

/* Add a Physical Minimym item to BUF with DATA.  */
#define HIDPhysicalMinimum(buf,data) HIDAddToBufSigned (buf, HIDTagPhysicalMinimum, HIDTypeGlobal, data, true)

/* Add a Physical Maximum item to BUF with DATA.  */
#define HIDPhysicalMaximum(buf,data) HIDAddToBufSigned (buf, HIDTagPhysicalMaximum, HIDTypeGlobal, data, true)

/* Add a Unit Exponent item to BUF with DATA.  */
#define HIDUnitExponent(buf,data) HIDAddToBufSigned (buf, HIDTagUnitExponent, HIDTypeGlobal, data, true)

/* Add a Unit item to BUF with DATA.  */
#define HIDUnit(buf,data) HIDAddToBufUnsigned (buf, HIDTagUnit, HIDTypeGlobal, data, true)

/* Add a Report Size item to BUF with DATA.  */
#define HIDReportSize(buf,data) HIDAddToBufUnsigned (buf, HIDTagReportSize, HIDTypeGlobal, data, true)

/* Add a Report ID item to BUF with DATA.  */
#define HIDReportID(buf,data) HIDAddToBufUnsigned (buf, HIDTagReportID, HIDTypeGlobal, data, true)

/* Add a Report Count item to BUF with DATA.  */
#define HIDReportCount(buf,data) HIDAddToBufUnsigned (buf, HIDTagReportCount, HIDTypeGlobal, data, true)

/* Add a Push item to BUF with DATA.  */
#define HIDPush(buf) HIDAddToBufUnsigned (buf, HIDTagPush, HIDTypeGlobal, 0, false)

/* Add a Pop item to BUF with DATA.  */
#define HIDPop(buf) HIDAddToBufUnsigned (buf, HIDTagPop, HIDTypeGlobal, 0, false)


/******************************************************************************
 *
 * Local items.
 *
 *****************************************************************************/

/* Local tags.  */
#define HIDTagUsage             0x00
#define HIDTagUsageMinimum      0x10
#define HIDTagUsageMaximum      0x20
#define HIDTagDesignatorIndex   0x30
#define HIDTagDesignatorMinimum 0x40
#define HIDTagDesignatorMaximum 0x50
#define HIDTagStringIndex       0x70
#define HIDTagStringMinimum     0x80
#define HIDTagStringMaximum     0x90
#define HIDTagDelimiter         0xA0

/* Add a Usage item to BUF with DATA.  */
#define HIDUsage(buf,data) HIDAddToBufUnsigned (buf, HIDTagUsage, HIDTypeLocal, data, true)

/* Add a Usage Minimum item to BUF with DATA.  */
#define HIDUsageMinimum(buf,data) HIDAddToBufUnsigned (buf, HIDTagUsageMinimum, HIDTypeLocal, data, true)

/* Add a Usage Maximum item to BUF with DATA.  */
#define HIDUsageMaximum(buf,data) HIDAddToBufUnsigned (buf, HIDTagUsageMaximum, HIDTypeLocal, data, true)

/* Add a Designator Index item to BUF with DATA.  */
#define HIDDesignatorIndex(buf,data) HIDAddToBufUnsigned (buf, HIDTagDesignatorIndex, HIDTypeLocal, data, true)

/* Add a Designator Minimum item to BUF with DATA.  */
#define HIDDesignatorMinimum(buf,data) HIDAddToBufUnsigned (buf, HIDTagDesignatorMinimum, HIDTypeLocal, data, true)

/* Add a Designator Maximum item to BUF with DATA.  */
#define HIDDesignatorMaximum(buf,data) HIDAddToBufUnsigned (buf, HIDTagDesignatorMaximum, HIDTypeLocal, data, true)

/* Add a String Index item to BUF with DATA.  */
#define HIDStringIndex(buf,data) HIDAddToBufUnsigned (buf, HIDTagStringIndex, HIDTypeLocal, data, true)

/* Add a String Minimum item to BUF with DATA.  */
#define HIDStringMinimum(buf,data) HIDAddToBufUnsigned (buf, HIDTagStringMinimum, HIDTypeLocal, data, true)

/* Add a String Maximum item to BUF with DATA.  */
#define HIDStringMaximum(buf,data) HIDAddToBufUnsigned (buf, HIDTagStringMaximum, HIDTypeLocal, data, true)

/* Add a Delimiter item to BUF with DATA.  */
#define HIDDelimiterOpen(buf) HIDAddToBufUnsigned (buf, HIDTagDelimiter, HIDTypeLocal, 1, true)
#define HIDDelimiterClose(buf) HIDAddToBufUnsigned (buf, HIDTagDelimiter, HIDTypeLocal, 0, true)

#endif  /* NORMAL_INCLUDE */
