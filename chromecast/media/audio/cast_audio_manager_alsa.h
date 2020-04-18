// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_MANAGER_ALSA_H_
#define CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_MANAGER_ALSA_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "chromecast/media/audio/cast_audio_manager.h"

namespace media {
class AlsaWrapper;
}

namespace chromecast {

namespace media {

class CastAudioManagerAlsa : public CastAudioManager {
 public:
  CastAudioManagerAlsa(
      std::unique_ptr<::media::AudioThread> audio_thread,
      ::media::AudioLogFactory* audio_log_factory,
      std::unique_ptr<CmaBackendFactory> backend_factory,
      scoped_refptr<base::SingleThreadTaskRunner> backend_task_runner,
      bool use_mixer);
  ~CastAudioManagerAlsa() override;

  // CastAudioManager implementation.
  bool HasAudioInputDevices() override;
  void GetAudioInputDeviceNames(
      ::media::AudioDeviceNames* device_names) override;
  ::media::AudioParameters GetInputStreamParameters(
      const std::string& device_id) override;

 private:
  enum StreamType {
    kStreamPlayback = 0,
    kStreamCapture,
  };

  // CastAudioManager implementation.
  ::media::AudioInputStream* MakeLinearInputStream(
      const ::media::AudioParameters& params,
      const std::string& device_id,
      const ::media::AudioManager::LogCallback& log_callback) override;
  ::media::AudioInputStream* MakeLowLatencyInputStream(
      const ::media::AudioParameters& params,
      const std::string& device_id,
      const ::media::AudioManager::LogCallback& log_callback) override;

  ::media::AudioInputStream* MakeInputStream(
      const ::media::AudioParameters& params,
      const std::string& device_id);

  // Gets a list of available ALSA devices.
  void GetAlsaAudioDevices(StreamType type,
                           ::media::AudioDeviceNames* device_names);

  // Gets the ALSA devices' names and ids that support streams of the
  // given type.
  void GetAlsaDevicesInfo(StreamType type,
                          void** hint,
                          ::media::AudioDeviceNames* device_names);

  // Checks if the specific ALSA device is available.
  static bool IsAlsaDeviceAvailable(StreamType type, const char* device_name);

  static const char* UnwantedDeviceTypeWhenEnumerating(StreamType wanted_type);

  std::unique_ptr<::media::AlsaWrapper> wrapper_;

  DISALLOW_COPY_AND_ASSIGN(CastAudioManagerAlsa);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_MANAGER_ALSA_H_
