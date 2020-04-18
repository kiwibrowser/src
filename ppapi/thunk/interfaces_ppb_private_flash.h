// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Please see inteface_ppb_public_stable for the documentation on the format of
// this file.

#include "ppapi/thunk/interfaces_preamble.h"

PROXIED_IFACE(PPB_FLASH_INTERFACE_12_4,
              PPB_Flash_12_4)
PROXIED_IFACE(PPB_FLASH_INTERFACE_12_5,
              PPB_Flash_12_5)
PROXIED_IFACE(PPB_FLASH_INTERFACE_12_6,
              PPB_Flash_12_6)
PROXIED_IFACE(PPB_FLASH_INTERFACE_13_0,
              PPB_Flash_13_0)

PROXIED_IFACE(PPB_FLASH_FILE_MODULELOCAL_INTERFACE_3_0,
              PPB_Flash_File_ModuleLocal_3_0)
PROXIED_IFACE(PPB_FLASH_FILE_FILEREF_INTERFACE,
              PPB_Flash_File_FileRef)

PROXIED_IFACE(PPB_FLASH_CLIPBOARD_INTERFACE_4_0,
              PPB_Flash_Clipboard_4_0)
PROXIED_IFACE(PPB_FLASH_CLIPBOARD_INTERFACE_5_0,
              PPB_Flash_Clipboard_5_0)
PROXIED_IFACE(PPB_FLASH_CLIPBOARD_INTERFACE_5_1,
              PPB_Flash_Clipboard_5_1)

PROXIED_IFACE(PPB_FLASH_DEVICEID_INTERFACE_1_0,
              PPB_Flash_DeviceID_1_0)
PROXIED_IFACE(PPB_FLASH_DRM_INTERFACE_1_0,
              PPB_Flash_DRM_1_0)
PROXIED_IFACE(PPB_FLASH_DRM_INTERFACE_1_1,
              PPB_Flash_DRM_1_1)

PROXIED_IFACE(PPB_FLASH_FONTFILE_INTERFACE_0_1,
              PPB_Flash_FontFile_0_1)
PROXIED_IFACE(PPB_FLASH_FONTFILE_INTERFACE_0_2,
              PPB_Flash_FontFile_0_2)

PROXIED_IFACE(PPB_FLASH_MENU_INTERFACE_0_2,
              PPB_Flash_Menu_0_2)

PROXIED_API(PPB_Flash_MessageLoop)
PROXIED_IFACE(PPB_FLASH_MESSAGELOOP_INTERFACE_0_1,
              PPB_Flash_MessageLoop_0_1)

PROXIED_IFACE(PPB_FLASH_PRINT_INTERFACE_1_0,
              PPB_Flash_Print_1_0)

#include "ppapi/thunk/interfaces_postamble.h"
