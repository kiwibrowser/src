/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash.idl modified Tue Oct 24 12:52:30 2017. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_H_

#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/trusted/ppb_browser_font_trusted.h"

#define PPB_FLASH_INTERFACE_12_4 "PPB_Flash;12.4"
#define PPB_FLASH_INTERFACE_12_5 "PPB_Flash;12.5"
#define PPB_FLASH_INTERFACE_12_6 "PPB_Flash;12.6"
#define PPB_FLASH_INTERFACE_13_0 "PPB_Flash;13.0"
#define PPB_FLASH_INTERFACE PPB_FLASH_INTERFACE_13_0

/**
 * @file
 * This file contains the <code>PPB_Flash</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
typedef enum {
  /**
   * No restrictions on Flash LSOs.
   */
  PP_FLASHLSORESTRICTIONS_NONE = 1,
  /**
   * Don't allow access to Flash LSOs.
   */
  PP_FLASHLSORESTRICTIONS_BLOCK = 2,
  /**
   * Store Flash LSOs in memory only.
   */
  PP_FLASHLSORESTRICTIONS_IN_MEMORY = 3
} PP_FlashLSORestrictions;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_FlashLSORestrictions, 4);

typedef enum {
  /**
   * Specifies if the system likely supports 3D hardware acceleration.
   *
   * The result is a boolean PP_Var, depending on the supported nature of 3D
   * acceleration. If querying this function returns true, the 3D system will
   * normally use the native hardware for rendering which will be much faster.
   *
   * Having this set to true only means that 3D should be used to draw 2D and
   * video elements. PP_FLASHSETTING_STAGE3D_ENABLED should be checked to
   * determine if it's ok to use 3D for arbitrary content.
   *
   * In rare cases (depending on the platform) this value will be true but a
   * created 3D context will use emulation because context initialization
   * failed.
   */
  PP_FLASHSETTING_3DENABLED = 1,
  PP_FLASHSETTING_FIRST = PP_FLASHSETTING_3DENABLED,
  /**
   * Specifies if the given instance is in private/incognito/off-the-record mode
   * (returns true) or "regular" mode (returns false). Returns an undefined
   * PP_Var on invalid instance.
   */
  PP_FLASHSETTING_INCOGNITO = 2,
  /**
   * Specifies if arbitrary 3d commands are supported (returns true), or if 3d
   * should only be used for drawing 2d and video (returns false).
   *
   * This should only be enabled if PP_FLASHSETTING_3DENABLED is true.
   */
  PP_FLASHSETTING_STAGE3DENABLED = 3,
  /**
   * Specifies the string for the language code of the UI of the browser.
   *
   * For example: "en-US" or "de".
   *
   * Returns an undefined PP_Var on invalid instance.
   */
  PP_FLASHSETTING_LANGUAGE = 4,
  /**
   * Specifies the number of CPU cores that are present on the system.
   */
  PP_FLASHSETTING_NUMCORES = 5,
  /**
   * Specifies restrictions on how flash should handle LSOs. The result is an
   * int from <code>PP_FlashLSORestrictions</code>.
   */
  PP_FLASHSETTING_LSORESTRICTIONS = 6,
  /**
   * Specifies if the driver is reliable enough to use Shader Model 3 commands
   * with it.
   *
   * This should only be enabled if PP_FLASHSETTING_STAGE3DENABLED is true.
   */
  PP_FLASHSETTING_STAGE3DBASELINEENABLED = 7,
  PP_FLASHSETTING_LAST = PP_FLASHSETTING_STAGE3DBASELINEENABLED
} PP_FlashSetting;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_FlashSetting, 4);

/**
 * This enum provides keys for setting breakpad crash report data.
 */
typedef enum {
  /**
   * Specifies the document URL which contains the flash instance.
   */
  PP_FLASHCRASHKEY_URL = 1,
  /**
   * Specifies the URL of the current swf.
   */
  PP_FLASHCRASHKEY_RESOURCE_URL = 2
} PP_FlashCrashKey;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_FlashCrashKey, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_Flash</code> interface contains pointers to various functions
 * that are only needed to support Pepper Flash.
 */
