/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Log.c
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
#include "Log.h"
#undef PRIVATE_INCLUDE

#include "BlueRemote_res.h"

/***********************************************************************
 *
 *  CONSTANTS
 *
 ***********************************************************************/

#define LOGFLUSH_PERIOD     500

#define LOGQUEUE_SIZE       100

#define BUFSIZE             512

#define LOGFILENAME         "BlueRemoteLog"

#define FILESIZE_LIMIT      (1024L * 1024L)

/***********************************************************************
 *
 *  TYPES
 *
 ***********************************************************************/

typedef char *charptr;

typedef struct LogLine {
    UInt32 timestamp;
    UInt8 severity;
    char msg[0];
} LogLine;

typedef struct LogGlobals {
#ifdef ENABLE_LOGDISPLAY
    /* The log queue.  */
    MsgQueuePtr queue;

    Boolean needsRedraw;
#endif

#ifdef ENABLE_LOGFILE
    /* File handle of the trace log stream.  */
    FileHand file;
    UInt32 filesize;
#endif

    /* The time of the next sync.  */
    UInt32 nextsync;

    /* Preallocated buffer for constructing log messages.  */
    char buf[BUFSIZE];

    /* Used to prevent messing up the buffer by recursive logging.  */
    Boolean lock;

    /* Used to detect buffer overflows.  */
    UInt16 canary;
} LogGlobals;

/***********************************************************************
 *
 *  STATIC FUNCTION DECLARATIONS
 *
 ***********************************************************************/


#ifdef ENABLE_LOGDISPLAY
static void QueueCallback (Globals *g, MsgQueuePtr queue, Boolean full)
    LOG_SECTION;

static void DrawLog (Globals *g, LogLine *l)
    LOG_SECTION;
#endif

static void AddMsg (Globals *g, char *file, int line,
                    UInt8 severity, MemHandle id)
    LOG_SECTION;

static void CanaryCheck (Globals *g)
    LOG_SECTION;

/***********************************************************************
 *
 *  GLOBAL VARIABLES
 *
 ***********************************************************************/

#ifdef ENABLE_LOGDISPLAY
#define g_queue log->queue
#define g_needsRedraw log->needsRedraw
#endif

#ifdef ENABLE_LOGFILE
#define g_file      log->file
#define g_filesize  log->filesize
#endif

#define g_nextsync log->nextsync
#define g_buf log->buf
#define g_lock log->lock
#define g_canary log->canary

/***********************************************************************
 *
 *  STATIC FUNCTION DEFINITIONS
 *
 ***********************************************************************/


#ifdef ENABLE_LOGDISPLAY
static void
QueueCallback (Globals *g, MsgQueuePtr queue, Boolean full)
{
    if (!g || !g->log) return;
    f_LogFlush (g, full);
}



static void
DrawLog (Globals *g, LogLine *l)
{
    RectangleType r2 = { { 1, 1 }, { 158, 12 } };
    RGBColorType oldrgb;
    RGBColorType newrgb;

    newrgb.index = 0;
    switch (l->severity) {
	case LogTrace:
	default:                    /* White */
	    newrgb.r = 255;
	    newrgb.g = 255;
	    newrgb.b = 255;
	    break;
	case LogDebug:              /* Light blue */
	    newrgb.r = 128;
	    newrgb.g = 128;
	    newrgb.b = 255;
	    break;
	case LogInfo:               /* Green */
	    newrgb.r = 0;
	    newrgb.g = 255;
	    newrgb.b = 0;
	    break;
	case LogWarn:               /* Orange */
	    newrgb.r = 255;
	    newrgb.g = 128;
	    newrgb.b = 0;
	    break;
	case LogError:
	case LogPanic:              /* Red */
	    newrgb.r = 255;
	    newrgb.g = 0;
	    newrgb.b = 0;
	    break;
    }
    WinSetBackColorRGB (&newrgb, &oldrgb);
    WinEraseRectangle (&r2, 2);
    WinDrawRectangleFrame (roundFrame, &r2);
    WinDrawTruncChars (l->msg, StrLen (l->msg), 3, 1, 155);
    WinSetBackColorRGB (&oldrgb, NULL);
}
#endif  /* ENABLE_LOGDISPLAY */



