// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_TRUETYPE_FONT_RESOURCE_H_
#define PPAPI_PROXY_TRUETYPE_FONT_RESOURCE_H_

#include <stdint.h>

#include <queue>
#include <string>

#include "base/macros.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/ppb_truetype_font_api.h"

namespace ppapi {

class TrackedCallback;

namespace proxy {

struct SerializedTrueTypeFontDesc;

class PPAPI_PROXY_EXPORT TrueTypeFontResource
    : public PluginResource,
      public thunk::PPB_TrueTypeFont_API {
 public:
  TrueTypeFontResource(Connection connection,
                       PP_Instance instance,
                       const PP_TrueTypeFontDesc_Dev& desc);
  ~TrueTypeFontResource() override;

  // Resource implementation.
  thunk::PPB_TrueTypeFont_API* AsPPB_TrueTypeFont_API() override;

  // PPB_TrueTypeFont_API implementation.
  int32_t Describe(
      PP_TrueTypeFontDesc_Dev* desc,
      scoped_refptr<TrackedCallback> callback) override;
  int32_t GetTableTags(
      const PP_ArrayOutput& output,
      scoped_refptr<TrackedCallback> callback) override;
  int32_t GetTable(
      uint32_t table,
      int32_t offset,
      int32_t max_data_length,
      const PP_ArrayOutput& output,
      scoped_refptr<TrackedCallback> callback) override;

  // PluginResource implementation.
  void OnReplyReceived(const ResourceMessageReplyParams& params,
                       const IPC::Message& msg) override;

 private:
  void OnPluginMsgCreateComplete(
      const ResourceMessageReplyParams& params,
      const ppapi::proxy::SerializedTrueTypeFontDesc& desc,
      int32_t result);
  void OnPluginMsgGetTableTagsComplete(
      scoped_refptr<TrackedCallback> callback,
      PP_ArrayOutput array_output,
      const ResourceMessageReplyParams& params,
      const std::vector<uint32_t>& data);
  void OnPluginMsgGetTableComplete(
      scoped_refptr<TrackedCallback> callback,
      PP_ArrayOutput array_output,
      const ResourceMessageReplyParams& params,
      const std::string& data);

  int32_t create_result_;
  // Valid only when create_result_ == PP_OK.
  ppapi::proxy::SerializedTrueTypeFontDesc desc_;

  // Params for pending Describe call.
  PP_TrueTypeFontDesc_Dev* describe_desc_;
  scoped_refptr<TrackedCallback> describe_callback_;

  DISALLOW_COPY_AND_ASSIGN(TrueTypeFontResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_TRUETYPE_FONT_RESOURCE_H_
