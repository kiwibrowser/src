// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/pipeline_status.h"

namespace media {

PipelineStatistics::PipelineStatistics() = default;
PipelineStatistics::PipelineStatistics(const PipelineStatistics& other) =
    default;
PipelineStatistics::~PipelineStatistics() = default;

bool operator==(const PipelineStatistics& first,
                const PipelineStatistics& second) {
  return first.audio_bytes_decoded == second.audio_bytes_decoded &&
         first.video_bytes_decoded == second.video_bytes_decoded &&
         first.video_frames_decoded == second.video_frames_decoded &&
         first.video_frames_dropped == second.video_frames_dropped &&
         first.video_frames_decoded_power_efficient ==
             second.video_frames_decoded_power_efficient &&
         first.audio_memory_usage == second.audio_memory_usage &&
         first.video_memory_usage == second.video_memory_usage &&
         first.video_keyframe_distance_average ==
             second.video_keyframe_distance_average &&
         first.video_frame_duration_average ==
             second.video_frame_duration_average &&
         first.audio_decoder_name == second.audio_decoder_name &&
         first.video_decoder_name == second.video_decoder_name;
}

bool operator!=(const PipelineStatistics& first,
                const PipelineStatistics& second) {
  return !(first == second);
}

}  // namespace media
