// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MEDIA_METRICS_PROVIDER_H_
#define MEDIA_MOJO_SERVICES_MEDIA_METRICS_PROVIDER_H_

#include <stdint.h>

#include "media/base/pipeline_status.h"
#include "media/base/timestamp_constants.h"
#include "media/mojo/interfaces/media_metrics_provider.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "url/origin.h"

namespace media {
class VideoDecodePerfHistory;

// See mojom::MediaMetricsProvider for documentation.
class MEDIA_MOJO_EXPORT MediaMetricsProvider
    : public mojom::MediaMetricsProvider {
 public:
  explicit MediaMetricsProvider(VideoDecodePerfHistory* perf_history);
  ~MediaMetricsProvider() override;

  // Creates a MediaMetricsProvider, |perf_history| may be nullptr if perf
  // history database recording is disabled.
  static void Create(VideoDecodePerfHistory* perf_history,
                     mojom::MediaMetricsProviderRequest request);

 private:
  // mojom::MediaMetricsProvider implementation:
  void Initialize(bool is_mse,
                  bool is_top_frame,
                  const url::Origin& untrusted_top_origin) override;
  void OnError(PipelineStatus status) override;
  void SetIsEME() override;
  void SetTimeToMetadata(base::TimeDelta elapsed) override;
  void SetTimeToFirstFrame(base::TimeDelta elapsed) override;
  void SetTimeToPlayReady(base::TimeDelta elapsed) override;
  void AcquireWatchTimeRecorder(
      mojom::PlaybackPropertiesPtr properties,
      mojom::WatchTimeRecorderRequest request) override;
  void AcquireVideoDecodeStatsRecorder(
      mojom::VideoDecodeStatsRecorderRequest request) override;

  // Session unique ID which maps to a given WebMediaPlayerImpl instances. Used
  // to coordinate multiply logged events with a singly logged metric.
  const uint64_t player_id_;

  // These values are not always sent but have known defaults.
  PipelineStatus pipeline_status_ = PIPELINE_OK;
  bool is_eme_ = false;

  // The values below are only set if |initialized_| is true.
  bool initialized_ = false;
  bool is_mse_;
  bool is_top_frame_;
  url::Origin untrusted_top_origin_;

  base::TimeDelta time_to_metadata_ = kNoTimestamp;
  base::TimeDelta time_to_first_frame_ = kNoTimestamp;
  base::TimeDelta time_to_play_ready_ = kNoTimestamp;

  VideoDecodePerfHistory* const perf_history_;

  DISALLOW_COPY_AND_ASSIGN(MediaMetricsProvider);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MEDIA_METRICS_PROVIDER_H_
