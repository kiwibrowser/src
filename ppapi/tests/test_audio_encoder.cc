// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_audio_encoder.h"

#include "ppapi/c/pp_codecs.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/audio_encoder.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(AudioEncoder);

bool TestAudioEncoder::Init() {
  audio_encoder_interface_ = static_cast<const PPB_AudioEncoder_0_1*>(
      pp::Module::Get()->GetBrowserInterface(PPB_AUDIOENCODER_INTERFACE_0_1));
  return audio_encoder_interface_ && CheckTestingInterface();
}

void TestAudioEncoder::RunTests(const std::string& filter) {
  RUN_CALLBACK_TEST(TestAudioEncoder, AvailableCodecs, filter);
  RUN_CALLBACK_TEST(TestAudioEncoder, IncorrectParametersFails, filter);
  RUN_CALLBACK_TEST(TestAudioEncoder, InitializeTwiceFails, filter);
  RUN_CALLBACK_TEST(TestAudioEncoder, InitializeOpusMono, filter);
  RUN_CALLBACK_TEST(TestAudioEncoder, InitializeOpusStereo, filter);
}

std::string TestAudioEncoder::TestAvailableCodecs() {
  // Test that we get results for supported formats. We should at
  // least get the software supported formats.
  pp::AudioEncoder audio_encoder(instance_);
  ASSERT_FALSE(audio_encoder.is_null());

  TestCompletionCallbackWithOutput<std::vector<PP_AudioProfileDescription> >
      callback(instance_->pp_instance(), false);
  callback.WaitForResult(
      audio_encoder.GetSupportedProfiles(callback.GetCallback()));
  ASSERT_GE(callback.result(), 1U);

  const std::vector<PP_AudioProfileDescription> audio_profiles =
      callback.output();
  ASSERT_GE(audio_profiles.size(), 1U);

  bool found_opus_48hz = false;
  for (uint32_t i = 0; i < audio_profiles.size(); ++i) {
    const PP_AudioProfileDescription& description = audio_profiles[i];
    if (description.profile == PP_AUDIOPROFILE_OPUS &&
        description.sample_rate == PP_AUDIOBUFFER_SAMPLERATE_48000 &&
        description.max_channels >= 2)
      found_opus_48hz = true;
  }
  ASSERT_TRUE(found_opus_48hz);

  PASS();
}

std::string TestAudioEncoder::TestIncorrectParametersFails() {
  // Test that initializing the encoder with incorrect size fails.
  pp::AudioEncoder audio_encoder(instance_);
  ASSERT_FALSE(audio_encoder.is_null());

  // Invalid number of channels.
  TestCompletionCallback callback(instance_->pp_instance(), false);
  callback.WaitForResult(audio_encoder.Initialize(
      42, PP_AUDIOBUFFER_SAMPLERATE_48000, PP_AUDIOBUFFER_SAMPLESIZE_16_BITS,
      PP_AUDIOPROFILE_OPUS, 10000, PP_HARDWAREACCELERATION_WITHFALLBACK,
      callback.GetCallback()));
  ASSERT_EQ(PP_ERROR_NOTSUPPORTED, callback.result());

  // Invalid sampling rate.
  callback.WaitForResult(audio_encoder.Initialize(
      2, static_cast<PP_AudioBuffer_SampleRate>(123456),
      PP_AUDIOBUFFER_SAMPLESIZE_16_BITS, PP_AUDIOPROFILE_OPUS, 10000,
      PP_HARDWAREACCELERATION_WITHFALLBACK, callback.GetCallback()));
  ASSERT_EQ(PP_ERROR_NOTSUPPORTED, callback.result());

  // Invalid sample size.
  callback.WaitForResult(audio_encoder.Initialize(
      2, static_cast<PP_AudioBuffer_SampleRate>(48000),
      static_cast<PP_AudioBuffer_SampleSize>(72), PP_AUDIOPROFILE_OPUS, 10000,
      PP_HARDWAREACCELERATION_WITHFALLBACK, callback.GetCallback()));
  ASSERT_EQ(PP_ERROR_NOTSUPPORTED, callback.result());

  PASS();
}

std::string TestAudioEncoder::TestInitializeTwiceFails() {
  // Test that initializing the encoder with incorrect size fails.
  pp::AudioEncoder audio_encoder(instance_);
  ASSERT_FALSE(audio_encoder.is_null());

  TestCompletionCallback callback(instance_->pp_instance(), false);
  callback.WaitForResult(audio_encoder.Initialize(
      2, PP_AUDIOBUFFER_SAMPLERATE_48000, PP_AUDIOBUFFER_SAMPLESIZE_16_BITS,
      PP_AUDIOPROFILE_OPUS, 10000, PP_HARDWAREACCELERATION_WITHFALLBACK,
      callback.GetCallback()));
  ASSERT_EQ(PP_OK, callback.result());

  callback.WaitForResult(audio_encoder.Initialize(
      2, PP_AUDIOBUFFER_SAMPLERATE_48000, PP_AUDIOBUFFER_SAMPLESIZE_16_BITS,
      PP_AUDIOPROFILE_OPUS, 10000, PP_HARDWAREACCELERATION_WITHFALLBACK,
      callback.GetCallback()));
  ASSERT_EQ(PP_ERROR_FAILED, callback.result());

  PASS();
}

std::string TestAudioEncoder::TestInitializeOpusMono() {
  return TestInitializeCodec(1, PP_AUDIOBUFFER_SAMPLERATE_48000,
                             PP_AUDIOBUFFER_SAMPLESIZE_16_BITS,
                             PP_AUDIOPROFILE_OPUS);
}

std::string TestAudioEncoder::TestInitializeOpusStereo() {
  return TestInitializeCodec(2, PP_AUDIOBUFFER_SAMPLERATE_48000,
                             PP_AUDIOBUFFER_SAMPLESIZE_16_BITS,
                             PP_AUDIOPROFILE_OPUS);
}

std::string TestAudioEncoder::TestInitializeCodec(
    uint32_t channels,
    PP_AudioBuffer_SampleRate input_sample_rate,
    PP_AudioBuffer_SampleSize input_sample_size,
    PP_AudioProfile output_profile) {
  pp::AudioEncoder audio_encoder(instance_);
  ASSERT_FALSE(audio_encoder.is_null());
  pp::Size audio_size(640, 480);

  TestCompletionCallback callback(instance_->pp_instance(), false);
  callback.WaitForResult(audio_encoder.Initialize(
      channels, input_sample_rate, input_sample_size, output_profile, 10000,
      PP_HARDWAREACCELERATION_WITHFALLBACK, callback.GetCallback()));
  ASSERT_EQ(PP_OK, callback.result());

  ASSERT_GE(audio_encoder.GetNumberOfSamples(), 2U);

  TestCompletionCallbackWithOutput<pp::AudioBuffer> get_buffer(
      instance_->pp_instance(), false);
  get_buffer.WaitForResult(audio_encoder.GetBuffer(get_buffer.GetCallback()));
  ASSERT_EQ(PP_OK, get_buffer.result());

  PASS();
}
