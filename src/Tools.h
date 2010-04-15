/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Tools.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION TOOLS_SECTION

#ifdef NORMAL_INCLUDE

typedef enum {
    DevUnknownDevice,
    DevPalmZire,
    DevPalmTungstenT,
    DevPalmTungstenW,
    DevPalmTungstenC,
    DevPalmZire71,
    DevPalmTungstenT2,
    DevPalmTungstenT3,
    DevPalmTungstenE,
    DevPalmZire21,
    DevPalmTreo600,
    DevPalmZire31,
    DevPalmZire72,
    DevPalmTungstenT5,
    DevPalmTreo650,
    DevPalmTungstenE2,
    DevPalmLifeDrive,
    DevPalmTX,
    DevPalmZ22,
    DevPalmTreo700p
} DeviceType;

typedef UInt32 DeviceFlags;


/* If this bit is set, we can directly support the device.
 * Otherwise we don't have it, so we must support it by guessing.  */
#define DeviceFlagSupported			1

/* If this bit is set in DeviceFlags, then the device is a smart phone
 * such as Palm's Treo family.  */
#define DeviceFlagSmartPhone			2

/* If this bit is set in DeviceFlags, then the device has a broken
 * Bluetooth library that crashes in EvtGetEvent after an ACL link
 * error.  This can be worked around by reinitializing the Bluetooth
 * Library immediately after the error.  */
#define DeviceFlagBluetoothCrashOnLinkError	4

#define MINUTES	60L
#define HOURS (60 * MINUTES)
#define DAYS (24 * HOURS)

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))

static inline UInt16
SWAP16 (UInt16 x) 
{
    return (x >> 8) | (x << 8);
}

static inline UInt32
SWAP32 (UInt32 x) 
{
    return ((UInt32) SWAP16 (x) << 16) | SWAP16 (x >> 16);
}

static inline UInt32
ROL (UInt32 x, UInt16 shift) 
{
    return (x << shift) | (x >> (32 - shift));
}

static inline UInt32
ROR (UInt32 x, UInt16 shift)
{
    return (x >> shift) | (x << (32 - shift));
}


#if CPU_TYPE == CPU_68K
#define CPU2BIG16(x) (x)
#define CPU2BIG32(x) (x)
#define CPU2LITTLE16(x) SWAP16 (x)
#define CPU2LITTLE32(x) SWAP32 (x)

#define BIG2CPU16(x) (x)
#define BIG2CPU32(x) (x)
#define LITTLE2CPU16(x) SWAP16 (x)
#define LITTLE2CPU32(x) SWAP32 (x)

#else
#error Unexpected CPU type.
#endif /* CPU_TYPE */
#endif  /* NORMAL_INCLUDE */

/* Returns the device type.  */
DEFUN0 (DeviceType, ToolsGetDeviceType);

/* Returns the device flags.  */
DEFUN0 (DeviceFlags, ToolsGetDeviceFlags);

DEFUN2 (MemPtr, CheckedMemPtrNew, UInt32 size, Boolean system);
DEFUN2 (MemHandle, CheckedMemHandleNew, UInt32 size, Boolean system);
DEFUN2 (void, CheckedMemHandleResize, MemHandle handle, UInt32 size);

/* Return a string describing error code ERR.  */
DEFUN1 (char *, StrError, Err err);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

