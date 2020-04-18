// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/common/create_video_encoder_params.h"

#include "ipc/ipc_message.h"

namespace media {

CreateVideoEncoderParams::CreateVideoEncoderParams()
    : input_format(PIXEL_FORMAT_UNKNOWN),
      output_profile(VIDEO_CODEC_PROFILE_UNKNOWN),
      initial_bitrate(0),
      encoder_route_id(MSG_ROUTING_NONE) {}

CreateVideoEncoderParams::~CreateVideoEncoderParams() = default;

}  // namespace media
