// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/test/fake_group_member.h"

#include <algorithm>
#include <cmath>

#include "base/numerics/math_constants.h"
#include "media/base/audio_bus.h"

namespace audio {

FakeGroupMember::FakeGroupMember(const base::UnguessableToken& group_id,
                                 const media::AudioParameters& params)
    : group_id_(group_id),
      params_(params),
      audio_bus_(media::AudioBus::Create(params_)),
      frequency_by_channel_(params_.channels(), 0.0) {
  CHECK(params_.IsValid());
}

FakeGroupMember::~FakeGroupMember() = default;

void FakeGroupMember::SetChannelTone(int ch, double frequency) {
  frequency_by_channel_[ch] = frequency;
}

void FakeGroupMember::SetVolume(double volume) {
  CHECK_GE(volume, 0.0);
  CHECK_LE(volume, 1.0);
  volume_ = volume;
}

void FakeGroupMember::RenderMoreAudio(base::TimeTicks output_timestamp) {
  if (snooper_) {
    for (int ch = 0; ch < params_.channels(); ++ch) {
      const double step = 2.0 * base::kPiDouble * frequency_by_channel_[ch] /
                          params_.sample_rate();
      float* const samples = audio_bus_->channel(ch);
      for (int frame = 0; frame < params_.frames_per_buffer(); ++frame) {
        samples[frame] = std::sin((at_frame_ + frame) * step);
      }
    }
    snooper_->OnData(*audio_bus_, output_timestamp, volume_);
  }
  at_frame_ += params_.frames_per_buffer();
}

const base::UnguessableToken& FakeGroupMember::GetGroupId() {
  return group_id_;
}

const media::AudioParameters& FakeGroupMember::GetAudioParameters() {
  return params_;
}

void FakeGroupMember::StartSnooping(Snooper* snooper) {
  CHECK(!snooper_);
  snooper_ = snooper;
}

void FakeGroupMember::StopSnooping(Snooper* snooper) {
  snooper_ = nullptr;
}

void FakeGroupMember::StartMuting() {
  // No effect for this fake implementation.
}

void FakeGroupMember::StopMuting() {
  // No effect for this fake implementation.
}

}  // namespace audio
