/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Main.c
 *
 ***********************************************************************/

#include "Common.h"

#define PRIVATE_INCLUDE
#include "Defun.h"
#if !defined (ENABLE_DEFUN_CHECK)
/* We need to include all core headers in order for Magic.i to see all
 * the function declarations. */
#include "CoreHeaders.h"
#endif
#undef PRIVATE_INCLUDE

#include "BlueRemote_res.h"
#include "AppMenu.h"
#include "MouseForm.h"
#include "KeyboardForm.h"
#include "ConnectForm.h"
#include "HardKeys.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef struct MainGlobals {
    UInt16 switchingToForm;
    UInt32 switchingSince;
    Boolean switched;
} MainGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/

/* Get previously created globals.  */
static Globals *GetGlobals (void)
    DEFAULT_SECTION;

/* Allocate new "global" variables.  */
static Globals *AllocateGlobals (void)
    DEFAULT_SECTION;

static void FreeGlobals (Globals *g)
    DEFAULT_SECTION;

/* Initialize core modules and restore state.  */
static Err AppInit (Globals *g)
    MAIN_SECTION;

/* Initialize UI modules and restore state.  */
static Err AppStart (Globals *g)
    MAIN_SECTION;

/* Close UI modules.  */
static void AppStop (Globals *g)
    MAIN_SECTION;

/* Close core modules and save state.  */
static void AppClose (Globals *g)
    MAIN_SECTION;

/* The event loop of the application.  */
static void AppEventLoop (Globals *g)
    MAIN_SECTION;

/* Handle application-wide events (frmLoadEvents).  */
static Boolean AppHandleEvent (Globals *g, EventType* pEvent)
    MAIN_SECTION;

/* Initialize form FORMID by calling its initailizator.  */
static void AppInitForm (Globals *g, UInt16 formID, FormType *pForm)
    MAIN_SECTION;

static Err AppVersionCheck (Boolean alert)
    DEFAULT_SECTION;


/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#define g_switchingToForm g->main->switchingToForm
#define g_switchingSince g->main->switchingSince
#define g_switched g->main->switched

Globals *globals;
Boolean longDisconnect;
Boolean retry;

enum {
    stDisconnected,
    stWaiting,
    stConnected,
} state;

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/

static Err
ReinitRadio (Globals *g)
{
    Err err;
    
    BTRadioClose();
    err = BTRadioInit();
    if (err) {
	ENQUEUE (CmdExitApp);
	return err;
    }
    
    err = BTListenInit();
    if (err) {
	ERROR ("%s", StrError (err));
	ENQUEUE (CmdExitApp);
	return err;
    }

    return 0;
}


static void
Connect (Globals *g)
{
    Device *d = DevListSelected();
    Err err;
    
    if (!d) {
	SndPlaySystemSound (sndError);
	return;
    }
    
    longDisconnect = true;
    ProgressStart ("Connecting to %s", d->nameStr);
    ProgressSetDetail ("Please wait while BlueRemote is setting up the "
		       "connection to %s.", d->nameStr);
    err = BTConnect();
    if (err) {
	if (err == appErrProgressCanceled)
	    SndPlaySystemSound (sndError);
	else if (err == btLibMeStatusPageTimeout)
	    ERROR ("BlueRemote could not find %s.  "
		   "Make sure the device is nearby, "
		   "has its radio turned on and connectable.",
		   d->nameStr);
	else 
	    ERROR ("%s", StrError (err)); /* TODO */
	SwitchToForm (ConnectForm);
	retry = false;
	state = stDisconnected;
	ProgressStop (true);
	AutoOffEnable (true);
    }
    else {
	SndPlaySystemSound (sndStartUp);
	SwitchToForm (UPREF (currentRemoteFormID));
	longDisconnect = true;
	state = stConnected;
	retry = true;
	ProgressStop (true);
	if (SPREF (disableAutoOff))
	    AutoOffEnable (false);
    }
}


