// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/alsa/alsa_util.h"

#include <stddef.h>

#include "base/logging.h"
#include "media/audio/alsa/alsa_wrapper.h"

namespace alsa_util {

static snd_pcm_t* OpenDevice(media::AlsaWrapper* wrapper,
                             const char* device_name,
                             snd_pcm_stream_t type,
                             int channels,
                             int sample_rate,
                             snd_pcm_format_t pcm_format,
                             int latency_us) {
  snd_pcm_t* handle = NULL;
  int error = wrapper->PcmOpen(&handle, device_name, type, SND_PCM_NONBLOCK);
  if (error < 0) {
    LOG(WARNING) << "PcmOpen: " << device_name << ","
                 << wrapper->StrError(error);
    return NULL;
  }

  error = wrapper->PcmSetParams(handle, pcm_format,
                                SND_PCM_ACCESS_RW_INTERLEAVED, channels,
                                sample_rate, 1, latency_us);
  if (error < 0) {
    LOG(WARNING) << "PcmSetParams: " << device_name << ", "
                 << wrapper->StrError(error) << " - Format: " << pcm_format
                 << " Channels: " << channels << " Latency: " << latency_us;
    if (alsa_util::CloseDevice(wrapper, handle) < 0) {
      // TODO(ajwong): Retry on certain errors?
      LOG(WARNING) << "Unable to close audio device. Leaking handle.";
    }
    return NULL;
  }

  return handle;
}

static std::string DeviceNameToControlName(const std::string& device_name) {
  const char kMixerPrefix[] = "hw";
  std::string control_name;
  size_t pos1 = device_name.find(':');
  if (pos1 == std::string::npos) {
    control_name = device_name;
  } else {
    // Examples:
    // deviceName: "front:CARD=Intel,DEV=0", controlName: "hw:CARD=Intel".
    // deviceName: "default:CARD=Intel", controlName: "CARD=Intel".
    size_t pos2 = device_name.find(',');
    control_name = (pos2 == std::string::npos)
                       ? device_name.substr(pos1 + 1)
                       : kMixerPrefix + device_name.substr(pos1, pos2 - pos1);
  }

  return control_name;
}

int CloseDevice(media::AlsaWrapper* wrapper, snd_pcm_t* handle) {
  std::string device_name = wrapper->PcmName(handle);
  int error = wrapper->PcmClose(handle);
  if (error < 0) {
    LOG(ERROR) << "PcmClose: " << device_name << ", "
               << wrapper->StrError(error);
  }

  return error;
}

snd_pcm_t* OpenCaptureDevice(media::AlsaWrapper* wrapper,
                             const char* device_name,
                             int channels,
                             int sample_rate,
                             snd_pcm_format_t pcm_format,
                             int latency_us) {
  return OpenDevice(wrapper, device_name, SND_PCM_STREAM_CAPTURE, channels,
                    sample_rate, pcm_format, latency_us);
}

snd_pcm_t* OpenPlaybackDevice(media::AlsaWrapper* wrapper,
                              const char* device_name,
                              int channels,
                              int sample_rate,
                              snd_pcm_format_t pcm_format,
                              int latency_us) {
  return OpenDevice(wrapper, device_name, SND_PCM_STREAM_PLAYBACK, channels,
                    sample_rate, pcm_format, latency_us);
}

snd_mixer_t* OpenMixer(media::AlsaWrapper* wrapper,
                       const std::string& device_name) {
  snd_mixer_t* mixer = NULL;

  int error = wrapper->MixerOpen(&mixer, 0);
  if (error < 0) {
    LOG(ERROR) << "MixerOpen: " << device_name << ", "
               << wrapper->StrError(error);
    return NULL;
  }

  std::string control_name = DeviceNameToControlName(device_name);
  error = wrapper->MixerAttach(mixer, control_name.c_str());
  if (error < 0) {
    LOG(ERROR) << "MixerAttach, " << control_name << ", "
               << wrapper->StrError(error);
    alsa_util::CloseMixer(wrapper, mixer, device_name);
    return NULL;
  }

  error = wrapper->MixerElementRegister(mixer, NULL, NULL);
  if (error < 0) {
    LOG(ERROR) << "MixerElementRegister: " << control_name << ", "
               << wrapper->StrError(error);
    alsa_util::CloseMixer(wrapper, mixer, device_name);
    return NULL;
  }

  return mixer;
}

void CloseMixer(media::AlsaWrapper* wrapper, snd_mixer_t* mixer,
                const std::string& device_name) {
  if (!mixer)
    return;

  wrapper->MixerFree(mixer);

  int error = 0;
  if (!device_name.empty()) {
    std::string control_name = DeviceNameToControlName(device_name);
    error = wrapper->MixerDetach(mixer, control_name.c_str());
    if (error < 0) {
      LOG(WARNING) << "MixerDetach: " << control_name << ", "
                   << wrapper->StrError(error);
    }
  }

  error = wrapper->MixerClose(mixer);
  if (error < 0) {
    LOG(WARNING) << "MixerClose: " << wrapper->StrError(error);
  }
}

snd_mixer_elem_t* LoadCaptureMixerElement(media::AlsaWrapper* wrapper,
                                          snd_mixer_t* mixer) {
  if (!mixer)
    return NULL;

  int error = wrapper->MixerLoad(mixer);
  if (error < 0) {
    LOG(ERROR) << "MixerLoad: " << wrapper->StrError(error);
    return NULL;
  }

  snd_mixer_elem_t* elem = NULL;
  snd_mixer_elem_t* mic_elem = NULL;
  const char kCaptureElemName[] = "Capture";
  const char kMicElemName[] = "Mic";
  for (elem = wrapper->MixerFirstElem(mixer);
       elem;
       elem = wrapper->MixerNextElem(elem)) {
    if (wrapper->MixerSelemIsActive(elem)) {
      const char* elem_name = wrapper->MixerSelemName(elem);
      if (strcmp(elem_name, kCaptureElemName) == 0)
        return elem;
      else if (strcmp(elem_name, kMicElemName) == 0)
        mic_elem = elem;
    }
  }

  // Did not find any Capture handle, use the Mic handle.
  return mic_elem;
}

}  // namespace alsa_util
