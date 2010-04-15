/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Bluetooth.h
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
#define SECTION BLUETOOTH_SECTION

#ifdef NORMAL_INCLUDE
typedef enum {
    classKeyboard = 1,
    classMouse = 2,
    classCombo = 3,
    classMouseKeyboard = 4,
} BTDeviceClassType;

Err f_BTInit (Globals *g) SECTION;

#elif defined (GLOBALS_INCLUDE)

MsgQueuePtr interruptInQueue;
MsgQueuePtr interruptOutQueue;
MsgQueuePtr controlInQueue;
MsgQueuePtr controlOutQueue;

#define g_interruptInQueue	g->interruptInQueue
#define g_interruptOutQueue	g->interruptOutQueue
#define g_controlInQueue	g->controlInQueue
#define g_controlOutQueue	g->controlOutQueue

#endif	/* GLOBALS_INCLUDE */

DEFUN0 (Err, BTRadioInit);
DEFUN0 (void, BTRadioClose);

/* Close the Bluetooth module.  */
DEFUN0 (void, BTClose);

/* Create the listener sockets and publish the SDP record.  */
DEFUN0 (Err, BTListenInit);

DEFUN0 (Err, BTWait);

/* Delete the listener sockets and unpublish the SDP record.  */
DEFUN0 (void, BTListenClose);

/* Handle a Bluetooth-specific event.  */
DEFUN1 (Boolean, BTHandleEvent, EventPtr e);

/* Connect to a remote device.  */
DEFUN0 (Err, BTConnect);

/* Disconnect the current connection.  */
DEFUN0 (void, BTDisconnect);

/* Returns true if we are currently connected and ready to transmit.  */
DEFUN0 (Boolean, BTIsConnected);

/* Select another device.  Puts application in cabled mode.  */
DEFUN0 (Err, BTSelectDevice);

/* Notify the Bluetooth module that data is ready to be sent with
   HIDPSendReport.  If BT decides to send a new report, this function
   returns the MemHandle of the last message sent.  If the interrupt
   channel is currently busy, this function returns 0.  */
DEFUN0 (MemHandle, BTDataReady);
