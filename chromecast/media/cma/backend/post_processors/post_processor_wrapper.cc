// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processors/post_processor_wrapper.h"

#include "base/logging.h"
#include "chromecast/public/media/audio_post_processor_shlib.h"

namespace chromecast {
namespace media {

AudioPostProcessorWrapper::AudioPostProcessorWrapper(
    std::unique_ptr<AudioPostProcessor> pp,
    int channels)
    : owned_pp_(std::move(pp)), pp_(owned_pp_.get()), channels_(channels) {
  DCHECK(owned_pp_);
}

AudioPostProcessorWrapper::AudioPostProcessorWrapper(AudioPostProcessor* pp,
                                                     int channels)
    : pp_(pp), channels_(channels) {
  DCHECK(pp_);
}

AudioPostProcessorWrapper::~AudioPostProcessorWrapper() = default;

bool AudioPostProcessorWrapper::SetSampleRate(int sample_rate) {
  return pp_->SetSampleRate(sample_rate);
}

int AudioPostProcessorWrapper::ProcessFrames(float* data,
                                             int frames,
                                             float system_volume,
                                             float volume_dbfs) {
  output_buffer_ = data;
  return pp_->ProcessFrames(data, frames, system_volume, volume_dbfs);
}

float* AudioPostProcessorWrapper::GetOutputBuffer() {
  return output_buffer_;
}

int AudioPostProcessorWrapper::GetRingingTimeInFrames() {
  return pp_->GetRingingTimeInFrames();
}

bool AudioPostProcessorWrapper::UpdateParameters(const std::string& message) {
  pp_->UpdateParameters(message);
  return true;
}

void AudioPostProcessorWrapper::SetContentType(AudioContentType content_type) {
  pp_->SetContentType(content_type);
}

void AudioPostProcessorWrapper::SetPlayoutChannel(int channel) {
  pp_->SetPlayoutChannel(channel);
}

int AudioPostProcessorWrapper::NumOutputChannels() {
  return channels_;
}

}  // namespace media
}  // namespace chromecast
