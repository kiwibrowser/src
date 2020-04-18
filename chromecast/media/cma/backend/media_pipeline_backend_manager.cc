// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/media_pipeline_backend_manager.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/time/time.h"
#include "chromecast/base/metrics/cast_metrics_helper.h"
#include "chromecast/chromecast_buildflags.h"
#include "chromecast/media/cma/backend/audio_decoder_wrapper.h"
#include "chromecast/media/cma/backend/cma_backend.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_wrapper.h"
#include "chromecast/public/volume_control.h"

#define RUN_ON_MEDIA_THREAD(method, ...)                              \
  media_task_runner_->PostTask(                                       \
      FROM_HERE, base::BindOnce(&MediaPipelineBackendManager::method, \
                                weak_factory_.GetWeakPtr(), ##__VA_ARGS__));

#define MAKE_SURE_MEDIA_THREAD(method, ...)            \
  if (!media_task_runner_->BelongsToCurrentThread()) { \
    RUN_ON_MEDIA_THREAD(method, ##__VA_ARGS__)         \
    return;                                            \
  }

namespace chromecast {
namespace media {

namespace {

#if BUILDFLAG(IS_CAST_AUDIO_ONLY) || BUILDFLAG(ENABLE_ASSISTANT)
constexpr int kAudioDecoderLimit = std::numeric_limits<int>::max();
#else
constexpr int kAudioDecoderLimit = 1;
#endif

constexpr base::TimeDelta kPowerSaveWaitTime = base::TimeDelta::FromSeconds(5);

}  // namespace

MediaPipelineBackendManager::MediaPipelineBackendManager(
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner)
    : media_task_runner_(std::move(media_task_runner)),
      playing_audio_streams_count_(0),
      playing_noneffects_audio_streams_count_(0),
      allow_volume_feedback_observers_(
          new base::ObserverListThreadSafe<AllowVolumeFeedbackObserver>()),
      global_volume_multipliers_({{AudioContentType::kMedia, 1.0f},
                                  {AudioContentType::kAlarm, 1.0f},
                                  {AudioContentType::kCommunication, 1.0f}},
                                 base::KEEP_FIRST_OF_DUPES),
      buffer_delegate_(nullptr),
      weak_factory_(this) {
  DCHECK(media_task_runner_);
  for (int i = 0; i < NUM_DECODER_TYPES; ++i) {
    decoder_count_[i] = 0;
  }
}

MediaPipelineBackendManager::~MediaPipelineBackendManager() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
}

std::unique_ptr<CmaBackend>
MediaPipelineBackendManager::CreateMediaPipelineBackend(
    const media::MediaPipelineDeviceParams& params) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  return std::make_unique<MediaPipelineBackendWrapper>(params, this);
}

bool MediaPipelineBackendManager::IncrementDecoderCount(DecoderType type) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(type < NUM_DECODER_TYPES);
  const int limit = (type == AUDIO_DECODER) ? kAudioDecoderLimit : 1;
  if (decoder_count_[type] >= limit) {
    LOG(WARNING) << "Decoder limit reached for type " << type;
    return false;
  }

  ++decoder_count_[type];
  return true;
}

void MediaPipelineBackendManager::DecrementDecoderCount(DecoderType type) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(type < NUM_DECODER_TYPES);
  DCHECK_GT(decoder_count_[type], 0);

  decoder_count_[type]--;
}

void MediaPipelineBackendManager::UpdatePlayingAudioCount(bool sfx,
                                                          int change) {
  DCHECK(change == -1 || change == 1) << "bad count change: " << change;

  bool had_playing_audio_streams = (playing_audio_streams_count_ > 0);
  playing_audio_streams_count_ += change;
  DCHECK_GE(playing_audio_streams_count_, 0);
  if (VolumeControl::SetPowerSaveMode) {
    if (playing_audio_streams_count_ == 0) {
      power_save_timer_.Start(FROM_HERE, kPowerSaveWaitTime, this,
                              &MediaPipelineBackendManager::EnterPowerSaveMode);
    } else if (!had_playing_audio_streams && playing_audio_streams_count_ > 0) {
      power_save_timer_.Stop();
      metrics::CastMetricsHelper::GetInstance()->RecordSimpleAction(
          "Cast.Platform.VolumeControl.PowerSaveOff");
      VolumeControl::SetPowerSaveMode(false);
    }
  }

  if (sfx) {
    return;
  }

  // Volume feedback sounds are only allowed when there are no non-effects
  // audio streams playing.
  bool prev_allow_feedback = (playing_noneffects_audio_streams_count_ == 0);
  playing_noneffects_audio_streams_count_ += change;
  DCHECK_GE(playing_noneffects_audio_streams_count_, 0);
  bool new_allow_feedback = (playing_noneffects_audio_streams_count_ == 0);

  if (new_allow_feedback != prev_allow_feedback) {
    allow_volume_feedback_observers_->Notify(
        FROM_HERE, &AllowVolumeFeedbackObserver::AllowVolumeFeedbackSounds,
        new_allow_feedback);
  }
}

void MediaPipelineBackendManager::EnterPowerSaveMode() {
  DCHECK_EQ(playing_audio_streams_count_, 0);
  DCHECK(VolumeControl::SetPowerSaveMode);
  metrics::CastMetricsHelper::GetInstance()->RecordSimpleAction(
      "Cast.Platform.VolumeControl.PowerSaveOn");
  VolumeControl::SetPowerSaveMode(true);
}

void MediaPipelineBackendManager::AddAllowVolumeFeedbackObserver(
    AllowVolumeFeedbackObserver* observer) {
  allow_volume_feedback_observers_->AddObserver(observer);
}

void MediaPipelineBackendManager::RemoveAllowVolumeFeedbackObserver(
    AllowVolumeFeedbackObserver* observer) {
  allow_volume_feedback_observers_->RemoveObserver(observer);
}

void MediaPipelineBackendManager::LogicalPause(CmaBackend* backend) {
  MediaPipelineBackendWrapper* wrapper =
      static_cast<MediaPipelineBackendWrapper*>(backend);
  wrapper->LogicalPause();
}

void MediaPipelineBackendManager::LogicalResume(CmaBackend* backend) {
  MediaPipelineBackendWrapper* wrapper =
      static_cast<MediaPipelineBackendWrapper*>(backend);
  wrapper->LogicalResume();
}

void MediaPipelineBackendManager::AddExtraPlayingStream(bool sfx) {
  UpdatePlayingAudioCount(sfx, 1);
}

void MediaPipelineBackendManager::RemoveExtraPlayingStream(bool sfx) {
  UpdatePlayingAudioCount(sfx, -1);
}

void MediaPipelineBackendManager::SetGlobalVolumeMultiplier(
    AudioContentType type,
    float multiplier) {
  MAKE_SURE_MEDIA_THREAD(SetGlobalVolumeMultiplier, type, multiplier);
  DCHECK_GE(multiplier, 0.0f);
  global_volume_multipliers_[type] = multiplier;
  for (auto* a : audio_decoders_) {
    if (a->content_type() == type) {
      a->SetGlobalVolumeMultiplier(multiplier);
    }
  }
}

void MediaPipelineBackendManager::SetBufferDelegate(
    BufferDelegate* buffer_delegate) {
  MAKE_SURE_MEDIA_THREAD(SetBufferDelegate, buffer_delegate);
  DCHECK(buffer_delegate);
  DCHECK(!buffer_delegate_);
  buffer_delegate_ = buffer_delegate;
}

void MediaPipelineBackendManager::AddAudioDecoder(
    AudioDecoderWrapper* decoder) {
  DCHECK(decoder);
  audio_decoders_.insert(decoder);
  decoder->SetGlobalVolumeMultiplier(
      global_volume_multipliers_[decoder->content_type()]);
}

void MediaPipelineBackendManager::RemoveAudioDecoder(
    AudioDecoderWrapper* decoder) {
  audio_decoders_.erase(decoder);
}

}  // namespace media
}  // namespace chromecast