static void
AddMsg (Globals *g, char *file, int line, UInt8 severity, MemHandle id)
{
    UInt16 length;
    LogGlobals *log = g->log;
    if (log == 0) return;

    length = StrLen (g_buf);

#ifdef ENABLE_LOGFILE
    if (g_file && g_filesize < FILESIZE_LIMIT) {
        DateTimeType d;
        char sevchar;
        char prefix[35];
        int i = 0, j;

        TimSecondsToDateTime (TimGetSeconds(), &d);

        switch (severity) {
	    case LogTrace:  sevchar = 'T'; break;
	    case LogDebug:  sevchar = 'D'; break;
	    case LogLog:    sevchar = 'L'; break;
	    case LogInfo:   sevchar = 'I'; break;
	    case LogWarn:   sevchar = 'W'; break;
	    case LogError:  sevchar = 'E'; break;
	    case LogPanic:  sevchar = 'P'; break;
	    default:        sevchar = '?'; break;
        }

        /* Log line format:

           T 2006-06-18 09:43:25.99 DEADBEEF Hello, world! (File.c:634)
           0 2          13          25       34
        */

        i = StrPrintF (prefix, "%c %04d-%02d-%02d %02d:%02d:%02d.%02d ",
                       sevchar, d.year, d.month, d.day,
                       d.hour, d.minute, d.second,
                       (Int16) (TimGetTicks() % 100));
        PANICIF (i < 0, "Logging bug");
        if (id)
            i += StrPrintF (&prefix[i], "%lx ", id);
        else
            i += StrPrintF (&prefix[i], "         ");
        prefix[34] = 0;
        FileWrite (g_file, prefix, 1, StrLen (prefix), NULL);
        g_filesize += StrLen (prefix);

	/* Convert newlines to spaces in message.  */
	for (j = 0; j < length; j++) {
	    if (g_buf[j] == '\n')
		g_buf[j] = ' ';
	}
	FileWrite (g_file, g_buf, 1, length, NULL);
        g_filesize += length;

        i = StrPrintF (prefix, " (%s:%d)\n", file, line);
        FileWrite (g_file, prefix, 1, StrLen (prefix), NULL);
        g_filesize += StrLen (prefix);

        if (g_filesize >= FILESIZE_LIMIT) {
            char *line = "Logfile size limit exceeded; "
                "the rest of the logs is discarded.";
            FileWrite (g_file, line, 1, StrLen (line), NULL);
        }

        FileFlush (g_file);
    }
#endif

#ifdef ENABLE_LOGDISPLAY
    if (g_queue && severity > LogTrace) {
        MemHandle handle;
        LogLine *l;

        if (length > 200)
            length = 200;
        handle = CheckedMemHandleNew (sizeof (LogLine) + length + 1,
				      true);

        l = (LogLine *)MemHandleLock (handle);
        l->timestamp = TimGetSeconds();
        l->severity = severity;
        StrNCopy (l->msg, g_buf, length);
        l->msg[length] = 0;
        MemPtrUnlock (l);

        MsgAddHandle (g_queue, handle);
    }
#endif
}



static void
CanaryCheck (Globals *g)
{
    LogGlobals *log = g->log;

    ErrFatalDisplayIf (g_canary != 42, "Buffer overflow");
    g_canary = 42;
}


/***********************************************************************
 *
 *  EXPORTED FUNCTION DEFINITIONS
 *
 ***********************************************************************/


