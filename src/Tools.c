/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Tools.c
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

#define PRIVATE_INCLUDE
#include "Defun.h"
#include "Tools.h"
#undef PRIVATE_INCLUDE

DeviceType
f_ToolsGetDeviceType (Globals *g)
{
    UInt32 company = 0;
    UInt32 device = 0;
    FtrGet (sysFtrCreator, sysFtrNumOEMCompanyID, &company);
    FtrGet (sysFtrCreator, sysFtrNumOEMDeviceID, &device);
    if (company == kPalmCompanyIDPalm
	|| company == kPalmCompanyIDOldPalm) {
        /* Palm devices. */
        switch (device) {
	    case kPalmOneDeviceIDZire:
		return DevPalmZire;
	    case kPalmOneDeviceIDTungstenT:
		return DevPalmTungstenT;
	    case kPalmOneDeviceIDTungstenW:
		return DevPalmTungstenW;
	    case kPalmOneDeviceIDTungstenC:
		return DevPalmTungstenC;
	    case kPalmOneDeviceIDZire71:
		return DevPalmZire71;
	    case kPalmOneDeviceIDTungstenT2:
		return DevPalmTungstenT2;
	    case kPalmOneDeviceIDTungstenT3:
		return DevPalmTungstenT3;
	    case kPalmOneDeviceIDTungstenTE:
		return DevPalmTungstenE;
	    case kPalmOneDeviceIDZire21:
		return DevPalmZire21;
	    case kPalmOneDeviceIDZire31:
		return DevPalmZire31;
	    case kPalmOneDeviceIDZire72:
		return DevPalmZire72;
	    case kPalmOneDeviceIDTungstenT5:
		return DevPalmTungstenT5;
	    case kPalmOneDeviceIDTungstenE2:
		return DevPalmTungstenE2;
	    case kPalmOneDeviceIDTungstenX:
		return DevPalmLifeDrive;
	    case kPalmDeviceIDTX:
		return DevPalmTX;
	    case kPalmDeviceIDZ22:
		return DevPalmZ22;
	    case kPalmOneDeviceIDVentura:
		return DevPalmTreo700p;
	    default:
		return DevUnknownDevice;
        }
    }
    else if (company == kPalmCompanyIDHandspring) {
        /* Handspring devices.  */
        switch (device) {
	    case kPalmOneDeviceIDTreo600:
	    case kPalmOneDeviceIDTreo600Sim:
		return DevPalmTreo600;

	    case kPalmOneDeviceIDTreo650:
	    case kPalmOneDeviceIDTreo650Sim:
		return DevPalmTreo650;

	    default:
		return DevUnknownDevice;
        }
    }
    return DevUnknownDevice;
}

DeviceFlags
f_ToolsGetDeviceFlags (Globals *g)
{
    DeviceFlags flags = 0;
    DeviceType type = f_ToolsGetDeviceType (g);
    UInt32 os = 0;
    UInt32 value = 0;

    /* Get OS version.  */
    FtrGet (sysFtrCreator, sysFtrNumROMVersion, &os);

    /* Supported?  */
    switch (type) {
	case DevPalmTungstenT:	/* The T was similar enough to the T2
				 * to consider them equivalent.  */
	case DevPalmTungstenT2:
	case DevPalmTreo650:
	    flags |= DeviceFlagSupported;
	    break;
	default:
	    break;
    }
    
    /* Smart phone? */
    if (FtrGet (hsFtrCreator, hsFtrIDVersion, &value) == 0) {
	/* As per the Palm FAQ.  */
	flags |= DeviceFlagSmartPhone;
    }

    /* Broken error handling in the Bluetooth Library? */
    if (os < sysMakeROMVersion (5, 4, 0, sysROMStageRelease, 0))
	/* I assume the bug was fixed in the 5.4 series; it does not
	   appear on the Treo 650.  */
	flags |= DeviceFlagBluetoothCrashOnLinkError;

    return flags;
}

MemPtr
f_CheckedMemPtrNew (Globals *g, UInt32 size, Boolean system)
{
    MemPtr result = MemPtrNew (size);
    if (result == 0)
        ErrFatalDisplay ("Out of memory");
#ifdef ENABLE_BACKGROUND_MODE
    if (system)
        MemHandleSetOwner (result, 0);
#endif
    MemSet (result, size, 0);
    return result;
}

MemHandle
f_CheckedMemHandleNew (Globals *g, UInt32 size, Boolean system)
{
    MemHandle result = MemHandleNew (size);
    char *ptr;

    if (result == 0)
        ErrFatalDisplay ("Out of memory");
#ifdef ENABLE_BACKGROUND_MODE
    if (system)
        MemHandleSetOwner (result, 0);
#endif
    ptr = MemHandleLock (result);
    MemSet (ptr, size, 0);
    MemPtrUnlock (ptr);
    MemHeapCheck (MemHandleHeapID (result));
    MemHeapScramble (MemHandleHeapID (result));

    return result;
}

void
f_CheckedMemHandleResize (Globals *g, MemHandle handle, UInt32 size)
{
    Err err = MemHandleResize (handle, size);
    ErrFatalDisplayIf (err == memErrNotEnoughSpace, "Out of memory");
    ErrFatalDisplayIf (err == memErrInvalidParam, "Invalid parameter");
    ErrFatalDisplayIf (err == memErrChunkLocked,
		       "Can't resize locked handle");
    ErrFatalDisplayIf (err, "Unknown error resizing handle");
}


char *
f_StrError (Globals *g, Err err)
{
    static char buf[256];
    return SysErrString (err, buf, 256);
}
#ifdef ENABLE_BACKGROUND_MODE
#error StrError is currently broken in background mode
#endif