struct PPB_Flash_13_0 {
  /**
   * Sets or clears the rendering hint that the given plugin instance is always
   * on top of page content. Somewhat more optimized painting can be used in
   * this case.
   */
  void (*SetInstanceAlwaysOnTop)(PP_Instance instance, PP_Bool on_top);
  /**
   * Draws the given pre-laid-out text. It is almost equivalent to Windows'
   * ExtTextOut with the addition of the transformation (a 3x3 matrix given the
   * transform to apply before drawing). It also adds the allow_subpixel_aa
   * flag which when true, will use subpixel antialiasing if enabled in the
   * system settings. For this to work properly, the graphics layer that the
   * text is being drawn into must be opaque.
   */
  PP_Bool (*DrawGlyphs)(
      PP_Instance instance,
      PP_Resource pp_image_data,
      const struct PP_BrowserFont_Trusted_Description* font_desc,
      uint32_t color,
      const struct PP_Point* position,
      const struct PP_Rect* clip,
      const float transformation[3][3],
      PP_Bool allow_subpixel_aa,
      uint32_t glyph_count,
      const uint16_t glyph_indices[],
      const struct PP_Point glyph_advances[]);
  /**
   * Retrieves the proxy that will be used for the given URL. The result will
   * be a string in PAC format, or an undefined var on error.
   */
  struct PP_Var (*GetProxyForURL)(PP_Instance instance, const char* url);
  /**
   * Navigate to the URL given by the given URLRequestInfo. (This supports GETs,
   * POSTs, and javascript: URLs.) May open a new tab if target is not "_self".
   */
  int32_t (*Navigate)(PP_Resource request_info,
                      const char* target,
                      PP_Bool from_user_action);
  /**
   * Retrieves the local time zone offset from GM time for the given UTC time.
   */
  double (*GetLocalTimeZoneOffset)(PP_Instance instance, PP_Time t);
  /**
   * Gets a (string) with "command-line" options for Flash; used to pass
   * run-time debugging parameters, etc.
   */
  struct PP_Var (*GetCommandLineArgs)(PP_Module module);
  /**
   * Loads the given font in a more privileged process on Windows. Call this if
   * Windows is giving errors for font calls. See
   * content/renderer/font_cache_dispatcher_win.cc
   *
   * The parameter is a pointer to a LOGFONTW structure.
   *
   * On non-Windows platforms, this function does nothing.
   */
  void (*PreloadFontWin)(const void* logfontw);
  /**
   * Returns whether the given rectangle (in the plugin) is topmost, i.e., above
   * all other web content.
   */
  PP_Bool (*IsRectTopmost)(PP_Instance instance, const struct PP_Rect* rect);
  /**
   * Indicates that there's activity and, e.g., the screensaver shouldn't kick
   * in.
   */
  void (*UpdateActivity)(PP_Instance instance);
  /**
   * Returns the value associated with the given setting. Invalid enums will
   * result in an undefined PP_Var return value.
   */
  struct PP_Var (*GetSetting)(PP_Instance instance, PP_FlashSetting setting);
  /**
   * Allows setting breakpad crash data which will be included in plugin crash
   * reports. Returns PP_FALSE if crash data could not be set.
   */
  PP_Bool (*SetCrashData)(PP_Instance instance,
                          PP_FlashCrashKey key,
                          struct PP_Var value);
  /**
   * Enumerates video capture devices. |video_capture| is a valid
   * PPB_VideoCapture_Dev resource. Once the operation has completed
   * successfully, |devices| will be set up with an array of
   * PPB_DeviceRef_Dev resources.
   *
   * PP_OK is returned on success and different pepper error code on failure.
   * The ref count of the returned |devices| has already been increased by 1 for
   * the caller.
   *
   * NOTE: This method is a synchronous version of |EnumerateDevices| in
   * PPB_VideoCapture_Dev.
   */
  int32_t (*EnumerateVideoCaptureDevices)(PP_Instance instance,
                                          PP_Resource video_capture,
                                          struct PP_ArrayOutput devices);
};

typedef struct PPB_Flash_13_0 PPB_Flash;

struct PPB_Flash_12_4 {
  void (*SetInstanceAlwaysOnTop)(PP_Instance instance, PP_Bool on_top);
  PP_Bool (*DrawGlyphs)(
      PP_Instance instance,
      PP_Resource pp_image_data,
      const struct PP_BrowserFont_Trusted_Description* font_desc,
      uint32_t color,
      const struct PP_Point* position,
      const struct PP_Rect* clip,
      const float transformation[3][3],
      PP_Bool allow_subpixel_aa,
      uint32_t glyph_count,
      const uint16_t glyph_indices[],
      const struct PP_Point glyph_advances[]);
  struct PP_Var (*GetProxyForURL)(PP_Instance instance, const char* url);
  int32_t (*Navigate)(PP_Resource request_info,
                      const char* target,
                      PP_Bool from_user_action);
  void (*RunMessageLoop)(PP_Instance instance);
  void (*QuitMessageLoop)(PP_Instance instance);
  double (*GetLocalTimeZoneOffset)(PP_Instance instance, PP_Time t);
  struct PP_Var (*GetCommandLineArgs)(PP_Module module);
  void (*PreloadFontWin)(const void* logfontw);
  PP_Bool (*IsRectTopmost)(PP_Instance instance, const struct PP_Rect* rect);
  int32_t (*InvokePrinting)(PP_Instance instance);
  void (*UpdateActivity)(PP_Instance instance);
  struct PP_Var (*GetDeviceID)(PP_Instance instance);
  int32_t (*GetSettingInt)(PP_Instance instance, PP_FlashSetting setting);
  struct PP_Var (*GetSetting)(PP_Instance instance, PP_FlashSetting setting);
};

