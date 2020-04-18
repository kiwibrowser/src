// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_ALSA_ALSA_UTIL_H_
#define MEDIA_AUDIO_ALSA_ALSA_UTIL_H_

#include <alsa/asoundlib.h>
#include <string>

namespace media {
class AlsaWrapper;
}

namespace alsa_util {

snd_pcm_t* OpenCaptureDevice(media::AlsaWrapper* wrapper,
                             const char* device_name,
                             int channels,
                             int sample_rate,
                             snd_pcm_format_t pcm_format,
                             int latency_us);

snd_pcm_t* OpenPlaybackDevice(media::AlsaWrapper* wrapper,
                              const char* device_name,
                              int channels,
                              int sample_rate,
                              snd_pcm_format_t pcm_format,
                              int latency_us);

int CloseDevice(media::AlsaWrapper* wrapper, snd_pcm_t* handle);

snd_mixer_t* OpenMixer(media::AlsaWrapper* wrapper,
                       const std::string& device_name);

void CloseMixer(media::AlsaWrapper* wrapper,
                snd_mixer_t* mixer,
                const std::string& device_name);

snd_mixer_elem_t* LoadCaptureMixerElement(media::AlsaWrapper* wrapper,
                                          snd_mixer_t* mixer);

}  // namespace alsa_util

#endif  // MEDIA_AUDIO_ALSA_ALSA_UTIL_H_
