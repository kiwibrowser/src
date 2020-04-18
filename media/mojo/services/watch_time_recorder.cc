// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/watch_time_recorder.h"

#include <algorithm>

#include "base/hash.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_piece.h"
#include "media/base/limits.h"
#include "media/base/watch_time_keys.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace media {

// The minimum amount of media playback which can elapse before we'll report
// watch time metrics for a playback.
constexpr base::TimeDelta kMinimumElapsedWatchTime =
    base::TimeDelta::FromSeconds(limits::kMinimumElapsedWatchTimeSecs);

// List of known AudioDecoder implementations; recorded to UKM, always add new
// values to the end and do not reorder or delete values from this list.
enum class AudioDecoderName : int {
  kUnknown = 0,     // Decoder name string is not recognized or n/a.
  kFFmpeg = 1,      // FFmpegAudioDecoder
  kMojo = 2,        // MojoAudioDecoder
  kDecrypting = 3,  // DecryptingAudioDecoder
};

// List of known VideoDecoder implementations; recorded to UKM, always add new
// values to the end and do not reorder or delete values from this list.
enum class VideoDecoderName : int {
  kUnknown = 0,     // Decoder name string is not recognized or n/a.
  kGpu = 1,         // GpuVideoDecoder
  kFFmpeg = 2,      // FFmpegVideoDecoder
  kVpx = 3,         // VpxVideoDecoder
  kAom = 4,         // AomVideoDecoder
  kMojo = 5,        // MojoVideoDecoder
  kDecrypting = 6,  // DecryptingVideoDecoder
};

static AudioDecoderName ConvertAudioDecoderNameToEnum(const std::string& name) {
  // See the unittest DISABLED_PrintExpectedDecoderNameHashes() for how these
  // values are computed.
  switch (base::PersistentHash(name)) {
    case 0xd39e0c2d:
      return AudioDecoderName::kFFmpeg;
    case 0xdaceafdb:
      return AudioDecoderName::kMojo;
    case 0xd39a2eda:
      return AudioDecoderName::kDecrypting;
    default:
      DLOG_IF(WARNING, !name.empty())
          << "Unknown decoder name encountered; metrics need updating: "
          << name;
  }
  return AudioDecoderName::kUnknown;
}

static VideoDecoderName ConvertVideoDecoderNameToEnum(const std::string& name) {
  // See the unittest DISABLED_PrintExpectedDecoderNameHashes() for how these
  // values are computed.
  switch (base::PersistentHash(name)) {
    case 0xacdee563:
      return VideoDecoderName::kFFmpeg;
    case 0x943f016f:
      return VideoDecoderName::kMojo;
    case 0xf66241b8:
      return VideoDecoderName::kGpu;
    case 0xb3802adb:
      return VideoDecoderName::kVpx;
    case 0xcff23b85:
      return VideoDecoderName::kAom;
    case 0xb52d52f5:
      return VideoDecoderName::kDecrypting;
    default:
      DLOG_IF(WARNING, !name.empty())
          << "Unknown decoder name encountered; metrics need updating: "
          << name;
  }
  return VideoDecoderName::kUnknown;
}

static void RecordWatchTimeInternal(
    base::StringPiece key,
    base::TimeDelta value,
    base::TimeDelta minimum = kMinimumElapsedWatchTime) {
  DCHECK(!key.empty());
  base::UmaHistogramCustomTimes(key.as_string(), value, minimum,
                                base::TimeDelta::FromHours(10), 50);
}

static void RecordMeanTimeBetweenRebuffers(base::StringPiece key,
                                           base::TimeDelta value) {
  DCHECK(!key.empty());

  // There are a maximum of 5 underflow events possible in a given 7s watch time
  // period, so the minimum value is 1.4s.
  RecordWatchTimeInternal(key, value, base::TimeDelta::FromSecondsD(1.4));
}

static void RecordDiscardedWatchTime(base::StringPiece key,
                                     base::TimeDelta value) {
  DCHECK(!key.empty());
  base::UmaHistogramCustomTimes(key.as_string(), value, base::TimeDelta(),
                                kMinimumElapsedWatchTime, 50);
}

