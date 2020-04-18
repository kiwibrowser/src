// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_FUNCTIONS_API_H_
#define PPAPI_THUNK_PPB_FLASH_FUNCTIONS_API_H_

#include <stdint.h>

#include <string>

#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/shared_impl/singleton_resource_id.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

struct PP_BrowserFont_Trusted_Description;

namespace ppapi {
namespace thunk {

// This class collects all of the Flash interface-related APIs into one place.
class PPAPI_THUNK_EXPORT PPB_Flash_Functions_API {
 public:
  virtual ~PPB_Flash_Functions_API() {}

  virtual PP_Var GetProxyForURL(PP_Instance instance,
                                const std::string& url) = 0;
  virtual void UpdateActivity(PP_Instance instance) = 0;
  virtual PP_Bool SetCrashData(PP_Instance instance, PP_FlashCrashKey key,
                               PP_Var value) = 0;
  virtual double GetLocalTimeZoneOffset(PP_Instance instance, PP_Time t) = 0;
  virtual PP_Var GetSetting(PP_Instance instance, PP_FlashSetting setting) = 0;
  virtual void SetInstanceAlwaysOnTop(PP_Instance instance, PP_Bool on_top) = 0;
  virtual PP_Bool DrawGlyphs(
      PP_Instance instance,
      PP_Resource pp_image_data,
      const PP_BrowserFont_Trusted_Description* font_desc,
      uint32_t color,
      const PP_Point* position,
      const PP_Rect* clip,
      const float transformation[3][3],
      PP_Bool allow_subpixel_aa,
      uint32_t glyph_count,
      const uint16_t glyph_indices[],
      const PP_Point glyph_advances[]) = 0;
  virtual int32_t Navigate(PP_Instance instance,
                           PP_Resource request_info,
                           const char* target,
                           PP_Bool from_user_action) = 0;
  virtual PP_Bool IsRectTopmost(PP_Instance instance, const PP_Rect* rect) = 0;
  virtual void InvokePrinting(PP_Instance instance) = 0;

  static const SingletonResourceID kSingletonResourceID = FLASH_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif // PPAPI_THUNK_PPB_FLASH_FUNCTIONS_API_H_
