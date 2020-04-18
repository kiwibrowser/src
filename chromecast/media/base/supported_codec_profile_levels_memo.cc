// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/base/supported_codec_profile_levels_memo.h"

#include "base/logging.h"
#include "chromecast/public/media/decoder_config.h"

namespace chromecast {
namespace media {

SupportedCodecProfileLevelsMemo::SupportedCodecProfileLevelsMemo()
    : codec_profile_levels_(), thread_checker_() {}

SupportedCodecProfileLevelsMemo::~SupportedCodecProfileLevelsMemo() {}

void SupportedCodecProfileLevelsMemo::AddSupportedCodecProfileLevel(
    CodecProfileLevel codec_profile_level) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(2) << __func__ << "(" << codec_profile_level.codec << ", "
           << codec_profile_level.profile << ", " << codec_profile_level.level
           << ")";
  codec_profile_levels_.push_back(codec_profile_level);
}

bool SupportedCodecProfileLevelsMemo::IsSupportedVideoConfig(
    VideoCodec codec,
    VideoProfile profile,
    int level) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__ << "(" << codec << ", " << profile << ", " << level
           << ")";
  for (const auto& supported_profile_info : codec_profile_levels_) {
    if (codec == supported_profile_info.codec &&
        profile == supported_profile_info.profile &&
        level <= supported_profile_info.level) {
      return true;
    }
  }
  return false;
}

}  // namespace media
}  // namespace chromecast
