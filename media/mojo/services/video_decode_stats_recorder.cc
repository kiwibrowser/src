// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/video_decode_stats_recorder.h"

#include "base/memory/ptr_util.h"
#include "media/mojo/services/video_decode_perf_history.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#include "base/logging.h"

namespace media {

VideoDecodeStatsRecorder::VideoDecodeStatsRecorder(
    const url::Origin& untrusted_top_frame_origin,
    bool is_top_frame,
    uint64_t player_id,
    VideoDecodePerfHistory* perf_history)
    : untrusted_top_frame_origin_(untrusted_top_frame_origin),
      is_top_frame_(is_top_frame),
      perf_history_(perf_history),
      player_id_(player_id) {
  DVLOG(2) << __func__
           << " untrusted_top_frame_origin:" << untrusted_top_frame_origin
           << " is_top_frame:" << is_top_frame;
}

VideoDecodeStatsRecorder::~VideoDecodeStatsRecorder() {
  DVLOG(2) << __func__ << " Finalize for IPC disconnect";
  FinalizeRecord();
}

void VideoDecodeStatsRecorder::StartNewRecord(
    mojom::PredictionFeaturesPtr features) {
  DCHECK_NE(features->profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(features->frames_per_sec, 0);
  DCHECK(features->video_size.width() > 0 && features->video_size.height() > 0);

  features_ = *features;
  FinalizeRecord();

  DVLOG(2) << __func__ << "profile: " << features_.profile
           << " sz:" << features_.video_size.ToString()
           << " fps:" << features_.frames_per_sec;

  // Reinitialize to defaults.
  targets_ = mojom::PredictionTargets();
}

void VideoDecodeStatsRecorder::UpdateRecord(
    mojom::PredictionTargetsPtr targets) {
  DVLOG(3) << __func__ << " decoded:" << targets->frames_decoded
           << " dropped:" << targets->frames_dropped;

  // Dropped can never exceed decoded.
  DCHECK_LE(targets->frames_dropped, targets->frames_decoded);
  // Power efficient frames can never exceed decoded frames.
  DCHECK_LE(targets->frames_decoded_power_efficient, targets->frames_decoded);
  // Should never go backwards.
  DCHECK_GE(targets->frames_decoded, targets_.frames_decoded);
  DCHECK_GE(targets->frames_dropped, targets_.frames_dropped);
  DCHECK_GE(targets->frames_decoded_power_efficient,
            targets_.frames_decoded_power_efficient);

  targets_ = *targets;
}

void VideoDecodeStatsRecorder::FinalizeRecord() {
  if (features_.profile == VIDEO_CODEC_PROFILE_UNKNOWN ||
      targets_.frames_decoded == 0 || !perf_history_) {
    return;
  }

  DVLOG(2) << __func__ << " profile: " << features_.profile
           << " size:" << features_.video_size.ToString()
           << " fps:" << features_.frames_per_sec
           << " decoded:" << targets_.frames_decoded
           << " dropped:" << targets_.frames_dropped
           << " power efficient decoded:"
           << targets_.frames_decoded_power_efficient;

  perf_history_->SavePerfRecord(untrusted_top_frame_origin_, is_top_frame_,
                                features_, targets_, player_id_);
}

}  // namespace media