static void
Wait (Globals *g)
{
    Err err;
    
    state = stWaiting;
    ProgressStart ("Waiting for a connection...");
    ProgressSetDetail ("Your Palm is now emulating a Bluetooth keyboard. "
		       "Go to your PC, start a search for Bluetooth "
		       "devices, and connect to it.");
    while (!BTIsConnected()) {
	err = BTWait();
	if (err) {
	    if (err == appErrProgressCanceled) {
		/* Only switch back if canceled.  */
		SndPlaySystemSound (sndError);
		SwitchToForm (ConnectForm);
		state = stDisconnected;
		retry = false;
		ProgressStop (true);
		return;
	    }
	    else {
		ERROR ("%s", StrError (err));
	    }
	}
    }

    SndPlaySystemSound (sndStartUp);
    SwitchToForm (UPREF (currentRemoteFormID));
    longDisconnect = true;
    state = stConnected;
    /* Don't try to reconnect automatically if the connection was
     * originally initiated from the host.  Windows would get
     * confused.  */
    retry = false;

    ProgressStop (true);

    if (SPREF (disableAutoOff))
	AutoOffEnable (false);
}


static void
Disconnect (Globals *g)
{
    retry = false;
    ProgressStart ("Disconnecting");
    BTDisconnect();
    longDisconnect = false;
    state = stDisconnected;
    ProgressStop (longDisconnect);
    AutoOffEnable (true);
    SwitchToForm (ConnectForm);
}


static void
Reconnect (Globals *g)
{
    Device *d = DevListSelected();
    Err err;
    
    if (!d) {
	SndPlaySystemSound (sndError);
	Disconnect (g);
	return;
    }
    
    ProgressStart ("Cleaning up");

    BTDisconnect();
    state = stDisconnected;
			    
    longDisconnect = true;
    ProgressStart ("Reconnecting to %s", d->nameStr);
    ProgressSetDetail ("Please wait while BlueRemote is "
		       "reconnecting to %s.", d->nameStr);
    err = BTConnect();
    if (err) {
	if (err == appErrProgressCanceled)
	    SndPlaySystemSound (sndError);
	else if (err == btLibMeStatusPageTimeout)
	    ERROR ("BlueRemote could not find %s.  "
		   "Make sure the device is nearby, "
		   "has its radio turned on and connectable.",
		   d->nameStr);
	else 
	    ERROR ("%s", StrError (err)); /* TODO */
	SwitchToForm (ConnectForm);
	retry = false;
	state = stDisconnected;
	ProgressStop (true);
	AutoOffEnable (true);
    }
    else {
	SndPlaySystemSound (sndStartUp);
	SwitchToForm (UPREF (currentRemoteFormID));
	longDisconnect = true;
	state = stConnected;
	retry = true;
	ProgressStop (true);
	if (SPREF (disableAutoOff))
	    AutoOffEnable (false);
    }
}



static Globals *
GetGlobals (void)
{
    Globals *g = NULL;
    if (FtrGet (appFileCreator, 1, (UInt32 *)&g))
        return NULL;
    return g;
}



static Globals *
AllocateGlobals (void)
{
    UInt16 cardNo;
    LocalID dbID;
    /* Can't use CheckedMemPtrNew yet. */
    Globals *newg = MemPtrNew (sizeof (Globals));

    ErrFatalDisplayIf (newg == 0, "Out of memory");

    MemPtrSetOwner (newg, 0);
    MemSet (newg, sizeof (Globals), 0);

    newg->main = MemPtrNew (sizeof (MainGlobals));
    ErrFatalDisplayIf (newg->main == 0, "Out of memory");

    MemPtrSetOwner (newg->main, 0);
    MemSet (newg->main, sizeof (MainGlobals), 0);

    SysCurAppDatabase (&cardNo, &dbID);
    MemHandleLock (DmGetResource (sysResTAppCode, 1));
    MemHandleLock (DmGetResource (sysResTAppCode, 2));
    MemHandleLock (DmGetResource (sysResTAppCode, 3));
    DmDatabaseProtect (cardNo, dbID, true);

    FtrSet (appFileCreator, 1, (UInt32)newg);
    return newg;
}



static void
FreeGlobals (Globals *g)
{
    UInt16 cardNo;
    LocalID dbID;

    MemPtrFree (g);
    FtrUnregister (appFileCreator, 1);

    SysCurAppDatabase (&cardNo, &dbID);
    MemHandleUnlock (DmGetResource (sysResTAppCode, 1));
    MemHandleUnlock (DmGetResource (sysResTAppCode, 2));
    MemHandleUnlock (DmGetResource (sysResTAppCode, 3));
    DmDatabaseProtect (cardNo, dbID, false);
}



