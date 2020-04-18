// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/audio/cast_audio_manager.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "chromecast/media/audio/cast_audio_mixer.h"
#include "chromecast/media/audio/cast_audio_output_stream.h"
#include "chromecast/media/cma/backend/cma_backend_factory.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace {
// TODO(alokp): Query the preferred value from media backend.
const int kDefaultSampleRate = 48000;

// Define bounds for the output buffer size (in frames).
// Note: These values are copied from AudioManagerPulse implementation.
// TODO(alokp): Query the preferred value from media backend.
static const int kMinimumOutputBufferSize = 512;
static const int kMaximumOutputBufferSize = 8192;
static const int kDefaultOutputBufferSize = 2048;

// TODO(jyw): Query the preferred value from media backend.
static const int kDefaultInputBufferSize = 1024;

}  // namespace

namespace chromecast {
namespace media {

CastAudioManager::CastAudioManager(
    std::unique_ptr<::media::AudioThread> audio_thread,
    ::media::AudioLogFactory* audio_log_factory,
    std::unique_ptr<CmaBackendFactory> backend_factory,
    scoped_refptr<base::SingleThreadTaskRunner> backend_task_runner,
    bool use_mixer)
    : AudioManagerBase(std::move(audio_thread), audio_log_factory),
      backend_factory_(std::move(backend_factory)),
      backend_task_runner_(std::move(backend_task_runner)) {
  if (use_mixer)
    mixer_ = std::make_unique<CastAudioMixer>(this);
}

CastAudioManager::~CastAudioManager() = default;

bool CastAudioManager::HasAudioOutputDevices() {
  return true;
}

bool CastAudioManager::HasAudioInputDevices() {
  return false;
}

void CastAudioManager::GetAudioInputDeviceNames(
    ::media::AudioDeviceNames* device_names) {
  DCHECK(device_names->empty());
  LOG(WARNING) << "No support for input audio devices";
}

::media::AudioParameters CastAudioManager::GetInputStreamParameters(
    const std::string& device_id) {
  LOG(WARNING) << "No support for input audio devices";
  // Need to send a valid AudioParameters object even when it will be unused.
  return ::media::AudioParameters(
      ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
      ::media::CHANNEL_LAYOUT_STEREO, kDefaultSampleRate,
      kDefaultInputBufferSize);
}

const char* CastAudioManager::GetName() {
  return "Cast";
}

void CastAudioManager::ReleaseOutputStream(::media::AudioOutputStream* stream) {
  // If |stream| is |mixer_output_stream_|, we should not use
  // AudioManagerBase::ReleaseOutputStream as we do not want the release
  // function to decrement |AudioManagerBase::num_output_streams_|. This is
  // because the stream generated from MakeMixerOutputStream was not created
  // using AudioManagerBase::MakeAudioOutputStream, which appropriately
  // increments this variable.
  if (mixer_output_stream_.get() == stream) {
    DCHECK(mixer_);  // Should only occur if |mixer_| exists
    mixer_output_stream_.reset();
  } else {
    AudioManagerBase::ReleaseOutputStream(stream);
  }
}

::media::AudioOutputStream* CastAudioManager::MakeLinearOutputStream(
    const ::media::AudioParameters& params,
    const ::media::AudioManager::LogCallback& log_callback) {
  DCHECK_EQ(::media::AudioParameters::AUDIO_PCM_LINEAR, params.format());

  // If |mixer_| exists, return a mixing stream.
  if (mixer_)
    return mixer_->MakeStream(params);
  else
    return new CastAudioOutputStream(params, this);
}

::media::AudioOutputStream* CastAudioManager::MakeLowLatencyOutputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id,
    const ::media::AudioManager::LogCallback& log_callback) {
  DCHECK_EQ(::media::AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());

  // If |mixer_| exists, return a mixing stream.
  if (mixer_)
    return mixer_->MakeStream(params);
  else
    return new CastAudioOutputStream(params, this);
}

::media::AudioInputStream* CastAudioManager::MakeLinearInputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id,
    const ::media::AudioManager::LogCallback& log_callback) {
  LOG(WARNING) << "No support for input audio devices";
  return nullptr;
}

::media::AudioInputStream* CastAudioManager::MakeLowLatencyInputStream(
    const ::media::AudioParameters& params,
    const std::string& device_id,
    const ::media::AudioManager::LogCallback& log_callback) {
  LOG(WARNING) << "No support for input audio devices";
  return nullptr;
}

::media::AudioParameters CastAudioManager::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const ::media::AudioParameters& input_params) {
  ::media::ChannelLayout channel_layout = ::media::CHANNEL_LAYOUT_STEREO;
  int sample_rate = kDefaultSampleRate;
  int buffer_size = kDefaultOutputBufferSize;
  if (input_params.IsValid()) {
    // Do not change:
    // - the channel layout
    // - the number of bits per sample
    // We support stereo only with 16 bits per sample.
    sample_rate = input_params.sample_rate();
    buffer_size = std::min(
        kMaximumOutputBufferSize,
        std::max(kMinimumOutputBufferSize, input_params.frames_per_buffer()));
  }

  ::media::AudioParameters output_params(
      ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
      sample_rate, buffer_size);
  return output_params;
}

::media::AudioOutputStream* CastAudioManager::MakeMixerOutputStream(
    const ::media::AudioParameters& params) {
  DCHECK(mixer_);
  DCHECK(!mixer_output_stream_);  // Only allow 1 |mixer_output_stream_|.

  // Keep a reference to this stream for proper behavior on
  // CastAudioManager::ReleaseOutputStream.
  mixer_output_stream_.reset(new CastAudioOutputStream(params, this));
  return mixer_output_stream_.get();
}

}  // namespace media
}  // namespace chromecast
