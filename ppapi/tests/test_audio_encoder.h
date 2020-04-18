// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_AUDIO_ENCODER_H_
#define PPAPI_TESTS_TEST_AUDIO_ENCODER_H_

#include <string>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb_audio_encoder.h"
#include "ppapi/tests/test_case.h"

class TestAudioEncoder : public TestCase {
 public:
  explicit TestAudioEncoder(TestingInstance* instance) : TestCase(instance) {}

 private:
  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

  std::string TestAvailableCodecs();
  std::string TestIncorrectParametersFails();
  std::string TestInitializeTwiceFails();
  std::string TestInitializeOpusMono();
  std::string TestInitializeOpusStereo();

  std::string TestInitializeCodec(uint32_t channels,
                                  PP_AudioBuffer_SampleRate input_sample_rate,
                                  PP_AudioBuffer_SampleSize input_sample_size,
                                  PP_AudioProfile output_profile);

  // Used by the tests that access the C API directly.
  const PPB_AudioEncoder_0_1* audio_encoder_interface_;
};

#endif  // PPAPI_TESTS_TEST_AUDIO_ENCODER_H_
