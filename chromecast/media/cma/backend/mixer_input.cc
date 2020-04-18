// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/mixer_input.h"

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "chromecast/media/cma/backend/filter_group.h"
#include "media/base/audio_bus.h"
#include "media/base/multi_channel_resampler.h"

namespace chromecast {
namespace media {

namespace {

const int64_t kMicrosecondsPerSecond = 1000 * 1000;
const int kDefaultSlewTimeMs = 15;

int RoundUpMultiple(int value, int multiple) {
  return multiple * ((value + (multiple - 1)) / multiple);
}

}  // namespace

MixerInput::MixerInput(Source* source,
                       int output_samples_per_second,
                       int read_size,
                       RenderingDelay initial_rendering_delay,
                       FilterGroup* filter_group)
    : source_(source),
      num_channels_(source->num_channels()),
      input_samples_per_second_(source->input_samples_per_second()),
      primary_(source->primary()),
      device_id_(source->device_id()),
      content_type_(source->content_type()),
      output_samples_per_second_(output_samples_per_second),
      filter_group_(filter_group),
      stream_volume_multiplier_(1.0f),
      type_volume_multiplier_(1.0f),
      mute_volume_multiplier_(1.0f),
      slew_volume_(kDefaultSlewTimeMs),
      resampler_buffered_frames_(0.0) {
  DCHECK(source_);
  DCHECK_GT(num_channels_, 0);
  DCHECK_GT(input_samples_per_second_, 0);
  DCHECK_GT(output_samples_per_second_, 0);

  int source_read_size = read_size;
  if (output_samples_per_second != input_samples_per_second_) {
    // Round up to nearest multiple of SincResampler::kKernelSize. The read size
    // must be > kKernelSize, so we round up to at least 2 * kKernelSize.
    source_read_size = std::max(source_->desired_read_size(),
                                ::media::SincResampler::kKernelSize + 1);
    source_read_size =
        RoundUpMultiple(source_read_size, ::media::SincResampler::kKernelSize);
    double resample_ratio = static_cast<double>(input_samples_per_second_) /
                            output_samples_per_second;
    resampler_ = std::make_unique<::media::MultiChannelResampler>(
        num_channels_, resample_ratio, source_read_size,
        base::BindRepeating(&MixerInput::ResamplerReadCallback,
                            base::Unretained(this)));
    resampler_->PrimeWithSilence();

    double resampler_queued_frames = resampler_->BufferedFrames();
    initial_rendering_delay.delay_microseconds +=
        static_cast<int64_t>(resampler_queued_frames * kMicrosecondsPerSecond /
                             input_samples_per_second_);
  }

  source_->InitializeAudioPlayback(source_read_size, initial_rendering_delay);

  if (filter_group_) {
    filter_group_->AddInput(this);
  }
}

MixerInput::~MixerInput() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (filter_group_) {
    filter_group_->RemoveInput(this);
  }
  source_->FinalizeAudioPlayback();
}

int MixerInput::FillAudioData(int num_frames,
                              RenderingDelay rendering_delay,
                              ::media::AudioBus* dest) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(dest);
  DCHECK_EQ(num_channels_, dest->channels());
  DCHECK_GE(dest->frames(), num_frames);

  if (resampler_) {
    mixer_rendering_delay_ = rendering_delay;
    // resampler_->BufferedFrames() gives incorrect values in the read callback,
    // so track the number of buffered frames ourselves.
    resampler_buffered_frames_ = resampler_->BufferedFrames();
    resampler_->Resample(num_frames, dest);
    return num_frames;
  } else {
    return source_->FillAudioPlaybackFrames(num_frames, rendering_delay, dest);
  }
}

void MixerInput::ResamplerReadCallback(int frame_delay,
                                       ::media::AudioBus* output) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  RenderingDelay delay = mixer_rendering_delay_;
  int64_t resampler_delay =
      std::round(resampler_buffered_frames_ * kMicrosecondsPerSecond /
                 input_samples_per_second_);
  delay.delay_microseconds += resampler_delay;

  const int needed_frames = output->frames();
  int filled = source_->FillAudioPlaybackFrames(needed_frames, delay, output);
  if (filled < needed_frames) {
    output->ZeroFramesPartial(filled, needed_frames - filled);
  }
  resampler_buffered_frames_ += output->frames();
}

void MixerInput::SignalError(Source::MixerError error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (filter_group_) {
    filter_group_->RemoveInput(this);
    filter_group_ = nullptr;
  }
  source_->OnAudioPlaybackError(error);
}

void MixerInput::VolumeScaleAccumulate(bool repeat_transition,
                                       const float* src,
                                       int frames,
                                       float* dest) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  slew_volume_.ProcessFMAC(repeat_transition, src, frames, 1, dest);
}

void MixerInput::SetVolumeMultiplier(float multiplier) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  stream_volume_multiplier_ = std::max(0.0f, multiplier);
  float target_volume = TargetVolume();
  LOG(INFO) << device_id_ << "(" << source_
            << "): stream volume = " << stream_volume_multiplier_
            << ", effective multiplier = " << target_volume;
  slew_volume_.SetMaxSlewTimeMs(kDefaultSlewTimeMs);
  slew_volume_.SetVolume(target_volume);
}

void MixerInput::SetContentTypeVolume(float volume, int fade_ms) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(content_type_ != AudioContentType::kOther);

  type_volume_multiplier_ = std::max(0.0f, std::min(volume, 1.0f));
  float target_volume = TargetVolume();
  LOG(INFO) << device_id_ << "(" << source_
            << "): type volume = " << type_volume_multiplier_
            << ", effective multiplier = " << target_volume;
  if (fade_ms < 0) {
    fade_ms = kDefaultSlewTimeMs;
  } else {
    LOG(INFO) << "Fade over " << fade_ms << " ms";
  }
  slew_volume_.SetMaxSlewTimeMs(fade_ms);
  slew_volume_.SetVolume(target_volume);
}

void MixerInput::SetMuted(bool muted) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(content_type_ != AudioContentType::kOther);

  mute_volume_multiplier_ = muted ? 0.0f : 1.0f;
  float target_volume = TargetVolume();
  LOG(INFO) << device_id_ << "(" << source_
            << "): mute volume = " << mute_volume_multiplier_
            << ", effective multiplier = " << target_volume;
  slew_volume_.SetMaxSlewTimeMs(kDefaultSlewTimeMs);
  slew_volume_.SetVolume(target_volume);
}

float MixerInput::TargetVolume() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  float volume = stream_volume_multiplier_ * type_volume_multiplier_ *
                 mute_volume_multiplier_;
  return std::max(0.0f, std::min(volume, 1.0f));
}

float MixerInput::InstantaneousVolume() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return slew_volume_.LastBufferMaxMultiplier();
}

}  // namespace media
}  // namespace chromecast
