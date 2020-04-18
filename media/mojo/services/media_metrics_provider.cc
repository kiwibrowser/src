// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_metrics_provider.h"

#include "media/mojo/services/video_decode_stats_recorder.h"
#include "media/mojo/services/watch_time_recorder.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace media {

constexpr char kInvalidInitialize[] = "Initialize() was not called correctly.";

static uint64_t g_player_id = 0;

MediaMetricsProvider::MediaMetricsProvider(VideoDecodePerfHistory* perf_history)
    : player_id_(g_player_id++), perf_history_(perf_history) {}

MediaMetricsProvider::~MediaMetricsProvider() {
  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder || !initialized_)
    return;

  const int32_t source_id = ukm_recorder->GetNewSourceID();

  // TODO(crbug.com/787209): Stop getting origin from the renderer.
  ukm_recorder->UpdateSourceURL(source_id, untrusted_top_origin_.GetURL());
  ukm::builders::Media_WebMediaPlayerState builder(source_id);
  builder.SetPlayerID(player_id_);
  builder.SetIsTopFrame(is_top_frame_);
  builder.SetIsEME(is_eme_);
  builder.SetIsMSE(is_mse_);
  builder.SetFinalPipelineStatus(pipeline_status_);

  if (time_to_metadata_ != kNoTimestamp)
    builder.SetTimeToMetadata(time_to_metadata_.InMilliseconds());
  if (time_to_first_frame_ != kNoTimestamp)
    builder.SetTimeToFirstFrame(time_to_first_frame_.InMilliseconds());
  if (time_to_play_ready_ != kNoTimestamp)
    builder.SetTimeToPlayReady(time_to_play_ready_.InMilliseconds());

  builder.Record(ukm_recorder);
}

// static
void MediaMetricsProvider::Create(VideoDecodePerfHistory* perf_history,
                                  mojom::MediaMetricsProviderRequest request) {
  mojo::MakeStrongBinding(std::make_unique<MediaMetricsProvider>(perf_history),
                          std::move(request));
}

void MediaMetricsProvider::Initialize(bool is_mse,
                                      bool is_top_frame,
                                      const url::Origin& untrusted_top_origin) {
  if (initialized_) {
    mojo::ReportBadMessage(kInvalidInitialize);
    return;
  }

  is_mse_ = is_mse;
  is_top_frame_ = is_top_frame;
  untrusted_top_origin_ = untrusted_top_origin;
  initialized_ = true;
}

void MediaMetricsProvider::OnError(PipelineStatus status) {
  DCHECK(initialized_);
  pipeline_status_ = status;
}

void MediaMetricsProvider::SetIsEME() {
  // This may be called before Initialize().
  is_eme_ = true;
}

void MediaMetricsProvider::SetTimeToMetadata(base::TimeDelta elapsed) {
  DCHECK(initialized_);
  DCHECK_EQ(time_to_metadata_, kNoTimestamp);
  time_to_metadata_ = elapsed;
}

void MediaMetricsProvider::SetTimeToFirstFrame(base::TimeDelta elapsed) {
  DCHECK(initialized_);
  DCHECK_EQ(time_to_first_frame_, kNoTimestamp);
  time_to_first_frame_ = elapsed;
}

void MediaMetricsProvider::SetTimeToPlayReady(base::TimeDelta elapsed) {
  DCHECK(initialized_);
  DCHECK_EQ(time_to_play_ready_, kNoTimestamp);
  time_to_play_ready_ = elapsed;
}

void MediaMetricsProvider::AcquireWatchTimeRecorder(
    mojom::PlaybackPropertiesPtr properties,
    mojom::WatchTimeRecorderRequest request) {
  if (!initialized_) {
    mojo::ReportBadMessage(kInvalidInitialize);
    return;
  }

  mojo::MakeStrongBinding(std::make_unique<WatchTimeRecorder>(
                              std::move(properties), untrusted_top_origin_,
                              is_top_frame_, player_id_),
                          std::move(request));
}

void MediaMetricsProvider::AcquireVideoDecodeStatsRecorder(
    mojom::VideoDecodeStatsRecorderRequest request) {
  if (!initialized_) {
    mojo::ReportBadMessage(kInvalidInitialize);
    return;
  }

  mojo::MakeStrongBinding(
      std::make_unique<VideoDecodeStatsRecorder>(
          untrusted_top_origin_, is_top_frame_, player_id_, perf_history_),
      std::move(request));
}

}  // namespace media
