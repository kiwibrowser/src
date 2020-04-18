// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_
#define MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_

#include <stdint.h>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "media/base/audio_codecs.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/watch_time_recorder.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "url/origin.h"

namespace media {

// See mojom::WatchTimeRecorder for documentation.
class MEDIA_MOJO_EXPORT WatchTimeRecorder : public mojom::WatchTimeRecorder {
 public:
  WatchTimeRecorder(mojom::PlaybackPropertiesPtr properties,
                    const url::Origin& untrusted_top_origin,
                    bool is_top_frame,
                    uint64_t player_id);
  ~WatchTimeRecorder() override;

  // mojom::WatchTimeRecorder implementation:
  void RecordWatchTime(WatchTimeKey key, base::TimeDelta watch_time) override;
  void FinalizeWatchTime(
      const std::vector<WatchTimeKey>& watch_time_keys) override;
  void OnError(PipelineStatus status) override;
  void SetAudioDecoderName(const std::string& name) override;
  void SetVideoDecoderName(const std::string& name) override;
  void SetAutoplayInitiated(bool value) override;

  void UpdateUnderflowCount(int32_t count) override;

 private:
  // Records a UKM event based on |aggregate_watch_time_info_|; only recorded
  // with a complete finalize (destruction or empty FinalizeWatchTime call).
  // Clears |aggregate_watch_time_info_| upon completion.
  void RecordUkmPlaybackData();

  const mojom::PlaybackPropertiesPtr properties_;

  // For privacy, only record the top origin. "Untrusted" signals that this
  // value comes from the renderer and should not be used for security checks.
  // TODO(crbug.com/787209): Stop getting origin from the renderer.
  const url::Origin untrusted_top_origin_;
  const bool is_top_frame_;

  // The provider ID which constructed this recorder. Used to record a UKM entry
  // at destruction that can be correlated with the final status for the
  // associated WebMediaPlayerImpl instance.
  const uint64_t player_id_;

  // Mapping of WatchTime metric keys to MeanTimeBetweenRebuffers (MTBR), smooth
  // rate (had zero rebuffers), and discard (<7s watch time) keys.
  struct ExtendedMetricsKeyMap {
    ExtendedMetricsKeyMap(const ExtendedMetricsKeyMap& copy);
    ExtendedMetricsKeyMap(WatchTimeKey watch_time_key,
                          base::StringPiece mtbr_key,
                          base::StringPiece smooth_rate_key,
                          base::StringPiece discard_key);
    const WatchTimeKey watch_time_key;
    const base::StringPiece mtbr_key;
    const base::StringPiece smooth_rate_key;
    const base::StringPiece discard_key;
  };
  const std::vector<ExtendedMetricsKeyMap> extended_metrics_keys_;

  using WatchTimeInfo = base::flat_map<WatchTimeKey, base::TimeDelta>;
  WatchTimeInfo watch_time_info_;

  // Sum of all watch time data since the last complete finalize.
  WatchTimeInfo aggregate_watch_time_info_;

  int underflow_count_ = 0;
  int total_underflow_count_ = 0;
  PipelineStatus pipeline_status_ = PIPELINE_OK;

  // Decoder name associated with this recorder. Reported to UKM as a
  // base::PersistentHash().
  std::string audio_decoder_name_;
  std::string video_decoder_name_;

  base::Optional<bool> autoplay_initiated_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorder);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_
