/************************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Error.h
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