static Err
AppInit (Globals *g)
{
    Err err;

    g->foreground = true;
    g->formList = NULL;
    g->currentForm = NULL;

    /* Set up function pointer table.  */
#include "Magic.i"

    /* Initialize modules.  */
    f_LogInit (g);
    TRACE ("AppInit");
    f_PrefInit (g);
    f_CmdInit (g);
    f_TimerInit (g);
    f_DRMInit (g);

    if (!DRMAppStart()) {
	ProgressStop (false);
	err = appErrTrialExpired;
	goto error;
    }
    
    f_DevListInit (g);
    f_ProgressInit (g);
    f_HIDPInit (g);
    HardKeyInit();
    f_AutoOffInit (g);
    
    ProgressStart ("Initializing Bluetooth");
    err = f_BTInit (g);
    if (err)
        goto error;

    err = BTListenInit();
    if (err)
        goto error;

    return 0;

error:
    BTClose();
    HardKeyClose();
    AutoOffClose();
    HIDPClose();
    ProgressClose();
    DevListClose();
    DRMClose();
    TimerClose();
    CmdClose();
    PrefClose();
    LogClose();
    return err;
}



static Err
AppStart (Globals *g)
{
    TRACE ("AppStart");
    ConnectInit();
    MouseInit();
    KeyboardInit();

    if (SPREF (autoConnect) && DevListSelected())
	Connect (g);
    else
	SwitchToForm (ConnectForm);
    ProgressStop (false);

    return errNone;
}



static void
AppStop (Globals *g)
{
    TRACE ("AppStop");
    // Close all the open forms.
    FrmCloseAllForms();

    KeyboardClose();
    MouseClose();
    ConnectClose();
}



static void
AppClose (Globals *g)
{
    BTClose();
    AutoOffClose();
    HardKeyClose();
    DRMClose();
    HIDPClose();
    ProgressClose();
    DevListClose();
    TimerClose();
    CmdClose();
    PrefClose();
    LogClose();
}


