// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/logging.h"
#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_functions_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/ppb_video_capture_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

void SetInstanceAlwaysOnTop(PP_Instance instance, PP_Bool on_top) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->SetInstanceAlwaysOnTop(instance, on_top);
}

PP_Bool DrawGlyphs(PP_Instance instance,
                   PP_Resource pp_image_data,
                   const PP_BrowserFont_Trusted_Description* font_desc,
                   uint32_t color,
                   const PP_Point* position,
                   const PP_Rect* clip,
                   const float transformation[3][3],
                   PP_Bool allow_subpixel_aa,
                   uint32_t glyph_count,
                   const uint16_t glyph_indices[],
                   const PP_Point glyph_advances[]) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->DrawGlyphs(
      instance, pp_image_data, font_desc, color, position, clip, transformation,
      allow_subpixel_aa, glyph_count, glyph_indices, glyph_advances);
}

PP_Var GetProxyForURL(PP_Instance instance, const char* url) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetProxyForURL(instance, url);
}

int32_t Navigate(PP_Resource request_id,
                 const char* target,
                 PP_Bool from_user_action) {
  // TODO(brettw): this function should take an instance.
  // To work around this, use the PP_Instance from the resource.
  PP_Instance instance;
  {
    EnterResource<PPB_URLRequestInfo_API> enter(request_id, true);
    if (enter.failed())
      return PP_ERROR_BADRESOURCE;
    instance = enter.resource()->pp_instance();
  }

  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->Navigate(instance, request_id, target,
                                     from_user_action);
}

void RunMessageLoop(PP_Instance instance) {
  // Deprecated.
  NOTREACHED();
  return;
}

void QuitMessageLoop(PP_Instance instance) {
  // Deprecated.
  NOTREACHED();
  return;
}

double GetLocalTimeZoneOffset(PP_Instance instance, PP_Time t) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return 0.0;
  return enter.functions()->GetLocalTimeZoneOffset(instance, t);
}

PP_Var GetCommandLineArgs(PP_Module /* pp_module */) {
  // There's no instance so we have to reach into the globals without thunking.
  ProxyAutoLock lock;
  return StringVar::StringToPPVar(PpapiGlobals::Get()->GetCmdLine());
}

void PreLoadFontWin(const void* logfontw) {
  // There's no instance so we have to reach into the delegate without
  // thunking.
  ProxyAutoLock lock;
  PpapiGlobals::Get()->PreCacheFontForFlash(logfontw);
}

PP_Bool IsRectTopmost(PP_Instance instance, const PP_Rect* rect) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->IsRectTopmost(instance, rect);
}

int32_t InvokePrinting(PP_Instance instance) {
  // This function is no longer supported, use PPB_Flash_Print instead.
  return PP_ERROR_NOTSUPPORTED;
}

void UpdateActivity(PP_Instance instance) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->UpdateActivity(instance);
}

PP_Var GetDeviceID(PP_Instance instance) {
  // Deprecated.
  NOTREACHED();
  return PP_MakeUndefined();
}

int32_t GetSettingInt(PP_Instance instance, PP_FlashSetting setting) {
  // Deprecated.
  NOTREACHED();
  return -1;
}

PP_Var GetSetting(PP_Instance instance, PP_FlashSetting setting) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetSetting(instance, setting);
}

PP_Bool SetCrashData(PP_Instance instance,
                     PP_FlashCrashKey key,
                     PP_Var value) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->SetCrashData(instance, key, value);
}

int32_t EnumerateVideoCaptureDevices(PP_Instance instance,
                                     PP_Resource video_capture,
                                     PP_ArrayOutput devices) {
  EnterResource<PPB_VideoCapture_API> enter(video_capture, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->EnumerateDevicesSync(devices);
}

const PPB_Flash_12_4 g_ppb_flash_12_4_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &InvokePrinting,
  &UpdateActivity,
  &GetDeviceID,
  &GetSettingInt,
  &GetSetting
};

const PPB_Flash_12_5 g_ppb_flash_12_5_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &InvokePrinting,
  &UpdateActivity,
  &GetDeviceID,
  &GetSettingInt,
  &GetSetting,
  &SetCrashData
};

const PPB_Flash_12_6 g_ppb_flash_12_6_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &InvokePrinting,
  &UpdateActivity,
  &GetDeviceID,
  &GetSettingInt,
  &GetSetting,
  &SetCrashData,
  &EnumerateVideoCaptureDevices
};

const PPB_Flash_13_0 g_ppb_flash_13_0_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &UpdateActivity,
  &GetSetting,
  &SetCrashData,
  &EnumerateVideoCaptureDevices
};

}  // namespace

const PPB_Flash_12_4* GetPPB_Flash_12_4_Thunk() {
  return &g_ppb_flash_12_4_thunk;
}

const PPB_Flash_12_5* GetPPB_Flash_12_5_Thunk() {
  return &g_ppb_flash_12_5_thunk;
}

const PPB_Flash_12_6* GetPPB_Flash_12_6_Thunk() {
  return &g_ppb_flash_12_6_thunk;
}

const PPB_Flash_13_0* GetPPB_Flash_13_0_Thunk() {
  return &g_ppb_flash_13_0_thunk;
}

}  // namespace thunk
}  // namespace ppapi
