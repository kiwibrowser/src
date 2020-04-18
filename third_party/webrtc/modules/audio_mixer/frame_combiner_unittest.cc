/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_mixer/frame_combiner.h"

#include <numeric>
#include <sstream>
#include <string>

#include "audio/utility/audio_frame_operations.h"
#include "modules/audio_mixer/gain_change_calculator.h"
#include "modules/audio_mixer/sine_wave_generator.h"
#include "rtc_base/checks.h"
#include "test/gtest.h"

namespace webrtc {

namespace {
using LimiterType = FrameCombiner::LimiterType;
using NativeRate = AudioProcessing::NativeRate;
struct FrameCombinerConfig {
  LimiterType limiter_type;
  NativeRate sample_rate_hz;
  int number_of_channels;
  float wave_frequency;
};

std::string ProduceDebugText(int sample_rate_hz,
                             int number_of_channels,
                             int number_of_sources) {
  std::ostringstream ss;
  ss << "Sample rate: " << sample_rate_hz << " ,";
  ss << "number of channels: " << number_of_channels << " ,";
  ss << "number of sources: " << number_of_sources;
  return ss.str();
}

std::string ProduceDebugText(const FrameCombinerConfig& config) {
  std::ostringstream ss;
  ss << "Sample rate: " << config.sample_rate_hz << " ,";
  ss << "number of channels: " << config.number_of_channels << " ,";
  ss << "limiter active: "
     << (config.limiter_type == LimiterType::kNoLimiter
             ? "off"

             : (config.limiter_type == LimiterType::kApmAgcLimiter ? "agc1"
                                                                   : "agc2"))
     << " ,";
  ss << "wave frequency: " << config.wave_frequency << " ,";
  return ss.str();
}

AudioFrame frame1;
AudioFrame frame2;
AudioFrame audio_frame_for_mixing;

void SetUpFrames(int sample_rate_hz, int number_of_channels) {
  for (auto* frame : {&frame1, &frame2}) {
    frame->UpdateFrame(0, nullptr, rtc::CheckedDivExact(sample_rate_hz, 100),
                       sample_rate_hz, AudioFrame::kNormalSpeech,
                       AudioFrame::kVadActive, number_of_channels);
  }
}
}  // namespace

TEST(FrameCombiner, BasicApiCallsLimiter) {
  FrameCombiner combiner(LimiterType::kApmAgcLimiter);
  for (const int rate : {8000, 16000, 32000, 48000}) {
    for (const int number_of_channels : {1, 2}) {
      const std::vector<AudioFrame*> all_frames = {&frame1, &frame2};
      SetUpFrames(rate, number_of_channels);

      for (const int number_of_frames : {0, 1, 2}) {
        SCOPED_TRACE(
            ProduceDebugText(rate, number_of_channels, number_of_frames));
        const std::vector<AudioFrame*> frames_to_combine(
            all_frames.begin(), all_frames.begin() + number_of_frames);
        combiner.Combine(frames_to_combine, number_of_channels, rate,
                         frames_to_combine.size(), &audio_frame_for_mixing);
      }
    }
  }
}

// No APM limiter means no AudioProcessing::NativeRate restriction
// on rate. The rate has to be divisible by 100 since we use
// 10 ms frames, though.
TEST(FrameCombiner, BasicApiCallsNoLimiter) {
  FrameCombiner combiner(LimiterType::kNoLimiter);
  for (const int rate : {8000, 10000, 11000, 32000, 44100}) {
    for (const int number_of_channels : {1, 2}) {
      const std::vector<AudioFrame*> all_frames = {&frame1, &frame2};
      SetUpFrames(rate, number_of_channels);

      for (const int number_of_frames : {0, 1, 2}) {
        SCOPED_TRACE(
            ProduceDebugText(rate, number_of_channels, number_of_frames));
        const std::vector<AudioFrame*> frames_to_combine(
            all_frames.begin(), all_frames.begin() + number_of_frames);
        combiner.Combine(frames_to_combine, number_of_channels, rate,
                         frames_to_combine.size(), &audio_frame_for_mixing);
      }
    }
  }
}

TEST(FrameCombiner, CombiningZeroFramesShouldProduceSilence) {
  FrameCombiner combiner(LimiterType::kNoLimiter);
  for (const int rate : {8000, 10000, 11000, 32000, 44100}) {
    for (const int number_of_channels : {1, 2}) {
      SCOPED_TRACE(ProduceDebugText(rate, number_of_channels, 0));

      const std::vector<AudioFrame*> frames_to_combine;
      combiner.Combine(frames_to_combine, number_of_channels, rate,
                       frames_to_combine.size(), &audio_frame_for_mixing);

      const int16_t* audio_frame_for_mixing_data =
          audio_frame_for_mixing.data();
      const std::vector<int16_t> mixed_data(
          audio_frame_for_mixing_data,
          audio_frame_for_mixing_data + number_of_channels * rate / 100);

      const std::vector<int16_t> expected(number_of_channels * rate / 100, 0);
      EXPECT_EQ(mixed_data, expected);
    }
  }
}

TEST(FrameCombiner, CombiningOneFrameShouldNotChangeFrame) {
  FrameCombiner combiner(LimiterType::kNoLimiter);
  for (const int rate : {8000, 10000, 11000, 32000, 44100}) {
    for (const int number_of_channels : {1, 2}) {
      SCOPED_TRACE(ProduceDebugText(rate, number_of_channels, 1));

      SetUpFrames(rate, number_of_channels);
      int16_t* frame1_data = frame1.mutable_data();
      std::iota(frame1_data, frame1_data + number_of_channels * rate / 100, 0);
      const std::vector<AudioFrame*> frames_to_combine = {&frame1};
      combiner.Combine(frames_to_combine, number_of_channels, rate,
                       frames_to_combine.size(), &audio_frame_for_mixing);

      const int16_t* audio_frame_for_mixing_data =
          audio_frame_for_mixing.data();
      const std::vector<int16_t> mixed_data(
          audio_frame_for_mixing_data,
          audio_frame_for_mixing_data + number_of_channels * rate / 100);

      std::vector<int16_t> expected(number_of_channels * rate / 100);
      std::iota(expected.begin(), expected.end(), 0);
      EXPECT_EQ(mixed_data, expected);
    }
  }
}

// Send a sine wave through the FrameCombiner, and check that the
// difference between input and output varies smoothly. Also check
// that it is inside reasonable bounds. This is to catch issues like
// chromium:695993 and chromium:816875.
TEST(FrameCombiner, GainCurveIsSmoothForAlternatingNumberOfStreams) {
  std::vector<FrameCombinerConfig> configs = {
      {LimiterType::kNoLimiter, NativeRate::kSampleRate32kHz, 2, 50.f},
      {LimiterType::kNoLimiter, NativeRate::kSampleRate16kHz, 1, 3200.f},
      {LimiterType::kApmAgcLimiter, NativeRate::kSampleRate8kHz, 1, 3200.f},
      {LimiterType::kApmAgcLimiter, NativeRate::kSampleRate16kHz, 1, 50.f},
      {LimiterType::kApmAgcLimiter, NativeRate::kSampleRate16kHz, 2, 3200.f},
      {LimiterType::kApmAgcLimiter, NativeRate::kSampleRate8kHz, 2, 50.f},
      {LimiterType::kApmAgc2Limiter, NativeRate::kSampleRate8kHz, 1, 3200.f},
      {LimiterType::kApmAgc2Limiter, NativeRate::kSampleRate32kHz, 1, 50.f},
      {LimiterType::kApmAgc2Limiter, NativeRate::kSampleRate48kHz, 2, 3200.f},
  };

  for (const auto& config : configs) {
    SCOPED_TRACE(ProduceDebugText(config));

    FrameCombiner combiner(config.limiter_type);

    constexpr int16_t wave_amplitude = 30000;
    SineWaveGenerator wave_generator(config.wave_frequency, wave_amplitude);

    GainChangeCalculator change_calculator;
    float cumulative_change = 0.f;

    constexpr size_t iterations = 100;

    for (size_t i = 0; i < iterations; ++i) {
      SetUpFrames(config.sample_rate_hz, config.number_of_channels);
      wave_generator.GenerateNextFrame(&frame1);
      AudioFrameOperations::Mute(&frame2);

      std::vector<AudioFrame*> frames_to_combine = {&frame1};
      if (i % 2 == 0) {
        frames_to_combine.push_back(&frame2);
      }
      const size_t number_of_samples =
          frame1.samples_per_channel_ * config.number_of_channels;

      // Ensures limiter is on if 'use_limiter'.
      constexpr size_t number_of_streams = 2;
      combiner.Combine(frames_to_combine, config.number_of_channels,
                       config.sample_rate_hz, number_of_streams,
                       &audio_frame_for_mixing);
      cumulative_change += change_calculator.CalculateGainChange(
          rtc::ArrayView<const int16_t>(frame1.data(), number_of_samples),
          rtc::ArrayView<const int16_t>(audio_frame_for_mixing.data(),
                                        number_of_samples));
    }

    // Check that the gain doesn't vary too much.
    EXPECT_LT(cumulative_change, 10);

    // Check that the latest gain is within reasonable bounds. It
    // should be slightly less that 1.
    EXPECT_LT(0.9f, change_calculator.LatestGain());
    EXPECT_LT(change_calculator.LatestGain(), 1.01f);
  }
}
}  // namespace webrtc
