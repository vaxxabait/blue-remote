/************************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Error.h
 *
 ***********************************************************************/

/* User canceled the operation.  */
#define appErrProgressCanceled	(appErrorClass + 0x0001)

/* Operation timed out.  */
#define appErrTimeout		(appErrorClass + 0x0002)

/* Encryption failed.  */
#define appErrEncryptionFailed	(appErrorClass + 0x0003)

/* HID is not connected.  */
#define appErrNotConnected	(appErrorClass + 0x0004)

/* Remote host terminated connection.  */
#define appErrConnTerminated	(appErrorClass + 0x0005)

/* The free trial period has expired.  */
#define appErrTrialExpired	(appErrorClass + 0x0006)
