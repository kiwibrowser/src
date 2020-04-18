/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash_drm.idl modified Mon Nov 11 14:49:53 2013. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_DRM_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_DRM_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_FLASH_DRM_INTERFACE_1_0 "PPB_Flash_DRM;1.0"
#define PPB_FLASH_DRM_INTERFACE_1_1 "PPB_Flash_DRM;1.1"
#define PPB_FLASH_DRM_INTERFACE PPB_FLASH_DRM_INTERFACE_1_1

/**
 * @file
 * This file contains the <code>PPB_Flash_DRM</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * A resource for performing Flash DRM-related operations.
 */
struct PPB_Flash_DRM_1_1 {
  /**
   * Creates a PPB_Flash_DRM resource for performing DRM-related operations in
   * Flash.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Asynchronously computes the device ID. When available, it will place the
   * string in |*id| and will call the completion callback. On failure the
   * given var will be PP_VARTYPE_UNDEFINED.
   */
  int32_t (*GetDeviceID)(PP_Resource drm,
                         struct PP_Var* id,
                         struct PP_CompletionCallback callback);
  /**
   * Windows and Mac only. Synchronously outputs the HMONITOR or
   * CGDirectDisplayID corresponding to the monitor on which the plugin instance
   * is displayed in |hmonitor|. This value is queried asynchronously and this
   * will return PP_FALSE if the value is not yet available or an error
   * occurred. PP_TRUE is returned on success.
   */
  PP_Bool (*GetHmonitor)(PP_Resource drm, int64_t* hmonitor);
  /**
   * Asynchronously returns a PPB_FileRef resource in |file_ref| which points to
   * the Voucher file for performing DRM verification. |callback| will be called
   * upon completion.
   */
  int32_t (*GetVoucherFile)(PP_Resource drm,
                            PP_Resource* file_ref,
                            struct PP_CompletionCallback callback);
  /**
   * Asynchronously returns a value indicating whether the monitor on which the
   * plugin instance is displayed is external. |callback| will be called upon
   * completion.
   */
  int32_t (*MonitorIsExternal)(PP_Resource drm,
                               PP_Bool* is_external,
                               struct PP_CompletionCallback callback);
};

typedef struct PPB_Flash_DRM_1_1 PPB_Flash_DRM;

struct PPB_Flash_DRM_1_0 {
  PP_Resource (*Create)(PP_Instance instance);
  int32_t (*GetDeviceID)(PP_Resource drm,
                         struct PP_Var* id,
                         struct PP_CompletionCallback callback);
  PP_Bool (*GetHmonitor)(PP_Resource drm, int64_t* hmonitor);
  int32_t (*GetVoucherFile)(PP_Resource drm,
                            PP_Resource* file_ref,
                            struct PP_CompletionCallback callback);
};
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_DRM_H_ */

