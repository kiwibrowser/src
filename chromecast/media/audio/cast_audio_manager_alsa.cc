// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/audio/cast_audio_manager_alsa.h"

#include <string>
#include <utility>

#include "base/memory/free_deleter.h"
#include "chromecast/media/cma/backend/cma_backend_factory.h"
#include "media/audio/alsa/alsa_input.h"
#include "media/audio/alsa/alsa_wrapper.h"

namespace chromecast {
namespace media {

namespace {
// TODO(alokp): Query the preferred value from media backend.
const int kDefaultSampleRate = 48000;

// TODO(jyw): Query the preferred value from media backend.
static const int kDefaultInputBufferSize = 1024;

// Since "default" and "dmix" devices are virtual devices mapped to real
// devices, we remove them from the list to avoiding duplicate counting.
static const char* kInvalidAudioInputDevices[] = {
    "default", "dmix", "null",
};

}  // namespace

CastAudioManagerAlsa::CastAudioManagerAlsa(
    std::unique_ptr<::media::AudioThread> audio_thread,
    ::media::AudioLogFactory* audio_log_factory,
    std::unique_ptr<CmaBackendFactory> backend_factory,
    scoped_refptr<base::SingleThreadTaskRunner> backend_task_runner,
    bool use_mixer)
    : CastAudioManager(std::move(audio_thread),
                       audio_log_factory,
                       std::move(backend_factory),
                       backend_task_runner,
                       use_mixer),
      wrapper_(new ::media::AlsaWrapper()) {}

CastAudioManagerAlsa::~CastAudioManagerAlsa() {}

bool CastAudioManagerAlsa::HasAudioInputDevices() {
  return true;
}

void CastAudioManagerAlsa::GetAudioInputDeviceNames(
    ::media::AudioDeviceNames* device_names) {
  DCHECK(device_names->empty());
  GetAlsaAudioDevices(kStreamCapture, device_names);
}

::media::AudioParameters CastAudioManagerAlsa::GetInputStreamParameters(
    const std::string& device_id) {
  // TODO(jyw): Be smarter about sample rate instead of hardcoding it.
  // Need to send a valid AudioParameters object even when it will be unused.
  return ::media::AudioParameters(
      ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
      ::media::CHANNEL_LAYOUT_STEREO, kDefaultSampleRate,
      kDefaultInputBufferSize);
}

::media::AudioInputStream* CastAudioManagerAlsa::MakeLinearInputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id,
    const ::media::AudioManager::LogCallback& log_callback) {
  DCHECK_EQ(::media::AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return MakeInputStream(params, device_id);
}

::media::AudioInputStream* CastAudioManagerAlsa::MakeLowLatencyInputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id,
    const ::media::AudioManager::LogCallback& log_callback) {
  DCHECK_EQ(::media::AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  return MakeInputStream(params, device_id);
}

::media::AudioInputStream* CastAudioManagerAlsa::MakeInputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id) {
  std::string device_name =
      (device_id == ::media::AudioDeviceDescription::kDefaultDeviceId)
          ? ::media::AlsaPcmInputStream::kAutoSelectDevice
          : device_id;
  return new ::media::AlsaPcmInputStream(this, device_name, params,
                                         wrapper_.get());
}

void CastAudioManagerAlsa::GetAlsaAudioDevices(
    StreamType type,
    ::media::AudioDeviceNames* device_names) {
  // Constants specified by the ALSA API for device hints.
  static const char kPcmInterfaceName[] = "pcm";
  int card = -1;

  // Loop through the sound cards to get ALSA device hints.
  while (!wrapper_->CardNext(&card) && card >= 0) {
    void** hints = NULL;
    int error = wrapper_->DeviceNameHint(card, kPcmInterfaceName, &hints);
    if (!error) {
      GetAlsaDevicesInfo(type, hints, device_names);

      // Destroy the hints now that we're done with it.
      wrapper_->DeviceNameFreeHint(hints);
    } else {
      DLOG(WARNING) << "GetAlsaAudioDevices: unable to get device hints: "
                    << wrapper_->StrError(error);
    }
  }
}

void CastAudioManagerAlsa::GetAlsaDevicesInfo(
    StreamType type,
    void** hints,
    ::media::AudioDeviceNames* device_names) {
  static const char kIoHintName[] = "IOID";
  static const char kNameHintName[] = "NAME";
  static const char kDescriptionHintName[] = "DESC";

  const char* unwanted_device_type = UnwantedDeviceTypeWhenEnumerating(type);

  for (void** hint_iter = hints; *hint_iter != NULL; hint_iter++) {
    // Only examine devices of the right type.  Valid values are
    // "Input", "Output", and NULL which means both input and output.
    std::unique_ptr<char, base::FreeDeleter> io(
        wrapper_->DeviceNameGetHint(*hint_iter, kIoHintName));
    if (io != NULL && strcmp(unwanted_device_type, io.get()) == 0)
      continue;

    // Found a device, prepend the default device since we always want
    // it to be on the top of the list for all platforms. And there is
    // no duplicate counting here since it is only done if the list is
    // still empty.  Note, pulse has exclusively opened the default
    // device, so we must open the device via the "default" moniker.
    if (device_names->empty())
      device_names->push_front(::media::AudioDeviceName::CreateDefault());

    // Get the unique device name for the device.
    std::unique_ptr<char, base::FreeDeleter> unique_device_name(
        wrapper_->DeviceNameGetHint(*hint_iter, kNameHintName));

    // Find out if the device is available.
    if (IsAlsaDeviceAvailable(type, unique_device_name.get())) {
      // Get the description for the device.
      std::unique_ptr<char, base::FreeDeleter> desc(
          wrapper_->DeviceNameGetHint(*hint_iter, kDescriptionHintName));

      ::media::AudioDeviceName name;
      name.unique_id = unique_device_name.get();
      if (desc) {
        // Use the more user friendly description as name.
        // Replace '\n' with '-'.
        char* pret = strchr(desc.get(), '\n');
        if (pret)
          *pret = '-';
        name.device_name = desc.get();
      } else {
        // Virtual devices don't necessarily have descriptions.
        // Use their names instead.
        name.device_name = unique_device_name.get();
      }

      // Store the device information.
      device_names->push_back(name);
    }
  }
}

// static
bool CastAudioManagerAlsa::IsAlsaDeviceAvailable(StreamType type,
                                                 const char* device_name) {
  if (!device_name)
    return false;

  // We do prefix matches on the device name to see whether to include
  // it or not.
  if (type == kStreamCapture) {
    // Check if the device is in the list of invalid devices.
    for (size_t i = 0; i < arraysize(kInvalidAudioInputDevices); ++i) {
      if (strncmp(kInvalidAudioInputDevices[i], device_name,
                  strlen(kInvalidAudioInputDevices[i])) == 0)
        return false;
    }
    return true;
  } else {
    DCHECK_EQ(kStreamPlayback, type);
    // We prefer the device type that maps straight to hardware but
    // goes through software conversion if needed (e.g. incompatible
    // sample rate).
    // TODO(joi): Should we prefer "hw" instead?
    static const char kDeviceTypeDesired[] = "plughw";
    return strncmp(kDeviceTypeDesired, device_name,
                   arraysize(kDeviceTypeDesired) - 1) == 0;
  }
}

// static
const char* CastAudioManagerAlsa::UnwantedDeviceTypeWhenEnumerating(
    StreamType wanted_type) {
  return wanted_type == kStreamPlayback ? "Input" : "Output";
}

}  // namespace media
}  // namespace chromecast
