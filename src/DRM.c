/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: DRM.c
 *
 ***********************************************************************/

#include "Common.h"

#include <DLServer.h>
#include <FrmGlue.h>

#include "BlueRemote_res.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/* Don't bother the user with registration forms until he has
 * used the application this many times.  */
#define DRM_INITIAL_STARTUPS	2

/* Don't bother the user with registration forms until this amount of
 * time has elapsed since the first installation.  */
#define DRM_INITIAL_TIMEOUT	(10 * MINUTES)

/* Don't nag the user more frequently than this amount of time.  */
#define DRM_NAG_FREQUENCY	(10 * MINUTES)

/* The software will require a registration code to work after this
 * amount of time has elapsed since the first installation.  */
#define DRM_EXPIRATION		(10 * DAYS)


/* Application name for the purposes of the registration code check.  */
#define APPNAME "BlueRemote"

/* Random passphrase to concatenate with the HotSync id.  */
#define PASSPHRASE "k9ssGFsKL6oIauPLaX5t3xkNLTYMsWrP"

/* The key for generating the checksum value embedded in the regcode.  */
#define KEY 0x7A16892CUL

/* Strings for decoding the digits in the registration code.  */
#define LINE0 "0123456789ABCDEF"
#define LINE1 "HJKLMNPQRTUVWXYZ"

/* This passphrase is concatenated with the runtime DRM state to get
 * the checksum.  */
#define RUNTIME	"6a8jytpI2NkEDD2IhiJM4GNozKBxpU2w"


/* Preferences-related constants.  */
#define appDRMCreator		'YUC2'
#define prefDRMID		0x00
#define prefDRMVersionNum	0x01


/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef enum {
    regOK,			/* Valid registration code */
    regWrongValue,		/* Wrong HotSync id, probably a regcode */
    regWrongChecksum,		/* Wrong checksum, probably not a regcode */
    regFormatError,		/* Invalid regcode format */
} RegStatus;

typedef struct DRMGlobals {
    DRMPreferences drm;
} DRMGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

static int IndexOf (Globals *g, char *str, char c)
    DRM_SECTION;

static void Cleanup (Globals *g, char *str)
    DRM_SECTION;

static UInt32 GetValue (Globals *g, char *id)
    DRM_SECTION;

static UInt8 GetChecksum (Globals *g, UInt32 val)
    DRM_SECTION;

static Boolean Verify (Globals *g, char *id)
    DRM_SECTION;

static RegStatus CheckCode (Globals *g) 
    DRM_SECTION;

#if 0
static RegStatus QuickCheckCode (Globals *g)
    DRM_SECTION;
#endif

static void DRMPrefChecksum (Globals *g, DRMPreferences *pref,
			     UInt8 *checksum)
    DRM_SECTION;

static Boolean VerifyPref (Globals *g, DRMPreferences *pref)
    DRM_SECTION;

static void SavePref (Globals *g)
    DRM_SECTION;

static void LoadPref (Globals *g)
    DRM_SECTION;

static Boolean EmptyFormEventHandler (EventType *e)
    DRM_SECTION;
    
/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_drm g->drm->drm

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static int
IndexOf (Globals *g, char *str, char c)
{
    int i;
    for (i = 0; str[i] != 0; i++) {
	if (str[i] == c)
	    return i;
    }
    return -1;
}

static void
Cleanup (Globals *g, char *str)
{
    char *p;
    char *q = str;
    for (p = str; *p != 0; p++) {
	UInt8 c = *p;
	if (c < 33		/* Low control characters and space */
	    || c == 0x7F	/* DEL */
	    || (c >= 0x80 && c <= 0x9F) /* High control characters */
	    || (c >= 0xA0 && c <= 0xBF) /* Fancy symbols */
	    || c == 0xD7		/* Multiplication sign */
	    || c == 0xF7)		/* Division sign */
	    continue;

	/* Replacements: */
	if (c == '`') c = '\'';
	if (c == '_') c = '-';

	/* Convert to uppercase (ASCII only) */
	if (c >= 'a' && c <= 'z') c -= 'a' - 'A';

	*q++ = c;
    }
    if (q - str > 32)
	q = str + 32;
    *q = 0;
}

