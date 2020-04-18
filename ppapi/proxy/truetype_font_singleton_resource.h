// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_TRUETYPE_FONT_SINGLETON_RESOURCE_H_
#define PPAPI_PROXY_TRUETYPE_FONT_SINGLETON_RESOURCE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/thunk/ppb_truetype_font_singleton_api.h"

namespace ppapi {

class TrackedCallback;

namespace proxy {

struct SerializedTrueTypeFontDesc;

// This handles the singleton calls (that don't take a PP_Resource parameter)
// on the TrueType font interface.
class TrueTypeFontSingletonResource
    : public PluginResource,
      public thunk::PPB_TrueTypeFont_Singleton_API {
 public:
  TrueTypeFontSingletonResource(Connection connection, PP_Instance instance);
  ~TrueTypeFontSingletonResource() override;

  // Resource override.
  thunk::PPB_TrueTypeFont_Singleton_API* AsPPB_TrueTypeFont_Singleton_API()
      override;

  // thunk::PPB_TrueTypeFont_Singleton_API implementation.
  int32_t GetFontFamilies(
      PP_Instance instance,
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) override;
  int32_t GetFontsInFamily(
      PP_Instance instance,
      PP_Var family,
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) override;

 private:
  void OnPluginMsgGetFontFamiliesComplete(
      scoped_refptr<TrackedCallback> callback,
      PP_ArrayOutput array_output,
      const ResourceMessageReplyParams& params,
      const std::vector<std::string>& data);
  void OnPluginMsgGetFontsInFamilyComplete(
      scoped_refptr<TrackedCallback> callback,
      PP_ArrayOutput array_output,
      const ResourceMessageReplyParams& params,
      const std::vector<SerializedTrueTypeFontDesc>& fonts);

  DISALLOW_COPY_AND_ASSIGN(TrueTypeFontSingletonResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_TRUETYPE_FONT_SINGLETON_RESOURCE_H_