struct PPB_Flash_12_5 {
  void (*SetInstanceAlwaysOnTop)(PP_Instance instance, PP_Bool on_top);
  PP_Bool (*DrawGlyphs)(
      PP_Instance instance,
      PP_Resource pp_image_data,
      const struct PP_BrowserFont_Trusted_Description* font_desc,
      uint32_t color,
      const struct PP_Point* position,
      const struct PP_Rect* clip,
      const float transformation[3][3],
      PP_Bool allow_subpixel_aa,
      uint32_t glyph_count,
      const uint16_t glyph_indices[],
      const struct PP_Point glyph_advances[]);
  struct PP_Var (*GetProxyForURL)(PP_Instance instance, const char* url);
  int32_t (*Navigate)(PP_Resource request_info,
                      const char* target,
                      PP_Bool from_user_action);
  void (*RunMessageLoop)(PP_Instance instance);
  void (*QuitMessageLoop)(PP_Instance instance);
  double (*GetLocalTimeZoneOffset)(PP_Instance instance, PP_Time t);
  struct PP_Var (*GetCommandLineArgs)(PP_Module module);
  void (*PreloadFontWin)(const void* logfontw);
  PP_Bool (*IsRectTopmost)(PP_Instance instance, const struct PP_Rect* rect);
  int32_t (*InvokePrinting)(PP_Instance instance);
  void (*UpdateActivity)(PP_Instance instance);
  struct PP_Var (*GetDeviceID)(PP_Instance instance);
  int32_t (*GetSettingInt)(PP_Instance instance, PP_FlashSetting setting);
  struct PP_Var (*GetSetting)(PP_Instance instance, PP_FlashSetting setting);
  PP_Bool (*SetCrashData)(PP_Instance instance,
                          PP_FlashCrashKey key,
                          struct PP_Var value);
};

struct PPB_Flash_12_6 {
  void (*SetInstanceAlwaysOnTop)(PP_Instance instance, PP_Bool on_top);
  PP_Bool (*DrawGlyphs)(
      PP_Instance instance,
      PP_Resource pp_image_data,
      const struct PP_BrowserFont_Trusted_Description* font_desc,
      uint32_t color,
      const struct PP_Point* position,
      const struct PP_Rect* clip,
      const float transformation[3][3],
      PP_Bool allow_subpixel_aa,
      uint32_t glyph_count,
      const uint16_t glyph_indices[],
      const struct PP_Point glyph_advances[]);
  struct PP_Var (*GetProxyForURL)(PP_Instance instance, const char* url);
  int32_t (*Navigate)(PP_Resource request_info,
                      const char* target,
                      PP_Bool from_user_action);
  void (*RunMessageLoop)(PP_Instance instance);
  void (*QuitMessageLoop)(PP_Instance instance);
  double (*GetLocalTimeZoneOffset)(PP_Instance instance, PP_Time t);
  struct PP_Var (*GetCommandLineArgs)(PP_Module module);
  void (*PreloadFontWin)(const void* logfontw);
  PP_Bool (*IsRectTopmost)(PP_Instance instance, const struct PP_Rect* rect);
  int32_t (*InvokePrinting)(PP_Instance instance);
  void (*UpdateActivity)(PP_Instance instance);
  struct PP_Var (*GetDeviceID)(PP_Instance instance);
  int32_t (*GetSettingInt)(PP_Instance instance, PP_FlashSetting setting);
  struct PP_Var (*GetSetting)(PP_Instance instance, PP_FlashSetting setting);
  PP_Bool (*SetCrashData)(PP_Instance instance,
                          PP_FlashCrashKey key,
                          struct PP_Var value);
  int32_t (*EnumerateVideoCaptureDevices)(PP_Instance instance,
                                          PP_Resource video_capture,
                                          struct PP_ArrayOutput devices);
};
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_H_ */

