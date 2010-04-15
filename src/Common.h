/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Common.h
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

