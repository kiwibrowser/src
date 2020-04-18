// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_audio_player.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util.h"

// The frame size we will request from the browser.
const int kFrameSizeMs = 40;

namespace remoting {

PepperAudioPlayer::PepperAudioPlayer(pp::Instance* instance)
    : instance_(instance), samples_per_frame_(0), weak_factory_(this) {}

PepperAudioPlayer::~PepperAudioPlayer() {
}

uint32_t PepperAudioPlayer::GetSamplesPerFrame() {
  return samples_per_frame_;
}

base::WeakPtr<PepperAudioPlayer> PepperAudioPlayer::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool PepperAudioPlayer::ResetAudioPlayer(
      AudioPacket::SamplingRate sampling_rate) {
  PP_AudioSampleRate pp_sampling_rate = PP_AUDIOSAMPLERATE_NONE;
  switch (sampling_rate) {
    case AudioPacket::SAMPLING_RATE_44100:
      pp_sampling_rate = PP_AUDIOSAMPLERATE_44100;
      break;
    case AudioPacket::SAMPLING_RATE_48000:
      pp_sampling_rate = PP_AUDIOSAMPLERATE_48000;
      break;
    default:
      LOG(ERROR) << "Unsupported audio sampling rate: " << sampling_rate;
      return false;
  }

  // Ask the browser/device for an appropriate frame size.
  samples_per_frame_ = pp::AudioConfig::RecommendSampleFrameCount(
      instance_, pp_sampling_rate,
      kFrameSizeMs * sampling_rate / base::Time::kMillisecondsPerSecond);

  // Create an audio configuration resource.
  pp::AudioConfig audio_config = pp::AudioConfig(
      instance_, pp_sampling_rate, samples_per_frame_);

  // Create an audio resource.
  audio_ = pp::Audio(instance_, audio_config, AudioPlayerCallback, this);

  // Immediately start the player.
  bool success = audio_.StartPlayback();
  if (!success)
    LOG(ERROR) << "Failed to start Pepper audio player";
  return success;
}

}  // namespace remoting
