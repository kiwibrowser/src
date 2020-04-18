// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_FLASH_RESOURCE_H_
#define PPAPI_PROXY_FLASH_RESOURCE_H_

#include <stdint.h>

#include "base/macros.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/thunk/ppb_flash_functions_api.h"

namespace ppapi {
namespace proxy {

class PluginDispatcher;

class FlashResource
    : public PluginResource,
      public thunk::PPB_Flash_Functions_API {
 public:
  FlashResource(Connection connection,
                PP_Instance instance,
                PluginDispatcher* plugin_dispatcher);
  ~FlashResource() override;

  // Resource override.
  thunk::PPB_Flash_Functions_API* AsPPB_Flash_Functions_API() override;

  // PPB_Flash_Functions_API implementation.
  PP_Var GetProxyForURL(PP_Instance instance, const std::string& url) override;
  void UpdateActivity(PP_Instance instance) override;
  PP_Bool SetCrashData(PP_Instance instance,
                       PP_FlashCrashKey key,
                       PP_Var value) override;
  double GetLocalTimeZoneOffset(PP_Instance instance, PP_Time t) override;
  PP_Var GetSetting(PP_Instance instance, PP_FlashSetting setting) override;
  void SetInstanceAlwaysOnTop(PP_Instance instance, PP_Bool on_top) override;
  PP_Bool DrawGlyphs(
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
      const PP_Point glyph_advances[]) override;
  int32_t Navigate(PP_Instance instance,
                   PP_Resource request_info,
                   const char* target,
                   PP_Bool from_user_action) override;
  PP_Bool IsRectTopmost(PP_Instance instance, const PP_Rect* rect) override;
  void InvokePrinting(PP_Instance instance) override;

 private:
  // Non-owning pointer to the PluginDispatcher that owns this object.
  PluginDispatcher* plugin_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(FlashResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_FLASH_RESOURCE_H_
