// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DECODE_CAPABILITIES_
#define MEDIA_BASE_DECODE_CAPABILITIES_

#include "media/base/audio_codecs.h"
#include "media/base/media_export.h"
#include "media/base/video_codecs.h"
#include "media/base/video_color_space.h"

namespace media {

// APIs to media's decoder capabilities. Embedders may customize decoder
// capabilities via MediaClient. See usage in mime_util_internal.cc.
struct MEDIA_EXPORT AudioConfig {
  AudioCodec codec;
};

struct MEDIA_EXPORT VideoConfig {
  VideoCodec codec;
  VideoCodecProfile profile;
  int level;
  VideoColorSpace color_space;
};

MEDIA_EXPORT bool IsSupportedAudioConfig(const AudioConfig& config);
MEDIA_EXPORT bool IsSupportedVideoConfig(const VideoConfig& config);

}  // namespace media

#endif  // MEDIA_BASE_DECODE_CAPABILITIES_
