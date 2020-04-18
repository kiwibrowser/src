/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash_font_file.idl modified Fri Oct 23 10:34:57 2015. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_FONT_FILE_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_FONT_FILE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/pp_private_font_charset.h"
#include "ppapi/c/trusted/ppb_browser_font_trusted.h"

#define PPB_FLASH_FONTFILE_INTERFACE_0_1 "PPB_Flash_FontFile;0.1"
#define PPB_FLASH_FONTFILE_INTERFACE_0_2 "PPB_Flash_FontFile;0.2"
#define PPB_FLASH_FONTFILE_INTERFACE PPB_FLASH_FONTFILE_INTERFACE_0_2

/**
 * @file
 * This file contains the <code>PPB_Flash_FontFile</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_Flash_FontFile_0_2 {
  /* Returns a resource identifying a font file corresponding to the given font
   * request after applying the browser-specific fallback.
   */
  PP_Resource (*Create)(
      PP_Instance instance,
      const struct PP_BrowserFont_Trusted_Description* description,
      PP_PrivateFontCharset charset);
  /* Determines if a given resource is Flash font file.
   */
  PP_Bool (*IsFlashFontFile)(PP_Resource resource);
  /* Returns the requested font table.
   * |output_length| should pass in the size of |output|. And it will return
   * the actual length of returned data. |output| could be NULL in order to
   * query the size of the buffer size needed. In that case, the input value of
   * |output_length| is ignored.
   * Note: it is Linux only and fails directly on other platforms.
   */
  PP_Bool (*GetFontTable)(PP_Resource font_file,
                          uint32_t table,
                          void* output,
                          uint32_t* output_length);
  /**
   * Returns whether <code>PPB_Flash_FontFile</code> is supported on Windows.
   */
  PP_Bool (*IsSupportedForWindows)(void);
};

typedef struct PPB_Flash_FontFile_0_2 PPB_Flash_FontFile;

struct PPB_Flash_FontFile_0_1 {
  PP_Resource (*Create)(
      PP_Instance instance,
      const struct PP_BrowserFont_Trusted_Description* description,
      PP_PrivateFontCharset charset);
  PP_Bool (*IsFlashFontFile)(PP_Resource resource);
  PP_Bool (*GetFontTable)(PP_Resource font_file,
                          uint32_t table,
                          void* output,
                          uint32_t* output_length);
};
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_FONT_FILE_H_ */

