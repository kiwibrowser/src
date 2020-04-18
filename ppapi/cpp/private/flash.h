// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_H_
#define PPAPI_CPP_PRIVATE_FLASH_H_

#include <string>
#include <vector>

#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"

struct PP_Point;

namespace pp {

class BrowserFontDescription;
class DeviceRef_Dev;
class ImageData;
class InstanceHandle;
class Module;
class Point;
class Rect;
class URLRequestInfo;
class Var;
class VideoCapture_Dev;

namespace flash {

class Flash {
 public:
  // Returns true if the required interface is available.
  static bool IsAvailable();

  static void SetInstanceAlwaysOnTop(const InstanceHandle& instance,
                                     bool on_top);
  static bool DrawGlyphs(const InstanceHandle& instance,
                         ImageData* image,
                         const BrowserFontDescription& font_desc,
                         uint32_t color,
                         const Point& position,
                         const Rect& clip,
                         const float transformation[3][3],
                         bool allow_subpixel_aa,
                         uint32_t glyph_count,
                         const uint16_t glyph_indices[],
                         const PP_Point glyph_advances[]);
  static Var GetProxyForURL(const InstanceHandle& instance,
                            const std::string& url);
  static int32_t Navigate(const URLRequestInfo& request_info,
                          const std::string& target,
                          bool from_user_action);
  static void RunMessageLoop(const InstanceHandle& instance);
  static void QuitMessageLoop(const InstanceHandle& instance);
  static double GetLocalTimeZoneOffset(const InstanceHandle& instance,
                                       PP_Time t);
  static Var GetCommandLineArgs(Module* module);
  static void PreloadFontWin(const void* logfontw);
  static bool IsRectTopmost(const InstanceHandle& instance, const Rect& rect);
  static void UpdateActivity(const InstanceHandle& instance);
  static Var GetDeviceID(const InstanceHandle& instance);
  static Var GetSetting(const InstanceHandle& instance,
                        PP_FlashSetting setting);
  static bool SetCrashData(const InstanceHandle& instance,
                           PP_FlashCrashKey key,
                           const pp::Var& value);
  static int32_t EnumerateVideoCaptureDevices(const InstanceHandle& instance,
      const VideoCapture_Dev& video_capture,
      std::vector<DeviceRef_Dev>* devices_out);

  // PPB_Flash_Print.
  static bool InvokePrinting(const InstanceHandle& instance);
};

}  // namespace flash
}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_H_