void
f_LogInit (Globals *g)
{
    LogGlobals *log;

    if (g->log)
        f_LogClose (g);
    g->log = CheckedMemPtrNew (sizeof (LogGlobals), true);
    log = g->log;

    g_nextsync = 0;
    g_lock = false;
    g_canary = 42;

#ifdef ENABLE_LOGDISPLAY
    g_queue = MsgNewQueue (LOGQUEUE_SIZE, "LogQueue",
			   false, true, QueueCallback);
    g_needsRedraw = false;
#endif

#ifdef ENABLE_LOGFILE
    {
        Err err;
        UInt16 cardNo;
        LocalID dbID;

        SysCurAppDatabase (&cardNo, &dbID);
        FileDelete (cardNo, LOGFILENAME);
        g_file = FileOpen (cardNo, LOGFILENAME, sysFileTFileStream,
                           appFileCreator,
                           fileModeReadWrite | fileModeLeaveOpen,
                           &err);
        g_filesize = 0;
    }
#endif

    DEBUG ("Logging system initialized");
}



void
f_LogClose (Globals *g)
{
    LogGlobals *log = g->log;
    if (log == 0) return;

    DEBUG ("Logging system closing");

    /* Prevent recursive callback calls etc. from messing us up.  */
    g->log = 0;

    g_lock = true;

#ifdef ENABLE_LOGFILE
    if (g_file) {
        FileClose (g_file);
        g_file = 0;
    }
#endif

#ifdef ENABLE_LOGDISPLAY
    if (g_queue) {
        MsgFreeQueue (g_queue);
    }
#endif

    MemPtrFree (log);
    g->log = 0;
}



void
f_LogFlush (Globals *g, Boolean force)
{
    LogGlobals *log = g->log;
    if (log == 0) return;

    if (!force && g_nextsync > TimGetTicks())
        return;

    g_nextsync = TimGetTicks() + LOGFLUSH_PERIOD;

#ifdef ENABLE_LOGFILE
    if (g_file) {
        FileFlush (g_file);
    }
#endif

#ifdef ENABLE_LOGDISPLAY
    if (g_queue) {
        if (g_foreground) {
            MemHandle handle, next;

            handle = MsgGet (g_queue);

            if (handle) {
                LogLine *l;
                if (force) {
                    /* Keep only the last message in the queue,
                     * discard the rest immediately.  */
                    next = MsgGet (g_queue);
                    while (next) {
                        MemHandleFree (handle);
                        MsgProcessed (g_queue, handle);
                        handle = next;
                        next = MsgGet (g_queue);
                    }
                }

                l = (LogLine *) MemHandleLock (handle);

                DrawLog (g, l);
                g_needsRedraw = true;
                g_nextsync = TimGetTicks() + 2 + l->severity * 30;

                MemPtrUnlock (l);
                MemHandleFree (handle);
                MsgProcessed (g_queue, handle);
            }
            else {
                if (g_needsRedraw)
                    ENQUEUE (CmdRedraw);
                g_needsRedraw = false;
            }
        }
        else {                      /* Background mode */
            MemHandle handle;
            /* Just clean out the queue.  */
            for (handle = MsgGet (g_queue);
                 handle;
                 handle = MsgGet (g_queue)) {
                MemHandleFree (handle);
                MsgProcessed (g_queue, handle);
            }
        }
    }
#endif

}



void
f_Alert (Globals *g, char *format, ...)
{
    LogGlobals *log = g->log;
    _Palm_va_list args;
    char *buf;
    Boolean newbuf;
    Boolean oldlock = false;
    if (log == 0 || g_lock) {
        buf = (char *) CheckedMemPtrNew (BUFSIZE, false);
        newbuf = true;
    }
    else {
        buf = g_buf;
        newbuf = false;
        oldlock = g_lock;
        g_lock = true;
    }

    va_start (args, format);
    CanaryCheck (g);
    StrVPrintF (g_buf, format, args);
    CanaryCheck (g);
    va_end (args);

    FrmCustomAlert (InformationAlert, g_buf, NULL, NULL);
    if (newbuf) {
        MemPtrFree (buf);
    }
    else {
	g_lock = oldlock;
    }	
}



