// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/video_source_private.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_video_source_private.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/private/video_frame_private.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_VideoSource_Private_0_1>() {
  return PPB_VIDEOSOURCE_PRIVATE_INTERFACE_0_1;
}

}  // namespace

VideoSource_Private::VideoSource_Private() {
}

VideoSource_Private::VideoSource_Private(const InstanceHandle& instance) {
  if (!has_interface<PPB_VideoSource_Private_0_1>())
    return;
  PassRefFromConstructor(get_interface<PPB_VideoSource_Private_0_1>()->Create(
      instance.pp_instance()));
}

VideoSource_Private::VideoSource_Private(const VideoSource_Private& other)
    : Resource(other) {
}

VideoSource_Private::VideoSource_Private(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

int32_t VideoSource_Private::Open(const Var& stream_url,
                                  const CompletionCallback& cc) {
  if (has_interface<PPB_VideoSource_Private_0_1>()) {
    int32_t result =
        get_interface<PPB_VideoSource_Private_0_1>()->Open(
            pp_resource(),
            stream_url.pp_var(), cc.pp_completion_callback());
    return result;
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t VideoSource_Private::GetFrame(
    const CompletionCallbackWithOutput<VideoFrame_Private>& cc) {
  if (has_interface<PPB_VideoSource_Private_0_1>()) {
    return get_interface<PPB_VideoSource_Private_0_1>()->GetFrame(
        pp_resource(),
        cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

void VideoSource_Private::Close() {
  if (has_interface<PPB_VideoSource_Private_0_1>()) {
    get_interface<PPB_VideoSource_Private_0_1>()->Close(pp_resource());
  }
}

}  // namespace pp
