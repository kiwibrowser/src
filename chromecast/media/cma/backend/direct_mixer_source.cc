// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/direct_mixer_source.h"

#include "base/logging.h"
#include "chromecast/media/cma/backend/stream_mixer.h"
#include "chromecast/public/media/direct_audio_source.h"
#include "chromecast/public/media/media_pipeline_device_params.h"
#include "media/base/audio_bus.h"

namespace chromecast {
namespace media {

namespace {

std::string AudioContentTypeToString(media::AudioContentType type) {
  switch (type) {
    case media::AudioContentType::kAlarm:
      return "alarm";
    case media::AudioContentType::kCommunication:
      return "communication";
    default:
      return "media";
  }
}

}  // namespace

// static
DirectAudioSourceToken* CastMediaShlib::AddDirectAudioSource(
    DirectAudioSource* source,
    const MediaPipelineDeviceParams& params,
    int playout_channel) {
  DCHECK(source);
  return new DirectMixerSource(source, params, playout_channel);
}

// static
void CastMediaShlib::RemoveDirectAudioSource(DirectAudioSourceToken* token) {
  DirectMixerSource* source = static_cast<DirectMixerSource*>(token);
  source->Remove();
}

// static
void CastMediaShlib::SetDirectAudioSourceVolume(DirectAudioSourceToken* token,
                                                float multiplier) {
  DirectMixerSource* source = static_cast<DirectMixerSource*>(token);
  source->SetVolumeMultiplier(multiplier);
}

DirectMixerSource::DirectMixerSource(DirectAudioSource* direct_source,
                                     const MediaPipelineDeviceParams& params,
                                     int playout_channel)
    : source_(direct_source),
      num_channels_(source_->GetNumChannels()),
      input_samples_per_second_(source_->GetSampleRate()),
      primary_(params.audio_type !=
               MediaPipelineDeviceParams::kAudioStreamSoundEffects),
      device_id_(params.device_id),
      content_type_(params.content_type),
      playout_channel_(playout_channel),
      mixer_(StreamMixer::Get()),
      channel_vector_(num_channels_) {
  LOG(INFO) << "Create " << device_id_ << " (" << this
            << "), content type = " << AudioContentTypeToString(content_type_);
  DCHECK(source_);
  DCHECK(mixer_);

  mixer_->AddInput(this);
}

DirectMixerSource::~DirectMixerSource() {
  LOG(INFO) << "Destroy " << device_id_ << " (" << this << ")";
}

int DirectMixerSource::num_channels() {
  return num_channels_;
}
int DirectMixerSource::input_samples_per_second() {
  return input_samples_per_second_;
}
bool DirectMixerSource::primary() {
  return primary_;
}
const std::string& DirectMixerSource::device_id() {
  return device_id_;
}
AudioContentType DirectMixerSource::content_type() {
  return content_type_;
}
int DirectMixerSource::desired_read_size() {
  return source_->GetDesiredFillSize();
}
int DirectMixerSource::playout_channel() {
  return playout_channel_;
}

void DirectMixerSource::SetVolumeMultiplier(float multiplier) {
  mixer_->SetVolumeMultiplier(this, multiplier);
}

void DirectMixerSource::InitializeAudioPlayback(
    int read_size,
    RenderingDelay initial_rendering_delay) {
  source_->InitializeAudioPlayback(read_size, initial_rendering_delay);
}

int DirectMixerSource::FillAudioPlaybackFrames(int num_frames,
                                               RenderingDelay rendering_delay,
                                               ::media::AudioBus* buffer) {
  DCHECK(buffer);
  DCHECK_EQ(num_channels_, buffer->channels());
  DCHECK_GE(buffer->frames(), num_frames);
  for (int c = 0; c < num_channels_; ++c) {
    channel_vector_[c] = buffer->channel(c);
  }

  return source_->FillAudioPlaybackFrames(num_frames, rendering_delay,
                                          channel_vector_);
}

void DirectMixerSource::OnAudioPlaybackError(MixerError error) {
  if (error == MixerError::kInputIgnored) {
    LOG(INFO) << "Mixer input " << device_id_ << " (" << this << ")"
              << " now being ignored due to output sample rate change";
  }
  source_->OnAudioPlaybackError();
}

void DirectMixerSource::Remove() {
  LOG(INFO) << "Remove " << device_id_ << " (" << this << ")";
  mixer_->RemoveInput(this);
}

void DirectMixerSource::FinalizeAudioPlayback() {
  source_->OnAudioPlaybackComplete();
  delete this;
}

}  // namespace media
}  // namespace chromecast
