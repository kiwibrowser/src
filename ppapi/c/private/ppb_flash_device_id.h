/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash_device_id.idl modified Tue May 14 10:55:27 2013. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_DEVICE_ID_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_DEVICE_ID_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_FLASH_DEVICEID_INTERFACE_1_0 "PPB_Flash_DeviceID;1.0"
#define PPB_FLASH_DEVICEID_INTERFACE PPB_FLASH_DEVICEID_INTERFACE_1_0

/**
 * @file
 * This file contains the <code>PPB_Flash_DeviceID</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/* TODO(raymes): This is deprecated by the PPB_Flash_DRM interface. Remove this
 * interface after a few versions of Chrome have passed. */
struct PPB_Flash_DeviceID_1_0 {
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Asynchronously computes the device ID. When available, it will place the
   * string in |*id| and will call the completion callback. On failure the
   * given var will be PP_VARTYPE_UNDEFINED.
   */
  int32_t (*GetDeviceID)(PP_Resource device_id,
                         struct PP_Var* id,
                         struct PP_CompletionCallback callback);
};

typedef struct PPB_Flash_DeviceID_1_0 PPB_Flash_DeviceID;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_DEVICE_ID_H_ */

