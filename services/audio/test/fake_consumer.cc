// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/test/fake_consumer.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/numerics/math_constants.h"
#include "media/base/audio_bus.h"

namespace audio {

FakeConsumer::FakeConsumer(int channels, int sample_rate)
    : sample_rate_(sample_rate) {
  recorded_channel_data_.resize(channels);
}

FakeConsumer::~FakeConsumer() = default;

int FakeConsumer::GetRecordedFrameCount() const {
  return static_cast<int>(recorded_channel_data_[0].size());
}

void FakeConsumer::Clear() {
  for (auto& data : recorded_channel_data_) {
    data.clear();
  }
}

void FakeConsumer::Consume(const media::AudioBus& bus) {
  CHECK_EQ(static_cast<int>(recorded_channel_data_.size()), bus.channels());
  for (int ch = 0; ch < static_cast<int>(recorded_channel_data_.size()); ++ch) {
    const float* const src = bus.channel(ch);
    std::vector<float>& samples = recorded_channel_data_[ch];
    samples.insert(samples.end(), src, src + bus.frames());
  }
}

bool FakeConsumer::IsSilent(int channel) const {
  return IsSilentInRange(channel, 0, GetRecordedFrameCount());
}

bool FakeConsumer::IsSilentInRange(int channel,
                                   int begin_frame,
                                   int end_frame) const {
  CHECK_LT(channel, static_cast<int>(recorded_channel_data_.size()));
  const std::vector<float>& samples = recorded_channel_data_[channel];

  CHECK_GE(begin_frame, 0);
  CHECK_LE(begin_frame, end_frame);
  CHECK_LE(end_frame, static_cast<int>(samples.size()));

  if (begin_frame == end_frame) {
    return true;
  }
  const float value = samples[begin_frame];
  return std::all_of(samples.data() + begin_frame + 1,
                     samples.data() + end_frame,
                     [&value](float f) { return f == value; });
}

int FakeConsumer::FindEndOfSilence(int channel, int begin_frame) const {
  CHECK_LT(channel, static_cast<int>(recorded_channel_data_.size()));
  CHECK_GE(begin_frame, 0);
  const std::vector<float>& samples = recorded_channel_data_[channel];

  if (static_cast<int>(samples.size()) <= begin_frame) {
    return begin_frame;
  }
  const float value = samples[begin_frame];
  const float* at = std::find_if(samples.data() + begin_frame + 1,
                                 samples.data() + GetRecordedFrameCount(),
                                 [&value](float f) { return f != value; });
  return at - samples.data();
}

double FakeConsumer::ComputeAmplitudeAt(int channel,
                                        double frequency,
                                        int end_frame) const {
  CHECK_LT(channel, static_cast<int>(recorded_channel_data_.size()));
  CHECK_GT(frequency, 0.0);
  const std::vector<float>& samples = recorded_channel_data_[channel];
  CHECK_LE(end_frame, static_cast<int>(samples.size()));

  // Attempt to analyze the last three cycles of waveform, or less if the
  // recording is shorter than that. Three is chosen here because this will
  // allow the algorithm below to reliably compute an amplitude value that is
  // very close to that which was used to generate the pure source signal, even
  // if the implementation under test has slightly stretched/compressed the
  // signal (e.g., +/- 1 Hz).
  const int analysis_length =
      std::min(end_frame, static_cast<int>(3 * sample_rate_ / frequency));
  if (analysis_length == 0) {
    return 0.0;
  }

  // Compute the amplitude for just the |frequency| of interest, as opposed to
  // doing a full Discrete Fourier Transform.
  const double step = 2.0 * base::kPiDouble * frequency / sample_rate_;
  double real_part = 0.0;
  double img_part = 0.0;
  for (int i = end_frame - analysis_length; i < end_frame; ++i) {
    real_part += samples[i] * std::cos(i * step);
    img_part -= samples[i] * std::sin(i * step);
  }
  const double normalization_factor = 2.0 / analysis_length;
  return std::sqrt(real_part * real_part + img_part * img_part) *
         normalization_factor;
}

}  // namespace audio
