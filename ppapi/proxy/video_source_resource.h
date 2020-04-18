// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_VIDEO_SOURCE_RESOURCE_H_
#define PPAPI_PROXY_VIDEO_SOURCE_RESOURCE_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/thunk/ppb_video_source_private_api.h"

struct PP_ImageDataDesc;
struct PP_VideoFrame_Private;

namespace ppapi {

class TrackedCallback;

namespace proxy {

class PPAPI_PROXY_EXPORT VideoSourceResource
    : public PluginResource,
      public thunk::PPB_VideoSource_Private_API {
 public:
  VideoSourceResource(Connection connection,
                      PP_Instance instance);
  ~VideoSourceResource() override;

  // Resource overrides.
  thunk::PPB_VideoSource_Private_API* AsPPB_VideoSource_Private_API() override;

  // PPB_VideoSource_Private_API implementation.
  int32_t Open(
      const PP_Var& stream_url,
      scoped_refptr<TrackedCallback> callback) override;
  int32_t GetFrame(
      PP_VideoFrame_Private* frame,
      scoped_refptr<TrackedCallback> callback) override;
  void Close() override;

 private:
  void OnPluginMsgOpenComplete(
      const ResourceMessageReplyParams& reply_params);
  void OnPluginMsgGetFrameComplete(
      PP_VideoFrame_Private* frame,
      const ResourceMessageReplyParams& reply_params,
      const HostResource& image_data,
      const PP_ImageDataDesc& image_desc_data,
      PP_TimeTicks timestamp);

  scoped_refptr<TrackedCallback> open_callback_;
  scoped_refptr<TrackedCallback> get_frame_callback_;
  bool is_open_;

  DISALLOW_COPY_AND_ASSIGN(VideoSourceResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_VIDEO_SOURCE_RESOURCE_H_
