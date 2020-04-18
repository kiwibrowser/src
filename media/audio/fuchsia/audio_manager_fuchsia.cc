// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/fuchsia/audio_manager_fuchsia.h"

#include <memory>

#include <media/audio.h>

#include "media/audio/fuchsia/audio_output_stream_fuchsia.h"

namespace media {

AudioManagerFuchsia::AudioManagerFuchsia(
    std::unique_ptr<AudioThread> audio_thread,
    AudioLogFactory* audio_log_factory)
    : AudioManagerBase(std::move(audio_thread), audio_log_factory),
      fuchsia_audio_manager_(fuchsia_audio_manager_create()) {}

AudioManagerFuchsia::~AudioManagerFuchsia() {
  fuchsia_audio_manager_free(fuchsia_audio_manager_);
}

bool AudioManagerFuchsia::HasAudioOutputDevices() {
  return fuchsia_audio_manager_get_output_devices(fuchsia_audio_manager_,
                                                  nullptr, 0) > 0;
}

bool AudioManagerFuchsia::HasAudioInputDevices() {
  NOTIMPLEMENTED();
  return false;
}

void AudioManagerFuchsia::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  device_names->clear();
  NOTIMPLEMENTED();
}

void AudioManagerFuchsia::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  device_names->clear();

  std::vector<fuchsia_audio_device_description> descriptions;
  descriptions.resize(16);
  bool try_again = true;
  while (try_again) {
    int result = fuchsia_audio_manager_get_output_devices(
        fuchsia_audio_manager_, descriptions.data(), descriptions.size());
    if (result < 0) {
      LOG(ERROR) << "fuchsia_audio_manager_get_output_devices() returned "
                 << result;
      device_names->clear();
      return;
    }

    // Try again if the buffer was too small.
    try_again = static_cast<size_t>(result) > descriptions.size();
    descriptions.resize(result);
  }

  // Create default device if we have any output devices present.
  if (!descriptions.empty())
    device_names->push_back(AudioDeviceName::CreateDefault());

  for (auto& desc : descriptions) {
    device_names->push_back(AudioDeviceName(desc.name, desc.id));
  }
}

AudioParameters AudioManagerFuchsia::GetInputStreamParameters(
    const std::string& device_id) {
  NOTREACHED();
  return AudioParameters();
}

AudioParameters AudioManagerFuchsia::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  fuchsia_audio_parameters device_params;
  int result = fuchsia_audio_manager_get_output_device_default_parameters(
      fuchsia_audio_manager_,
      output_device_id == AudioDeviceDescription::kDefaultDeviceId
          ? nullptr
          : const_cast<char*>(output_device_id.c_str()),
      &device_params);
  if (result < 0) {
    LOG(ERROR) << "fuchsia_audio_manager_get_default_output_device_parameters()"
                  " returned "
               << result;

    return AudioParameters();
  }

  int user_buffer_size = GetUserBufferSize();
  if (user_buffer_size > 0)
    device_params.buffer_size = user_buffer_size;

  int sample_rate = input_params.sample_rate();
  if (sample_rate < 8000 || sample_rate > 96000)
    sample_rate = device_params.sample_rate;

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                         GuessChannelLayout(device_params.num_channels),
                         sample_rate, device_params.buffer_size);
}

const char* AudioManagerFuchsia::GetName() {
  return "Fuchsia";
}

AudioOutputStream* AudioManagerFuchsia::MakeLinearOutputStream(
    const AudioParameters& params,
    const LogCallback& log_callback) {
  NOTREACHED();
  return nullptr;
}

AudioOutputStream* AudioManagerFuchsia::MakeLowLatencyOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  return new AudioOutputStreamFuchsia(this, device_id, params);
}

AudioInputStream* AudioManagerFuchsia::MakeLinearInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  NOTREACHED();
  return nullptr;
}

AudioInputStream* AudioManagerFuchsia::MakeLowLatencyInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  NOTREACHED();
  return nullptr;
}

std::unique_ptr<AudioManager> CreateAudioManager(
    std::unique_ptr<AudioThread> audio_thread,
    AudioLogFactory* audio_log_factory) {
  return std::make_unique<AudioManagerFuchsia>(std::move(audio_thread),
                                               audio_log_factory);
}

}  // namespace media
