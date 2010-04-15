/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Common.h
 *
 ***********************************************************************/

#include "Config.h"

#include <PalmOS.h>
#include <BtLib.h>
#include <TraceMgr.h>
#include <unix_stdarg.h>


#define uint8_t	UInt8			/* Incs/68K/System/Slider.h */
#include <Common/Libraries/Imaging/palmOnePhotoCommon.h>
#include <palmOne_68K.h>

#include <68K/Hs.h>

typedef struct Globals Globals;
#include "Sections.h"

#include "Error.h"

#undef NORMAL_INCLUDE
#define NORMAL_INCLUDE
#include "CoreHeaders.h"
#undef NORMAL_INCLUDE

struct Globals {
    /* True if we are the current application.  */
    Boolean foreground;

    struct MainGlobals *main;
    struct LogGlobals *log;
    struct CmdGlobals *cmd;
    struct BTGlobals *bt;
    struct UnsavedPreferences *unsavedPref;
    struct SavedPreferences *savedPref;
    struct HIDPGlobals *hidp;
    struct UIToolsGlobals *uitools;
    struct DevListGlobals *devlist;
    struct TimerGlobals *timer;
    struct DRMGlobals *drm;
    struct AutoOffGlobals *autooff;

#undef GLOBALS_INCLUDE
#define GLOBALS_INCLUDE
#include "CoreHeaders.h"
#undef GLOBALS_INCLUDE
};

#define g_foreground g->foreground

#include "Magic.g"

// register your own at http://www.palmos.com/dev/creatorid/
#define appFileCreator			'YUC1'
#define appVersionNum			0x01

#define UNUSED	__attribute__ ((__unused__))

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

