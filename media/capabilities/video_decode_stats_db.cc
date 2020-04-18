// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capabilities/video_decode_stats_db.h"

#include "media/capabilities/bucket_utility.h"

namespace media {

// static
VideoDecodeStatsDB::VideoDescKey
VideoDecodeStatsDB::VideoDescKey::MakeBucketedKey(
    VideoCodecProfile codec_profile,
    const gfx::Size& size,
    int frame_rate) {
  // Bucket size and framerate to prevent an explosion of one-off values in the
  // database and add basic guards against fingerprinting.
  return VideoDescKey(codec_profile, GetSizeBucket(size),
                      GetFpsBucket(frame_rate));
}

VideoDecodeStatsDB::VideoDescKey::VideoDescKey(VideoCodecProfile codec_profile,
                                               const gfx::Size& size,
                                               int frame_rate)
    : codec_profile(codec_profile), size(size), frame_rate(frame_rate) {}

VideoDecodeStatsDB::DecodeStatsEntry::DecodeStatsEntry(
    uint64_t frames_decoded,
    uint64_t frames_dropped,
    uint64_t frames_decoded_power_efficient)
    : frames_decoded(frames_decoded),
      frames_dropped(frames_dropped),
      frames_decoded_power_efficient(frames_decoded_power_efficient) {}

}  // namespace media
