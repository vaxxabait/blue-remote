/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: DRM.h
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
#define SECTION DRM_SECTION

#ifdef NORMAL_INCLUDE

typedef struct DRMPreferences {
    /* Version number, set to prefDRMVersionNum.  */
    Int16 version;

    /* The time of the first installation.  */
    UInt32 installtime;

    /* The number of times the application has been started.  */
    UInt32 startups;
    
    /* The last time we nagged the user.  */
    UInt32 nagtime;

    /* The registration code, as entered by the user.  */
    char regcode[9];

    /* SHA1 digest of the above (plus some seasoning).  */
    UInt8 checksum[20];
} DRMPreferences;

#endif


DEFUN0 (void, DRMInit);
DEFUN0 (void, DRMClose);
DEFUN0 (Boolean, DRMAppStart);
DEFUN0 (Boolean, DRMIsRegistered);
DEFUN0 (void, DRMRegisterForm);