static UInt32
GetValue (Globals *g, char *id)
{
    SHA1State *state;
    UInt32 hash[5];
    UInt32 val;
    int i;
    
    state = SHA1Create();
    SHA1Append (state, APPNAME, StrLen (APPNAME));
    SHA1Append (state, "_", 1);
    SHA1Append (state, PASSPHRASE, StrLen (PASSPHRASE));
    SHA1Append (state, "_", 1);
    Cleanup (g, id);
    SHA1Append (state, id, StrLen (id));
    SHA1Append (state, "_R", 2);
    SHA1Finish (state, (UInt8*) hash);

    val = 0;
    for (i = 0; i < 5; i++)
	val ^= BIG2CPU32 (hash[i]);
    return val;
}

static UInt8
GetChecksum (Globals *g, UInt32 val)
{
    /* Produce an encoded checksum that is useful for detecting
       HotSync name mismatches. */
    UInt32 key = val ^ KEY;
    UInt8 checksum = (key ^ (key >> 8) ^ (key >> 16) ^ (key >> 24));
    return checksum;
}

static RegStatus
Verify (Globals *g, char *id) 
{
    char *p;
    UInt8 checksum = 0;
    UInt32 val = 0;

    if (StrLen (g_drm.regcode) != 8) {
	DEBUG ("Invalid registration code length");
	return regFormatError;
    }

    for (p = g_drm.regcode; *p != 0; p++) {
	UInt8 c = *p;
	int i0, i1;
	int v;
	
	if (c == 'O') c = '0';
	if (c == 'I') c = '1';
	if (c == 'G') c = '6';
	if (c == 'S') c = '5';
	i0 = IndexOf (g, LINE0, c);
	i1 = IndexOf (g, LINE1, c);

	if (i0 != -1) {
	    v = i0;
	    checksum <<= 1;
	}
	else if (i1 != -1) {
	    v = i1;
	    checksum <<= 1;
	    checksum++;
	}
	else {
	    DEBUG ("Invalid character in registration code");
	    return regFormatError;
	}
	
	if (v > 15 || v < 0) {
	    DEBUG ("Invalid digit value in registration code");
	    return regFormatError;
	}

	val <<= 4;
	val |= v & 0x0F;
    }

    if (checksum != GetChecksum (g, val)) {
	DEBUG ("Checksum mismatch");
	return regWrongChecksum;
    }

    if (id != NULL && val != GetValue (g, id)) {
	DEBUG ("Value mismatch");
	return regWrongValue;
    }

    return regOK;
}


static RegStatus
CheckCode (Globals *g) 
{
    Char id[dlkUserNameBufSize];
    DlkGetSyncInfo (NULL, NULL, NULL, id, NULL, NULL);
    return Verify (g, id);
}

#if 0
static RegStatus
QuickCheckCode (Globals *g)
{
    return Verify (g, NULL);
}
#endif

static void
DRMPrefChecksum (Globals *g, DRMPreferences *pref, UInt8 *checksum)
{
    Char id[dlkUserNameBufSize];
    SHA1State *state = SHA1Create();
    /* First, take everything in the struct before the checksum.  */
    SHA1Append (state,
		(UInt8 *)pref,
		((UInt8*)&pref->checksum) - ((UInt8*)pref));
    /* Next, add the runtime passphrase.  */
    SHA1Append (state, RUNTIME, StrLen (RUNTIME));
    /* Finally, close the bit sequence with a copy of the HotSync id.  */
    DlkGetSyncInfo (NULL, NULL, NULL, id, NULL, NULL);
    SHA1Append (state, id, StrLen (id));
    SHA1Finish (state, checksum);
}


