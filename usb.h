/*
  VitaVault - PSVita Backup Utility
  USB Mass Storage Support (adapted from VitaShell)

  Copyright (C) 2015-2018, TheFloW (original VitaShell USB code)
  Copyright (C) 2026 theheroGAC (VitaVault adaptation)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __USB_H__
#define __USB_H__

#include <psp2/types.h>
#include "types.h"

// USB Mass Storage types
#define USBSTOR_TYPE_FAT    0
#define USBSTOR_TYPE_MEMORY 1


extern int g_usb_active;
extern SceUID g_usb_modid;


SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type);
int stopUsb(SceUID modid);


void usb_init(void);


int usb_start_mass_storage(void);
int usb_stop_mass_storage(void);

// Mount/Unmount functions (from VitaShell)
int mountGamecardUx0();
int umountGamecardUx0();
int mountUsbUx0();
int umountUsbUx0();

#endif