static void RecordRebuffersCount(base::StringPiece key, int underflow_count) {
  DCHECK(!key.empty());
  base::UmaHistogramCounts100(key.as_string(), underflow_count);
}

WatchTimeRecorder::WatchTimeRecorder(mojom::PlaybackPropertiesPtr properties,
                                     const url::Origin& untrusted_top_origin,
                                     bool is_top_frame,
                                     uint64_t player_id)
    : properties_(std::move(properties)),
      untrusted_top_origin_(untrusted_top_origin),
      is_top_frame_(is_top_frame),
      player_id_(player_id),
      extended_metrics_keys_(
          {{WatchTimeKey::kAudioSrc, kMeanTimeBetweenRebuffersAudioSrc,
            kRebuffersCountAudioSrc, kDiscardedWatchTimeAudioSrc},
           {WatchTimeKey::kAudioMse, kMeanTimeBetweenRebuffersAudioMse,
            kRebuffersCountAudioMse, kDiscardedWatchTimeAudioMse},
           {WatchTimeKey::kAudioEme, kMeanTimeBetweenRebuffersAudioEme,
            kRebuffersCountAudioEme, kDiscardedWatchTimeAudioEme},
           {WatchTimeKey::kAudioVideoSrc,
            kMeanTimeBetweenRebuffersAudioVideoSrc,
            kRebuffersCountAudioVideoSrc, kDiscardedWatchTimeAudioVideoSrc},
           {WatchTimeKey::kAudioVideoMse,
            kMeanTimeBetweenRebuffersAudioVideoMse,
            kRebuffersCountAudioVideoMse, kDiscardedWatchTimeAudioVideoMse},
           {WatchTimeKey::kAudioVideoEme,
            kMeanTimeBetweenRebuffersAudioVideoEme,
            kRebuffersCountAudioVideoEme, kDiscardedWatchTimeAudioVideoEme}}) {}

WatchTimeRecorder::~WatchTimeRecorder() {
  FinalizeWatchTime({});
  RecordUkmPlaybackData();
}

void WatchTimeRecorder::RecordWatchTime(WatchTimeKey key,
                                        base::TimeDelta watch_time) {
  watch_time_info_[key] = watch_time;
}

void WatchTimeRecorder::FinalizeWatchTime(
    const std::vector<WatchTimeKey>& keys_to_finalize) {
  // If the filter set is empty, treat that as finalizing all keys; otherwise
  // only the listed keys will be finalized.
  const bool should_finalize_everything = keys_to_finalize.empty();

  // Record metrics to be finalized, but do not erase them yet; they are still
  // needed by for UKM and MTBR recording below.
  for (auto& kv : watch_time_info_) {
    if (!should_finalize_everything &&
        std::find(keys_to_finalize.begin(), keys_to_finalize.end(), kv.first) ==
            keys_to_finalize.end()) {
      continue;
    }

    // Report only certain keys to UMA and only if they have at met the minimum
    // watch time requirement. Otherwise, for SRC/MSE/EME keys, log them to the
    // discard metric.
    base::StringPiece key_str = ConvertWatchTimeKeyToStringForUma(kv.first);
    if (!key_str.empty()) {
      if (kv.second >= kMinimumElapsedWatchTime) {
        RecordWatchTimeInternal(key_str, kv.second);
      } else if (kv.second > base::TimeDelta()) {
        auto it = std::find_if(extended_metrics_keys_.begin(),
                               extended_metrics_keys_.end(),
                               [kv](const ExtendedMetricsKeyMap& map) {
                                 return map.watch_time_key == kv.first;
                               });
        if (it != extended_metrics_keys_.end())
          RecordDiscardedWatchTime(it->discard_key, kv.second);
      }
    }

    // At finalize, update the aggregate entry.
    aggregate_watch_time_info_[kv.first] += kv.second;
  }

  // If we're not finalizing everything, we're done after removing keys.
  if (!should_finalize_everything) {
    for (auto key : keys_to_finalize)
      watch_time_info_.erase(key);
    return;
  }

  // Check for watch times entries that have corresponding MTBR entries and
  // report the MTBR value using watch_time / |underflow_count|. Do this only
  // for foreground reporters since we only have UMA keys for foreground.
  if (!properties_->is_background && !properties_->is_muted) {
    for (auto& mapping : extended_metrics_keys_) {
      auto it = watch_time_info_.find(mapping.watch_time_key);
      if (it == watch_time_info_.end() || it->second < kMinimumElapsedWatchTime)
        continue;

      if (underflow_count_) {
        RecordMeanTimeBetweenRebuffers(mapping.mtbr_key,
                                       it->second / underflow_count_);
      }

      RecordRebuffersCount(mapping.smooth_rate_key, underflow_count_);
    }
  }

  // Ensure values are cleared in case the reporter is reused.
  total_underflow_count_ += underflow_count_;
  underflow_count_ = 0;
  watch_time_info_.clear();
}