static Boolean
VerifyPref (Globals *g, DRMPreferences *pref)
{
    UInt8 digest[20];
    DRMPrefChecksum (g, pref, digest);
    return !MemCmp (digest, pref->checksum, 20);
}

static void
SavePref (Globals *g) 
{
    void *feature;
    Err err;
    /* Calculate checksum.  */
    DRMPrefChecksum (g, &g_drm, g_drm.checksum);

    /* Set saved preferences.  */
    PrefSetAppPreferences (appDRMCreator, prefDRMID, prefDRMVersionNum,
			   &g_drm, sizeof (DRMPreferences), true);

    /* Set feature.  */
    err = FtrGet (appDRMCreator, prefDRMID, (UInt32*) &feature);
    if (err == ftrErrNoSuchFeature)
	FtrPtrNew (appDRMCreator, prefDRMID,
		   sizeof (DRMPreferences), &feature);
    DmWrite (feature, 0, &g_drm, sizeof (DRMPreferences));
}


static void
LoadPref (Globals *g)
{
    Int16 version;
    UInt16 size = sizeof (DRMPreferences);
    DRMPreferences *feature;
    Err err;

    MemSet (&g_drm, sizeof (DRMPreferences), 0);
    version = PrefGetAppPreferences (appDRMCreator, prefDRMID,
				     &g_drm, &size, true);
    err = FtrGet (appDRMCreator, prefDRMID, (UInt32*) &feature);

    if (version != prefDRMVersionNum || !VerifyPref (g, &g_drm))
	version = noPreferenceFound;

    if (err == ftrErrNoSuchFeature || !VerifyPref (g, feature))
	err = ftrErrNoSuchFeature;
    
    if (version == noPreferenceFound && err == ftrErrNoSuchFeature) {
	DEBUG ("Installing DRM info");
	g_drm.version = prefDRMVersionNum;
	g_drm.installtime = TimGetSeconds();
	g_drm.startups = 0;
	g_drm.nagtime = 0;
	MemSet (g_drm.regcode, 8, 0);
	SavePref (g);
    }
    else if (version != noPreferenceFound && err == ftrErrNoSuchFeature) {
	DEBUG ("DRM feature not found, probably after reset");
	SavePref (g);
    }
    else if (version == noPreferenceFound && err != ftrErrNoSuchFeature) {
	DEBUG ("DRM feature found, but no preferences");
	MemMove (&g_drm, feature, sizeof (DRMPreferences));
	SavePref (g);
    }
    else {			/* Both are present. */
	if (MemCmp (&g_drm, feature, sizeof (DRMPreferences))) {
	    DEBUG ("DRM feature does not match preferences");
	    g_drm.installtime = MIN (g_drm.installtime,
				     feature->installtime);
	    SavePref (g);
	}
    }

    ASSERT (VerifyPref (g, &g_drm));
}