static void
AppEventLoop (Globals *g)
{
    Err err;
    EventType event;

    TRACE ("AppEventLoop");
    do {

        TimerTick();

        /* Try to get a command from the command queue before sleeping
         * for events.  */
        if (CmdGet (&event)) {
            if (g_switchingToForm && !g_switched) {
                if (g_currentForm->formID == g_switchingToForm) {
                    g_switchingToForm = 0;
                    g_switchingSince = 0;
                }
                else {
                    FrmGotoForm (g_switchingToForm);
                    g_switched = true;
                }
            }
            EvtGetEvent(&event, TimerTimeout());
        }

        LogFlush (false);

        if (BTHandleEvent (&event))
            continue;

        /* formOpenEvents are lost when a popup dialog appears after
         * the old frame is closed but before the new one is opened.
         * (Internal event loops swallow them.  WaitForEvent above has
         * a similar effect.)
         *
         * The Bluetooth module likes to pop up error dialogs in
         * callback functions, and it is not always easy to replace
         * them.  Work around the problem by reissuing FrmGotoForm
         * when the form switch hasn't taken place in two seconds.  */
        if (!Progressing()
            && g_switchingToForm
            && TimGetTicks() - g_switchingSince > 200) {
            DEBUG ("Repeating switch to form %d", g_switchingToForm);
            g_switchingSince = TimGetTicks();
            FrmGotoForm (g_switchingToForm);
        }

        if (g_currentForm->formID == MouseForm
            && MouseFormOverrideEvent (&event))
            continue;

        if (Progressing()) {
            if (ProgressHandleEvent (&event))
                continue;
        }
        else {
            if (HardKeyProcessEvent (true, &event))
                continue;
            if (SysHandleEvent (&event))
                continue;
        }

        if (HardKeyProcessEvent (false, &event))
            continue;

        if (MenuHandleEvent (0, &event, &err))
            continue;

        if (AppHandleEvent (g, &event))
            continue;

        if (FrmDispatchEvent (&event))
            continue;

        if (event.eType == appStopEvent) {
	    ENQUEUE (CmdExitApp);
        }
        else if (event.eType == commandEvent) {
	    Command cmd = EVENT_COMMAND (&event);

	    /* If an incoming connection is received while on the
	       Welcome screen, switch to the Wait screen.  */
	    if (CmdIsAsyncResult (cmd)
		&& CmdAsyncKind (cmd) == AsyncConnect
		&& CmdAsyncErr (cmd) == 0
		&& state == stDisconnected) {
		Wait (g);
		continue;
	    }
	    
            switch (cmd) {
		case CmdInitiateConnect:
		    switch (state) {
			case stDisconnected:
			case stWaiting:
			    Connect (g);
			    break;
			    
			case stConnected:
			    /* Ignore.  */
			    break;
		    }
		    break;
		
		case CmdWaitConnection:
		    switch (state) {
			case stDisconnected:
			    Wait (g);
			    break;
			case stWaiting:
			case stConnected:
			    /* Ignore.  */
			    break;
		    }
		    break;

		case CmdUnplugReceived:
		    switch (state) {
			case stDisconnected:
			    /* Ignore.  */
			    break;
			case stWaiting:
			case stConnected:
			{
			    Device *d = DevListSelected();
			    if (d) {
				ERROR ("%s has unplugged the connection. "
				       "You will have to repeat the "
				       "pairing process to connect to "
				       "%s again.", d->nameStr, d->nameStr);
				DevListDelete (d);
			    }
			    SwitchToForm (ConnectForm);
			    retry = false;
			    state = stDisconnected;
			    ProgressStop (true);
			    AutoOffEnable (true);
			    break;
			}
		    }
		    break;
		case CmdConnectionError:
		    switch (state) {
			case stDisconnected:
			case stWaiting:
			    /* Ignore.  */
			    break;
			case stConnected:
			    SndPlaySystemSound (sndError);
			    if (retry)
				Reconnect (g);
			    else
				Disconnect (g);
			    break;
		    }
		    break;

		case CmdRadioError:
		    switch (state) {
			case stDisconnected:
			    ProgressStart ("Reinitializing radio");
			    ReinitRadio (g);
			    ProgressStop (false);
			    break;

			case stWaiting:
			    INFO ("Reinitializing radio");
			    ReinitRadio (g);
			    break;
				
			case stConnected:
			    ProgressStart ("Reinitializing radio");
			    if (ReinitRadio (g) == 0) {
				if (retry)
				    Reconnect (g);
				else
				    Disconnect (g);
				break;
			    }
		    }
		    break;

		case CmdInitiateDisconnect:
		    switch (state) {
			case stDisconnected:
			    /* Ignore.  */
			    break;
			case stWaiting:
			case stConnected:
			    Disconnect (g);
			    break;
		    }
		    break;

		case CmdProgressCanceled:
		    switch (state) {
			case stWaiting:
			    ProgressStop (true);
			    DEBUG ("Connection canceled");
			    SndPlaySystemSound (sndError);
			    SwitchToForm (ConnectForm);
			    state = stDisconnected;
			    break;
			case stDisconnected:
			case stConnected:
			    /* Ignore.  */
			    break;
		    }
		    break;

		case CmdShowWelcome:
		{
		    int btn;
		    SndPlaySystemSound (sndStartUp);
		    btn = PopupForm
			("Welcome", true, "Welcome to BlueRemote!",
			 "Connect", NULL,
			 "BlueRemote is a Bluetooth remote control "
			 "for your PC's keyboard and mouse.\n"
			 "To begin using BlueRemote, you need to connect "
			 "it to a Bluetooth-capable PC.");
		    if (btn == -1)
			ENQUEUE (CmdExitApp);
		    else
			ENQUEUE (CmdWaitConnection);
		}
		    
		case CmdRedraw:
		    FrmDrawForm (FrmGetActiveForm());
		    break;

		case CmdMouseForm:
		    SwitchToForm (MouseForm);
		    break;

		case CmdKeyboardForm:
		    SwitchToForm (KeyboardForm);
		    break;

		case CmdSwitchForms:
		    if (!g_currentForm)
			break;
		    switch (g_currentForm->formID) {
			case MouseForm:
			    SwitchToForm (KeyboardForm);
			    break;
			case KeyboardForm:
			    SwitchToForm (MouseForm);
			    break;
			default:
			    break;
		    }
		    break;
		default:
		    break;
            }
        }

        if (event.eType == commandEvent)
            CmdExecute (EVENT_COMMAND (&event));

    } while (event.eType != commandEvent
             || EVENT_COMMAND (&event) != CmdExitApp);
}



