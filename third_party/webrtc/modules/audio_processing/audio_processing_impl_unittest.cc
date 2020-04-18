/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_processing/audio_processing_impl.h"

#include "modules/audio_processing/test/test_utils.h"
#include "test/gmock.h"
#include "test/gtest.h"

using ::testing::Invoke;

namespace webrtc {
namespace {

class MockInitialize : public AudioProcessingImpl {
 public:
  explicit MockInitialize(const webrtc::Config& config)
      : AudioProcessingImpl(config) {}

  MOCK_METHOD0(InitializeLocked, int());
  int RealInitializeLocked() RTC_NO_THREAD_SAFETY_ANALYSIS {
    return AudioProcessingImpl::InitializeLocked();
  }

  MOCK_CONST_METHOD0(AddRef, void());
  MOCK_CONST_METHOD0(Release, rtc::RefCountReleaseStatus());
};

void GenerateFixedFrame(int16_t audio_level,
                        size_t input_rate,
                        size_t num_channels,
                        AudioFrame* fixed_frame) {
  const size_t samples_per_input_channel = rtc::CheckedDivExact(
      input_rate, static_cast<size_t>(rtc::CheckedDivExact(
                      1000, AudioProcessing::kChunkSizeMs)));
  fixed_frame->samples_per_channel_ = samples_per_input_channel;
  fixed_frame->sample_rate_hz_ = input_rate;
  fixed_frame->num_channels_ = num_channels;

  RTC_DCHECK_LE(samples_per_input_channel * num_channels,
                AudioFrame::kMaxDataSizeSamples);
  for (size_t i = 0; i < samples_per_input_channel * num_channels; ++i) {
    fixed_frame->mutable_data()[i] = audio_level;
  }
}

}  // namespace

TEST(AudioProcessingImplTest, AudioParameterChangeTriggersInit) {
  webrtc::Config config;
  MockInitialize mock(config);
  ON_CALL(mock, InitializeLocked())
      .WillByDefault(Invoke(&mock, &MockInitialize::RealInitializeLocked));

  EXPECT_CALL(mock, InitializeLocked()).Times(1);
  mock.Initialize();

  AudioFrame frame;
  // Call with the default parameters; there should be an init.
  frame.num_channels_ = 1;
  SetFrameSampleRate(&frame, 16000);
  EXPECT_CALL(mock, InitializeLocked()).Times(0);
  EXPECT_NOERR(mock.ProcessStream(&frame));
  EXPECT_NOERR(mock.ProcessReverseStream(&frame));

  // New sample rate. (Only impacts ProcessStream).
  SetFrameSampleRate(&frame, 32000);
  EXPECT_CALL(mock, InitializeLocked())
      .Times(1);
  EXPECT_NOERR(mock.ProcessStream(&frame));

  // New number of channels.
  // TODO(peah): Investigate why this causes 2 inits.
  frame.num_channels_ = 2;
  EXPECT_CALL(mock, InitializeLocked())
      .Times(2);
  EXPECT_NOERR(mock.ProcessStream(&frame));
  // ProcessStream sets num_channels_ == num_output_channels.
  frame.num_channels_ = 2;
  EXPECT_NOERR(mock.ProcessReverseStream(&frame));

  // A new sample rate passed to ProcessReverseStream should cause an init.
  SetFrameSampleRate(&frame, 16000);
  EXPECT_CALL(mock, InitializeLocked()).Times(1);
  EXPECT_NOERR(mock.ProcessReverseStream(&frame));
}

TEST(AudioProcessingImplTest, UpdateCapturePreGainRuntimeSetting) {
  std::unique_ptr<AudioProcessing> apm(AudioProcessingBuilder().Create());
  webrtc::AudioProcessing::Config apm_config;
  apm_config.pre_amplifier.enabled = true;
  apm_config.pre_amplifier.fixed_gain_factor = 1.f;
  apm->ApplyConfig(apm_config);

  AudioFrame frame;
  constexpr int16_t audio_level = 10000;
  constexpr size_t input_rate = 48000;
  constexpr size_t num_channels = 2;

  GenerateFixedFrame(audio_level, input_rate, num_channels, &frame);
  apm->ProcessStream(&frame);
  EXPECT_EQ(frame.data()[100], audio_level)
      << "With factor 1, frame shouldn't be modified.";

  constexpr float gain_factor = 2.f;
  apm->SetRuntimeSetting(
      AudioProcessing::RuntimeSetting::CreateCapturePreGain(gain_factor));

  // Process for two frames to have time to ramp up gain.
  for (int i = 0; i < 2; ++i) {
    GenerateFixedFrame(audio_level, input_rate, num_channels, &frame);
    apm->ProcessStream(&frame);
  }
  EXPECT_EQ(frame.data()[100], gain_factor * audio_level)
      << "Frame should be amplified.";
}

}  // namespace webrtc