static Boolean
EmptyFormEventHandler (EventType *e)
{
    return false;
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

void
f_DRMInit (Globals *g) 
{
    if (g->drm)
	f_DRMClose (g);
    g->drm = CheckedMemPtrNew (sizeof (DRMGlobals), true);
    LoadPref (g);
}

void
f_DRMClose (Globals *g)
{
    if (!g->drm)
	return;
    MemPtrFree (g->drm);
}

Boolean
f_DRMAppStart (Globals *g)
{
    UInt32 left;
    ASSERT (VerifyPref (g, &g_drm));

    /* Don't bother the user if he has payed us.  */
    if (f_DRMIsRegistered (g))
	return true;

    g_drm.startups++;
    SavePref (g);

    /* Don't bother the user on the first few startups.  */
    if (g_drm.startups <= DRM_INITIAL_STARTUPS)
	return true;

    if (g_drm.installtime + DRM_EXPIRATION < TimGetSeconds()) {
	/* Trial period expired.  */
	int btn;
	SndPlaySystemSound (sndError);
	btn = PopupForm ("Trial Period Expired", false,
			 NULL, "Register...", "Exit",
			 "Thank you for trying BlueRemote!\n"
			 "I hope you had as much fun using this application "
			 "as I had developing it.\n"
			 "Unfortunately this trial version has expired. "
			 "Please consider purchasing a registration "
			 "code at " APP_URL " to continue using "
			 "BlueRemote.");
	if (btn == -1)		/* Exit */
	    return false;
	if (btn == 1)
	    f_DRMRegisterForm (g);
	
	if (f_DRMIsRegistered (g))
	    return true;
	else {
	    ENQUEUE (CmdExitApp);
	    return false;
	}
    }

#if 0
    /* Don't bother the user on the first few days.  */
    if (g_drm.installtime + DRM_INITIAL_TIMEOUT > TimGetSeconds())
	return true;
#endif

    /* Don't bother the user too frequently.  */
    if (g_drm.nagtime < TimGetSeconds()
	&& g_drm.nagtime + DRM_NAG_FREQUENCY > TimGetSeconds())
	return true;

    left = DRM_EXPIRATION - (TimGetSeconds() - g_drm.installtime);
    if (left < DAYS) {
	int btn;
	SndPlaySystemSound (sndWarning);
	btn = PopupForm
	    ("Please Register", false, NULL, "Not Now", "Register...",
	     "Thank you for trying BlueRemote!\n"
	     "This is the last day of your free trial period.\n"
	     "If you like BlueRemote, please consider "
	     "purchasing a registration code at " APP_URL ".");
	if (btn == -1)
	    return false;
	if (btn == 2)
	    f_DRMRegisterForm (g);
    }
    else {
	int btn;
	SndPlaySystemSound (sndWarning);
	btn = PopupForm
	    ("Please Register", false, NULL, "Not Now", "Register...",
	     "Thank you for trying BlueRemote! "
	     "I hope you enjoy using it.\n"
	     "This free trial version will work for %ld more days.\n"
	     "If you like BlueRemote, please consider "
	     "purchasing a registration code at " APP_URL ".",
	     left / DAYS);
	if (btn == -1)
	    return false;
	if (btn == 2)
	    f_DRMRegisterForm (g);
    }
    g_drm.nagtime = TimGetSeconds();
    SavePref (g);
    return true;
}

Boolean
f_DRMIsRegistered (Globals *g)
{
    return CheckCode (g) == regOK;
}

void
f_DRMRegisterForm (Globals *g)
{
    FormActiveStateType frmstate;
    FormType *f;
    FieldType *rc;
    Boolean registered = f_DRMIsRegistered (g);
    Boolean exit, newcode;
    EventType event;
    Char id[dlkUserNameBufSize];
    MemHandle regcode = CheckedMemHandleNew (9, false);
    
    FrmSaveActiveState (&frmstate);

    /* Create the new form.  */
    f = FrmInitForm (registered ? RegisteredForm : RegistrationForm);

    /* Add the HotSync id label.  */
    {
	Coord x;
	FontID old;
	DlkGetSyncInfo (NULL, NULL, NULL, id, NULL, NULL);
	if (StrLen (id) == 0) {
	    SndPlaySystemSound (sndError);
	    PopupForm ("Registration Problem", false,
		       NULL, "OK", NULL,
		       "You have to set a HotSync ID before registering "
		       APPNAME ".\nThe Palm Desktop software that came with "
		       "your device automatically sets a HotSync ID when "
		       "you first synchronize your device with your PC.");
	    return;
	}
	old = FntSetFont (largeBoldFont);
	x = (156 - FntCharsWidth (id, StrLen (id))) / 2;
	if (x < 0) x = 0;
	FntSetFont (old);
	FrmNewLabel (&f, 13, id, x, (registered ? 79 : 58), largeBoldFont);
    }

    /* Get a pointer to the registration code field.  */
    {
	UInt16 idx = FrmGetObjectIndex (f, RegistrationRegCodeField);
	ASSERT (idx != frmInvalidObjectId);
	rc = FrmGetObjectPtr (f, idx);
	ASSERT (rc);
    }

    /* Set the text of the registration code field.  */
    {
	MemHandle oldH = FldGetTextHandle (rc);
	char *p = MemHandleLock (regcode);
	StrNCopy (p, g_drm.regcode, 9);
	p[8] = 0;
	MemPtrUnlock (p);
	FldSetTextHandle (rc, regcode);
	if (oldH)
	    MemHandleFree (oldH);
    }

    /* Activate the form.  */
    FrmSetEventHandler (f, EmptyFormEventHandler);
    FrmSetActiveForm (f);
    if (FrmGlueNavIsSupported()) {
	FrmSetNavState (f, kFrmNavStateFlagsObjectFocusMode);
    }
    if (!registered) {
	FrmGlueNavObjectTakeFocus (f, FrmGetObjectIndexFromPtr (f, rc));
    }
    
    /* Draw the form.  */
    FrmDrawForm (f);

    /* Grab the focus and set Caps Lock on.  */
    FrmSetFocus (f, FrmGetObjectIndexFromPtr (f, rc));
    GrfSetState (true, false, false);
    
    exit = false;
    newcode = false;    
    do {
	EvtGetEvent (&event, evtWaitForever);
	if (SysHandleEvent (&event))
	    continue;
	switch (event.eType) {
	    case appStopEvent:
		ENQUEUE (CmdExitApp);
		exit = true;
		continue;

	    case ctlSelectEvent:
		switch (event.data.ctlSelect.controlID) {
		    case 1000:	/* Register */
			if (!registered) {
			    char *p = MemHandleLock (regcode);
			    p[8] = 0;
			    Cleanup (g, p);
			    MemSet (g_drm.regcode, 9, 0);
			    StrNCopy (g_drm.regcode, p, 9);
			    SavePref (g);
			    MemPtrUnlock (p);
			    newcode = true;
			}
			exit = true;
			continue;
			
		    case 1001:	/* Cancel/OK */
			exit = true;
			continue;
		}
		
	    default:
		break;
		
	}

	if (FrmDispatchEvent (&event))
	    continue;
    } while (!exit);
    
    GrfSetState (false, false, false);
    FrmEraseForm (f);
    FrmRestoreActiveState (&frmstate);
    FrmDeleteForm (f);

    if (registered || !newcode)
	return;
    if (f_DRMIsRegistered (g)) {
	SndPlaySystemSound (sndConfirmation);
	PopupForm ("Thank You!", false, NULL, "OK", NULL,
		   "Thank you very much for registering BlueRemote!\n"
		   "You are now free to use it as long as you like, "
		   "and you are entitled to free upgrades for at "
		   "least a year.\nThanks again for your support!");
    }
    else {
	RegStatus st = CheckCode (g);
	char *msg = NULL;
	int btn;
	ASSERT (st != regOK);
	MemSet (g_drm.regcode, 9, 0);
	SavePref (g);
	
	switch (st) {
	    case regWrongValue:
		msg = "The code you entered is a valid registration code, "
		    "but it does not match your HotSync ID.\n"
		    "If you have changed your HotSync ID since purchasing "
		    APPNAME ", please write to " APP_SUPPORT
		    " describing the problem.";
		break;
	    case regWrongChecksum:
		msg = "The code you entered is not a valid registration "
		    "code for this application.\n"
		    "Please make sure that you enter the code exactly "
		    "as it was sent to you.  An example registration "
		    "code is LK8CVB61.";
		break;
	    case regFormatError:
		msg = "The registration code you entered is of the wrong "
		    "format, or it contains invalid characters.\n"
		    "Please make sure that you enter the code exactly "
		    "as it was sent to you.  An example registration "
		    "code is LK8CVB61.";
		break;
	    case regOK:
	    default:
		ASSERT (false);
	}
	SndPlaySystemSound (sndError);
	btn = PopupForm ("Registration Problem", false, NULL,
			 "Cancel", "Try Again", "%s", msg);
	if (btn == 2) {
	    f_DRMRegisterForm (g);
	}
    }
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

