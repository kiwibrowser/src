// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_AUDIO_RENDERER_MIXER_POOL_H_
#define MEDIA_BASE_AUDIO_RENDERER_MIXER_POOL_H_

#include <string>

#include "media/base/audio_latency.h"
#include "media/base/output_device_info.h"

namespace media {
class AudioParameters;
class AudioRendererMixer;

// Provides AudioRendererMixer instances for shared usage.
// Thread safe.
class MEDIA_EXPORT AudioRendererMixerPool {
 public:
  AudioRendererMixerPool() {}
  virtual ~AudioRendererMixerPool() {}

  // Obtains a pointer to mixer instance based on AudioParameters. The pointer
  // is guaranteed to be valid (at least) until it's rereleased by a call to
  // ReturnMixer().
  virtual AudioRendererMixer* GetMixer(int owner_id,
                                       const AudioParameters& params,
                                       AudioLatency::LatencyType latency,
                                       const std::string& device_id,
                                       OutputDeviceStatus* device_status) = 0;

  // Returns mixer back to the pool, must be called when the mixer is not needed
  // any more to avoid memory leakage.
  virtual void ReturnMixer(AudioRendererMixer* mixer) = 0;

  // Returns output device information
  virtual OutputDeviceInfo GetOutputDeviceInfo(
      int owner_id,
      int session_id,
      const std::string& device_id) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioRendererMixerPool);
};

}  // namespace media

#endif  // MEDIA_BASE_AUDIO_RENDERER_MIXER_POOL_H_
