/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Log.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION LOG_SECTION

#ifdef NORMAL_INCLUDE

/* You can set the default loglevel in DEFAULT_LOGLEVEL.  This
 * loglevel will be used if the file does not set its own.  */
#ifndef LOGLEVEL
#define LOGLEVEL DEFAULT_LOGLEVEL
#endif

/* You can override file-local loglevels by defining FORCE_LOGLEVEL.  */
#ifdef FORCE_LOGLEVEL
#undef LOGLEVEL
#define LOGLEVEL FORCE_LOGLEVEL
#endif

#define LogTrace    0           /* Verbose debug messages */
#define LogDebug    1           /* Interesting debug messages */
#define LogLog      2           /* Important debug messages */
#define LogInfo     3           /* Visible to the user, if progressing */
#define LogWarn     4           /* Visible to the user */
#define LogError    5           /* Alert dialog */
#define LogPanic    6           /* Reset */

#if LOGLEVEL > LogTrace
#define MTRACE(...)
#else
#define MTRACE(...) Log (__FILE__, __LINE__, LogTrace, __VA_ARGS__)
#endif

#if LOGLEVEL > LogLog
#define MLOG(...)
#else
#define MLOG(...) Log (__FILE__, __LINE__, LogLog, __VA_ARGS__)
#endif

#if LOGLEVEL > LogDebug
#define MDEBUG(...)
#else
#define MDEBUG(...) Log (__FILE__, __LINE__, LogDebug, __VA_ARGS__)
#endif

#if LOGLEVEL > LogInfo
#define MINFO(...)
#else
#define MINFO(...) Log (__FILE__, __LINE__, LogInfo, __VA_ARGS__)
#endif

#if LOGLEVEL > LogWarn
#define MWARN(...)
#else
#define MWARN(...) Log (__FILE__, __LINE__, LogWarn, __VA_ARGS__)
#endif

#define MERROR(id, ...) Error (__FILE__, __LINE__, id, __VA_ARGS__)
#define MPANIC(...) Panic (__FILE__, __LINE__, __VA_ARGS__)

#define MDUMP(s, i, p, st, l) DumpData (__FILE__, __LINE__, s, i, p, st, l)

/* Simple versions without an id.  */
#define TRACE(...) MTRACE (0, __VA_ARGS__)
#define DEBUG(...) MDEBUG (0, __VA_ARGS__)
#define LOG(...) MLOG (0, __VA_ARGS__)
#define INFO(...) MINFO (0, __VA_ARGS__)
#define WARN(...) MWARN (0, __VA_ARGS__)
#define ERROR(...) MERROR (0, __VA_ARGS__)
#define PANIC(...) MPANIC (0, __VA_ARGS__)

#if ERROR_CHECK_LEVEL >= ERROR_CHECK_PARTIAL
#define PANICIF(cond, ...) if (cond) PANIC (__VA_ARGS__)
#define MPANICIF(cond, ...) if (cond) MPANIC (__VA_ARGS__)
#else
#define PANICIF(cond, ...) if (cond) PANIC (__VA_ARGS__)
#define MPANICIF(cond, ...) if (cond) MPANIC (__VA_ARGS__)
#endif

#define DUMP(s, p, st, l) MDUMP (s, 0, p, st, l)

#define ASSERT(cond)    ((cond) ? 0 : PANIC ("Assertion error: %s", #cond))

void f_LogInit (Globals *g);

#endif

DEFUN0 (void, LogClose);
DEFUN1 (void, LogFlush, Boolean force);
DEFUNV (void, Alert, char *format);
DEFUNP (5, 6, void, Log, char *file, int line, UInt8 severity, MemHandle id, char *format);
DEFUNN (void, LogV, char *file, int line, UInt8 severity, MemHandle id, char *format, _Palm_va_list args);
DEFUNP (4, 5, void, Error, char *file, int line, MemHandle id, char *format);
DEFUNP (4, 5, void, Panic, char *file, int line, MemHandle id, char *format);
DEFUNN (void, DumpData, char *file, int line, UInt8 severity, MemHandle id, char *prefix, UInt8 *start, UInt32 length);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

