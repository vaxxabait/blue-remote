/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: DRM.h
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