static Boolean
AppHandleEvent (Globals *g, EventType* pEvent)
{
    UInt16 formId;
    FormType *pForm;

    if (pEvent->eType == frmLoadEvent) {
        /* Load the form resource. */
        TRACE ("frmLoadEvent");
        formId = pEvent->data.frmLoad.formID;

        pForm = FrmInitForm (formId);
        FrmSetActiveForm (pForm);

        AppInitForm (g, formId, pForm);

        return true;
    }

    return false;
}



static void
AppInitForm (Globals *g, UInt16 formID, FormType *pForm)
{
    FormDesc *fd = g_formList;
    TRACE ("AppInitForm %d", formID);
    while (fd) {
        if (fd->formID == formID) {
            g_currentForm = fd;
            if (fd->Init != NULL)
                fd->Init (fd, pForm);
            return;
        }
        fd = fd->next;
    }
    ErrFatalDisplay ("Could not find current form");
}



static Err
AppVersionCheck (Boolean alert)
{
    UInt32 version;

    /* Check for PalmOS 5.x.  */
    FtrGet (sysFtrCreator, sysFtrNumROMVersion, &version);
    if (version < sysMakeROMVersion(5, 0, 0, sysROMStageRelease, 0)) {
        if (alert) FrmAlert (WrongROMVersionAlert);
        return sysErrRomIncompatible;
    }

    /* Check for Bluetooth components.  */
    if (FtrGet (btLibFeatureCreator,
                btLibFeatureVersion,
                &version)) {
        if (alert) FrmAlert (MissingBluetoothAlert);
        return sysErrRomIncompatible;
    }

    return 0;
}

/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/

/* Put FORM in the form list, registering it as a valid form.  */
void
f_RegisterForm (Globals *g, FormDesc *form)
{
    FormDesc *f;
    if (g_formList == NULL) {
        g_formList = form;
        g_formList->next = NULL;
        return;
    }
    for (f = g_formList; f->next != NULL; f = f->next) {
        if (form == f)
            return;  /* Already registered.  */
    }
    f->next = form;
    form->next = NULL;
}

void
f_SwitchToForm (Globals *g, UInt16 formID)
{
    g_switchingToForm = formID;
    g_switchingSince = TimGetTicks();
    g_switched = false;
    // TODO find out why does this cause flickering.
    //FrmGotoForm (formID);
}

void
f_FormDrawn (Globals *g, UInt16 formID)
{
    if (!g_switchingToForm)
        return;
    if (formID != g_switchingToForm) {
        FrmGotoForm (g_switchingToForm);
    }
    else {
        g_switchingToForm = 0;
        g_switchingSince = 0;
    }
}

/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code.
 *              cmdPB - pointer to a structure that is associated with
 *                  the launch code.
 *              launchFlags -  word value providing extra information
 *                  about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err err = errNone;

    err = AppVersionCheck ((launchFlags & sysAppLaunchFlagNewGlobals)
			   && (launchFlags & sysAppLaunchFlagUIApp));
    if (err)
        return err;

    switch (cmd) {
        case sysAppLaunchCmdNormalLaunch:
        {
            Globals *g = GetGlobals();

            if (g == NULL) {
                g = AllocateGlobals();
		g_foreground = true;
		globals = g;
                err = AppInit (g);
                if (err) {
                    FreeGlobals (g);
                    return err;
                }
            }
            else {
                DEBUG ("Coming to foreground");
            }
	    
            if (AppStart (g) == 0)
		AppEventLoop (g);
            AppStop (g);
#ifdef ENABLE_BACKGROUND_MODE
            if (BTIsConnected()) {
                g_foreground = false;
                DEBUG ("Going to background");
            }
            else
#endif
            {
                DEBUG ("Closing application");
                AppClose (g);
                FreeGlobals (g);
            }
            break;
        }
        default:
            break;
    }

    return err;
}

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