void
f_Log (Globals *g, char *file, int line,
       UInt8 severity, MemHandle id,
       char *format, ...)
{
    LogGlobals *log = g->log;
    _Palm_va_list args;
    if (log == 0 || g_lock) return;

    va_start (args, format);
    CanaryCheck (g);
    f_LogV (g, file, line, severity, id, format, args);
    CanaryCheck (g);
    va_end (args);
}



void
f_LogV (Globals *g, char *file, int line,
        UInt8 severity, MemHandle id,
        char *format, _Palm_va_list args)
{
    LogGlobals *log = g->log;
    if (log == 0 || g_lock) return;

    g_lock = true;
    CanaryCheck (g);
    StrVPrintF (g_buf, format, args);
    CanaryCheck (g);

    if (severity >= LogInfo)
        ProgressUpdate (severity, "%s", g_buf);
    AddMsg (g, file, line, severity, id);
    g_lock = false;
}



void
f_Error (Globals *g, char *file, int line,
	 MemHandle id, char *format, ...)
{
    _Palm_va_list args;
    LogGlobals *log = g->log;
    if (log == 0) return;

    if (!g_lock) {
        g_lock = true;
        va_start (args, format);
        CanaryCheck (g);
        StrVPrintF (g_buf, format, args);
        CanaryCheck (g);
        va_end (args);

        AddMsg (g, file, line, LogError, id);

#ifdef ENABLE_LOGFILE
        if (g_file)
            FileFlush (g_file);
#endif

        if (!Progressing())
            FrmCustomAlert (ErrorAlert, g_buf, NULL, NULL);
        else
            ProgressUpdate (LogError, "%s", g_buf);
        g_lock = false;
    }
    else {
        /* Errors should at least be shown to the user.  */
        char *buf = CheckedMemPtrNew (BUFSIZE, false);
        va_start (args, format);
        StrVPrintF (buf, format, args);
        va_end (args);

        if (!Progressing())
            FrmCustomAlert (ErrorAlert, buf, NULL, NULL);
        else
            ProgressUpdate (LogError, "%s", buf);
        MemPtrFree (buf);
    }
}



void
f_Panic (Globals *g, char *file, int line,
	 MemHandle id, char *format, ...)
{
    _Palm_va_list args;
    char *buf;
    LogGlobals *log = g->log;
    if (log == 0 || g_lock)
        buf = (char *) CheckedMemPtrNew (BUFSIZE, false);
    else {
        CanaryCheck (g);
        buf = g_buf;
    }

    g_lock = true;
    va_start (args, format);
    StrVPrintF (buf, format, args);
    va_end (args);
    if (log != 0) {
        CanaryCheck (g);
        AddMsg (g, file, line, LogPanic, id);
#ifdef ENABLE_LOGFILE
        if (g_file)
            FileFlush (g_file);
#endif
    }
    g_lock = false;
    ErrDisplayFileLineMsg (file, line, buf);
}



void
f_DumpData (Globals *g, char *file, int line,
            UInt8 severity, MemHandle id,
            char *prefix,
            UInt8 *start, UInt32 length)
{
    UInt8 *end = start + length;
    int i = 0;
    LogGlobals *log = g->log;
    if (log == 0 || g_lock) return;

    g_lock = true;
    while (start < end) {
        CanaryCheck (g);
        StrCopy (g_buf, prefix);
        if (i > 0)
            StrCat (g_buf, " (cont)");
        i = StrLen (g_buf);
        g_buf[i++] = ' ';
        while (start < end && i < BUFSIZE - 4) {
            UInt8 byte = *start;
            UInt8 nibble = ((byte & 0xF0) >> 4);
            if (nibble <= 9)
                g_buf[i++] = '0' + nibble;
            else
                g_buf[i++] = 'A' + nibble - 10;
            nibble = (byte & 0x0F);
            if (nibble <= 9)
                g_buf[i++] = '0' + nibble;
            else
                g_buf[i++] = 'A' + nibble - 10;
            start++;
            g_buf[i++] = ' ';
        }
        g_buf[i] = 0;
        CanaryCheck (g);
        AddMsg (g, file, line, severity, id);
    }
    g_lock = false;
}
