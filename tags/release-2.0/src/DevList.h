/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: DevList.h
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
#define SECTION DEVLIST_SECTION

#ifdef NORMAL_INCLUDE

#define DEVICE_STRUCT_VERSION	1

/* Maximum length of a Bluetooth device name, including the
 * terminating NUL character.  */
#define BLUETOOTH_NAME_SIZE 40

/* Length of an ASCII Bluetooth address string.  */
#define BLUETOOTH_ADDR_SIZE 20

typedef struct {
    /* Set to DEVICE_STRUCT_VERSION.  */
    UInt16 version;

    /* Internal id.  Never change this.  */
    UInt32 uid;

    /* The Bluetooth address of the remote device.  */
    BtLibDeviceAddressType address;
    /* The Bluetooth name of the remote device.
       The friendlyName.name points to name.  */
    BtLibFriendlyNameType name;

    /* UTF-8 device name.  */
    char nameStr[BLUETOOTH_NAME_SIZE];

    /* Human-readable rendering of the device address.  */
    char addressStr[BLUETOOTH_ADDR_SIZE];

    /* The time when the device was last connected.  */
    UInt32 lastConnected;

    /* If true, BlueRemote will encrypt the link before connecting the
       HID channels.  This is normally done by the PC.  */
    Boolean encryptLink;
} Device;

void f_DevListInit (Globals *g) SECTION;
#endif

DEFUN0 (void, DevListClose);
DEFUN0 (UInt16, DevListLength);

/* Returns the selected device.  */
DEFUN0 (Device *, DevListSelected);

/* Sets the selected device.  */
DEFUN1 (void, DevListSetSelected, Device *device);

DEFUN1 (Device *, DevListGet, UInt16 index);
DEFUN1 (Device *, DevListFindByUID, UInt32 uid);
DEFUN1 (Device *, DevListFindByAddress, BtLibDeviceAddressType *addr);
DEFUN1 (void, DevListUpdate, Device *device);

DEFUN1 (void, DevListDelete, Device *device);
DEFUN1 (Device *, DevListAdd, Device *device);

DEFUN1 (void, DevListMoveTop, Device *device);

DEFUN0 (UInt32, DevListModNum);

DEFUN0 (void, DevListDetails);
