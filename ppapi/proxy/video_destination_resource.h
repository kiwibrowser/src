// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_VIDEO_DESTINATION_RESOURCE_H_
#define PPAPI_PROXY_VIDEO_DESTINATION_RESOURCE_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/thunk/ppb_video_destination_private_api.h"

struct PP_VideoFrame_Private;

namespace ppapi {

class TrackedCallback;

namespace proxy {

class PPAPI_PROXY_EXPORT VideoDestinationResource
    : public PluginResource,
      public thunk::PPB_VideoDestination_Private_API {
 public:
  VideoDestinationResource(Connection connection,
                           PP_Instance instance);
  ~VideoDestinationResource() override;

  // Resource overrides.
  thunk::PPB_VideoDestination_Private_API* AsPPB_VideoDestination_Private_API()
      override;

  // PPB_VideoDestination_Private_API implementation.
  int32_t Open(
      const PP_Var& stream_url,
      scoped_refptr<TrackedCallback> callback) override;
  int32_t PutFrame(const PP_VideoFrame_Private& frame) override;
  void Close() override;

 private:
  void OnPluginMsgOpenComplete(
      const ResourceMessageReplyParams& params);

  scoped_refptr<TrackedCallback> open_callback_;
  bool is_open_;

  DISALLOW_COPY_AND_ASSIGN(VideoDestinationResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_VIDEO_DESTINATION_RESOURCE_H_
