// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_FUCHSIA_AUDIO_MANAGER_FUCHSIA_H_
#define MEDIA_AUDIO_FUCHSIA_AUDIO_MANAGER_FUCHSIA_H_

#include <media/audio.h>

#include "media/audio/audio_manager_base.h"

namespace media {

class AudioManagerFuchsia : public AudioManagerBase {
 public:
  AudioManagerFuchsia(std::unique_ptr<AudioThread> audio_thread,
                      AudioLogFactory* audio_log_factory);
  ~AudioManagerFuchsia() override;

  // Implementation of AudioManager.
  bool HasAudioOutputDevices() override;
  bool HasAudioInputDevices() override;
  void GetAudioInputDeviceNames(AudioDeviceNames* device_names) override;
  void GetAudioOutputDeviceNames(AudioDeviceNames* device_names) override;
  AudioParameters GetInputStreamParameters(
      const std::string& device_id) override;
  const char* GetName() override;

  // Implementation of AudioManagerBase.
  AudioOutputStream* MakeLinearOutputStream(
      const AudioParameters& params,
      const LogCallback& log_callback) override;
  AudioOutputStream* MakeLowLatencyOutputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLinearInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLowLatencyInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;

  fuchsia_audio_manager* GetFuchsiaAudioManager() const {
    return fuchsia_audio_manager_;
  }

 protected:
  AudioParameters GetPreferredOutputStreamParameters(
      const std::string& output_device_id,
      const AudioParameters& input_params) override;

 private:
  fuchsia_audio_manager* fuchsia_audio_manager_;

  DISALLOW_COPY_AND_ASSIGN(AudioManagerFuchsia);
};

}  // namespace media

#endif  // MEDIA_AUDIO_FUCHSIA_AUDIO_MANAGER_FUCHSIA_H_
