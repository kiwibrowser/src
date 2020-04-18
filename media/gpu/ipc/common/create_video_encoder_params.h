// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_IPC_COMMON_CREATE_VIDEO_ENCODER_PARAMS_H_
#define MEDIA_GPU_IPC_COMMON_CREATE_VIDEO_ENCODER_PARAMS_H_

#include "media/base/video_codecs.h"
#include "media/base/video_types.h"
#include "ui/gfx/geometry/size.h"

namespace media {

struct CreateVideoEncoderParams {
  CreateVideoEncoderParams();
  ~CreateVideoEncoderParams();
  VideoPixelFormat input_format;
  gfx::Size input_visible_size;
  VideoCodecProfile output_profile;
  uint32_t initial_bitrate;
  int32_t encoder_route_id;
};

}  // namespace media

#endif  // MEDIA_GPU_IPC_COMMON_CREATE_VIDEO_ENCODER_PARAMS_H_
