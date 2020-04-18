// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_VIDEO_DECODE_STATS_RECORDER_H_
#define MEDIA_MOJO_SERVICES_VIDEO_DECODE_STATS_RECORDER_H_

#include <stdint.h>
#include <string>

#include "base/time/time.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/video_decode_stats_recorder.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "url/gurl.h"

namespace media {

class VideoDecodePerfHistory;

// See mojom::VideoDecodeStatsRecorder for documentation.
class MEDIA_MOJO_EXPORT VideoDecodeStatsRecorder
    : public mojom::VideoDecodeStatsRecorder {
 public:
  // |perf_history| required to save decode stats to local database and report
  // metrics. Callers must ensure that |perf_history| outlives this object; may
  // be nullptr if database recording is currently disabled.
  VideoDecodeStatsRecorder(const url::Origin& untrusted_top_frame_origin,
                           bool is_top_frame,
                           uint64_t player_id,
                           VideoDecodePerfHistory* perf_history);
  ~VideoDecodeStatsRecorder() override;

  // mojom::VideoDecodeStatsRecorder implementation:
  void StartNewRecord(mojom::PredictionFeaturesPtr features) override;
  void UpdateRecord(mojom::PredictionTargetsPtr targets) override;

 private:
  // Save most recent stats values to disk. Called during destruction and upon
  // starting a new record.
  void FinalizeRecord();

  const url::Origin untrusted_top_frame_origin_;
  const bool is_top_frame_;
  VideoDecodePerfHistory* const perf_history_;
  const uint64_t player_id_;

  mojom::PredictionFeatures features_;
  mojom::PredictionTargets targets_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecodeStatsRecorder);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_VIDEO_DECODE_STATS_RECORDER_H_