void WatchTimeRecorder::OnError(PipelineStatus status) {
  pipeline_status_ = status;
}

void WatchTimeRecorder::SetAudioDecoderName(const std::string& name) {
  DCHECK(audio_decoder_name_.empty());
  audio_decoder_name_ = name;
}

void WatchTimeRecorder::SetVideoDecoderName(const std::string& name) {
  DCHECK(video_decoder_name_.empty());
  video_decoder_name_ = name;
}

void WatchTimeRecorder::SetAutoplayInitiated(bool value) {
  DCHECK(!autoplay_initiated_.has_value() || value == autoplay_initiated_);
  autoplay_initiated_ = value;
}

void WatchTimeRecorder::UpdateUnderflowCount(int32_t count) {
  underflow_count_ = count;
}

void WatchTimeRecorder::RecordUkmPlaybackData() {
  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  const int32_t source_id = ukm_recorder->GetNewSourceID();

  // TODO(crbug.com/787209): Stop getting origin from the renderer.
  ukm_recorder->UpdateSourceURL(source_id, untrusted_top_origin_.GetURL());
  ukm::builders::Media_BasicPlayback builder(source_id);

  builder.SetIsTopFrame(is_top_frame_);
  builder.SetIsBackground(properties_->is_background);
  builder.SetIsMuted(properties_->is_muted);
  builder.SetPlayerID(player_id_);

  bool recorded_all_metric = false;
  for (auto& kv : aggregate_watch_time_info_) {
    if (kv.first == WatchTimeKey::kAudioAll ||
        kv.first == WatchTimeKey::kAudioBackgroundAll ||
        kv.first == WatchTimeKey::kAudioVideoAll ||
        kv.first == WatchTimeKey::kAudioVideoMutedAll ||
        kv.first == WatchTimeKey::kAudioVideoBackgroundAll ||
        kv.first == WatchTimeKey::kVideoAll ||
        kv.first == WatchTimeKey::kVideoBackgroundAll) {
      // Only one of these keys should be present.
      DCHECK(!recorded_all_metric);
      recorded_all_metric = true;

      builder.SetWatchTime(kv.second.InMilliseconds());
      if (total_underflow_count_) {
        builder.SetMeanTimeBetweenRebuffers(
            (kv.second / total_underflow_count_).InMilliseconds());
      }
    } else if (kv.first == WatchTimeKey::kAudioAc ||
               kv.first == WatchTimeKey::kAudioBackgroundAc ||
               kv.first == WatchTimeKey::kAudioVideoAc ||
               kv.first == WatchTimeKey::kAudioVideoMutedAc ||
               kv.first == WatchTimeKey::kAudioVideoBackgroundAc ||
               kv.first == WatchTimeKey::kVideoAc ||
               kv.first == WatchTimeKey::kVideoBackgroundAc) {
      builder.SetWatchTime_AC(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioBattery ||
               kv.first == WatchTimeKey::kAudioBackgroundBattery ||
               kv.first == WatchTimeKey::kAudioVideoBattery ||
               kv.first == WatchTimeKey::kAudioVideoMutedBattery ||
               kv.first == WatchTimeKey::kAudioVideoBackgroundBattery ||
               kv.first == WatchTimeKey::kVideoBattery ||
               kv.first == WatchTimeKey::kVideoBackgroundBattery) {
      builder.SetWatchTime_Battery(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioNativeControlsOn ||
               kv.first == WatchTimeKey::kAudioVideoNativeControlsOn ||
               kv.first == WatchTimeKey::kAudioVideoMutedNativeControlsOn ||
               kv.first == WatchTimeKey::kVideoNativeControlsOn) {
      builder.SetWatchTime_NativeControlsOn(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioNativeControlsOff ||
               kv.first == WatchTimeKey::kAudioVideoNativeControlsOff ||
               kv.first == WatchTimeKey::kAudioVideoMutedNativeControlsOff ||
               kv.first == WatchTimeKey::kVideoNativeControlsOff) {
      builder.SetWatchTime_NativeControlsOff(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayFullscreen ||
               kv.first == WatchTimeKey::kAudioVideoMutedDisplayFullscreen ||
               kv.first == WatchTimeKey::kVideoDisplayFullscreen) {
      builder.SetWatchTime_DisplayFullscreen(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayInline ||
               kv.first == WatchTimeKey::kAudioVideoMutedDisplayInline ||
               kv.first == WatchTimeKey::kVideoDisplayInline) {
      builder.SetWatchTime_DisplayInline(kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayPictureInPicture ||
               kv.first ==
                   WatchTimeKey::kAudioVideoMutedDisplayPictureInPicture ||
               kv.first == WatchTimeKey::kVideoDisplayPictureInPicture) {
      builder.SetWatchTime_DisplayPictureInPicture(kv.second.InMilliseconds());
    }
  }

  // See note in mojom::PlaybackProperties about why we have both of these.
  builder.SetAudioCodec(properties_->audio_codec);
  builder.SetVideoCodec(properties_->video_codec);
  builder.SetHasAudio(properties_->has_audio);
  builder.SetHasVideo(properties_->has_video);

  // We convert decoder names to a hash and then translate that hash to a zero
  // valued enum to avoid burdening the rest of the decoder code base. This was
  // the simplest and most effective solution for the following reasons:
  //
  // - We can't report hashes to UKM since the privacy team worries they may
  //   end up as hashes of user data.
  // - Given that decoders are defined and implemented all over the code base
  //   it's unwieldly to have a single location which defines all decoder names.
  // - Due to the above, no single media/ location has access to all names.
  //
  builder.SetAudioDecoderName(
      static_cast<int64_t>(ConvertAudioDecoderNameToEnum(audio_decoder_name_)));
  builder.SetVideoDecoderName(
      static_cast<int64_t>(ConvertVideoDecoderNameToEnum(video_decoder_name_)));

  builder.SetIsEME(properties_->is_eme);
  builder.SetIsMSE(properties_->is_mse);
  builder.SetLastPipelineStatus(pipeline_status_);
  builder.SetRebuffersCount(total_underflow_count_);
  builder.SetVideoNaturalWidth(properties_->natural_size.width());
  builder.SetVideoNaturalHeight(properties_->natural_size.height());
  builder.SetAutoplayInitiated(autoplay_initiated_.value_or(false));
  builder.Record(ukm_recorder);

  aggregate_watch_time_info_.clear();
}

WatchTimeRecorder::ExtendedMetricsKeyMap::ExtendedMetricsKeyMap(
    const ExtendedMetricsKeyMap& copy)
    : ExtendedMetricsKeyMap(copy.watch_time_key,
                            copy.mtbr_key,
                            copy.smooth_rate_key,
                            copy.discard_key) {}

WatchTimeRecorder::ExtendedMetricsKeyMap::ExtendedMetricsKeyMap(
    WatchTimeKey watch_time_key,
    base::StringPiece mtbr_key,
    base::StringPiece smooth_rate_key,
    base::StringPiece discard_key)
    : watch_time_key(watch_time_key),
      mtbr_key(mtbr_key),
      smooth_rate_key(smooth_rate_key),
      discard_key(discard_key) {}

}  // namespace media
