// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_descriptor.h"

#include <algorithm>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

using LPCMAudioStreamDescriptor =
    extensions::WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream;

namespace extensions {

namespace {

// Copy constructors cannot always be tested by calling them directly as
// a compiler is allowed to optimize copy constructor calls away in certain
// cases (that is called return value optimization). Therefore, this helper
// function is needed to really create a copy of an object.
template <typename T>
T Copy(const T& t) {
  return t;
}

class Data : public std::vector<uint8_t> {
 public:
  template <size_t N>
  explicit Data(const char (&str)[N]) {
    EXPECT_EQ('\0', str[N - 1]);
    insert(end(), str, str + N - 1);
  }

  bool operator==(const WiFiDisplayElementaryStreamDescriptor& rhs) const {
    return size() == rhs.size() && std::equal(begin(), end(), rhs.begin());
  }
};

TEST(WiFiDisplayElementaryStreamDescriptorTest, AVCTimingAndHRDDescriptor) {
  using AVCTimingAndHRDDescriptor =
      WiFiDisplayElementaryStreamDescriptor::AVCTimingAndHRD;
  EXPECT_EQ(Data("\x2A\x02\x7E\x1F"), AVCTimingAndHRDDescriptor::Create());
  EXPECT_EQ(Data("\x2A\x02\x7E\x1F"),
            Copy(AVCTimingAndHRDDescriptor::Create()));
}

TEST(WiFiDisplayElementaryStreamDescriptorTest, AVCVideoDescriptor) {
  using AVCVideoDescriptor = WiFiDisplayElementaryStreamDescriptor::AVCVideo;
  EXPECT_EQ(Data("\x28\x04\x42\x00\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, false, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x00\x2A\x3F"),
            Copy(AVCVideoDescriptor::Create(
                AVCVideoDescriptor::PROFILE_BASELINE, false, false, false, 0x0u,
                AVCVideoDescriptor::LEVEL_4_2, false)));
  EXPECT_EQ(Data("\x28\x04\x64\x00\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_HIGH, false,
                                       false, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x80\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       true, false, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x40\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, true, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x20\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, false, true, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x1F\x2A\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, false, false, 0x1Fu,
                                       AVCVideoDescriptor::LEVEL_4_2, false));
  EXPECT_EQ(Data("\x28\x04\x42\x00\x1F\x3F"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, false, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_3_1, false));
  EXPECT_EQ(Data("\x28\x04\x42\x00\x2A\xBF"),
            AVCVideoDescriptor::Create(AVCVideoDescriptor::PROFILE_BASELINE,
                                       false, false, false, 0x0u,
                                       AVCVideoDescriptor::LEVEL_4_2, true));
}

class LPCMAudioStreamDescriptorTest
    : public testing::TestWithParam<
          testing::tuple<LPCMAudioStreamDescriptor::SamplingFrequency,
                         LPCMAudioStreamDescriptor::BitsPerSample,
                         bool,
                         LPCMAudioStreamDescriptor::NumberOfChannels,
                         Data>> {
 protected:
  LPCMAudioStreamDescriptorTest()
      : sampling_frequency_(testing::get<0>(GetParam())),
        bits_per_sample_(testing::get<1>(GetParam())),
        emphasis_flag_(testing::get<2>(GetParam())),
        number_of_channels_(testing::get<3>(GetParam())),
        expected_data_(testing::get<4>(GetParam())),
        descriptor_(LPCMAudioStreamDescriptor::Create(sampling_frequency_,
                                                      bits_per_sample_,
                                                      emphasis_flag_,
                                                      number_of_channels_)) {}

  const LPCMAudioStreamDescriptor::SamplingFrequency sampling_frequency_;
  const LPCMAudioStreamDescriptor::BitsPerSample bits_per_sample_;
  const bool emphasis_flag_;
  const LPCMAudioStreamDescriptor::NumberOfChannels number_of_channels_;
  const Data expected_data_;
  const WiFiDisplayElementaryStreamDescriptor descriptor_;
};

TEST_P(LPCMAudioStreamDescriptorTest, Create) {
  EXPECT_EQ(expected_data_, descriptor_);
  EXPECT_EQ(expected_data_, Copy(descriptor_));
}

TEST_P(LPCMAudioStreamDescriptorTest, Accessors) {
  ASSERT_EQ(LPCMAudioStreamDescriptor::kTag, descriptor_.tag());
  const LPCMAudioStreamDescriptor& descriptor =
      *static_cast<const LPCMAudioStreamDescriptor*>(&descriptor_);
  EXPECT_EQ(sampling_frequency_, descriptor.sampling_frequency());
  EXPECT_EQ(bits_per_sample_, descriptor.bits_per_sample());
  EXPECT_EQ(emphasis_flag_, descriptor.emphasis_flag());
  EXPECT_EQ(number_of_channels_, descriptor.number_of_channels());
}

INSTANTIATE_TEST_CASE_P(
    WiFiDisplayElementaryStreamDescriptorTests,
    LPCMAudioStreamDescriptorTest,
    testing::Values(testing::make_tuple(
                        LPCMAudioStreamDescriptor::SAMPLING_FREQUENCY_44_1K,
                        LPCMAudioStreamDescriptor::BITS_PER_SAMPLE_16,
                        false,
                        LPCMAudioStreamDescriptor::NUMBER_OF_CHANNELS_DUAL_MONO,
                        Data("\x83\x02\x26\x0F")),
                    testing::make_tuple(
                        LPCMAudioStreamDescriptor::SAMPLING_FREQUENCY_48K,
                        LPCMAudioStreamDescriptor::BITS_PER_SAMPLE_16,
                        false,
                        LPCMAudioStreamDescriptor::NUMBER_OF_CHANNELS_DUAL_MONO,
                        Data("\x83\x02\x46\x0F")),
                    testing::make_tuple(
                        LPCMAudioStreamDescriptor::SAMPLING_FREQUENCY_44_1K,
                        LPCMAudioStreamDescriptor::BITS_PER_SAMPLE_16,
                        true,
                        LPCMAudioStreamDescriptor::NUMBER_OF_CHANNELS_DUAL_MONO,
                        Data("\x83\x02\x27\x0F")),
                    testing::make_tuple(
                        LPCMAudioStreamDescriptor::SAMPLING_FREQUENCY_44_1K,
                        LPCMAudioStreamDescriptor::BITS_PER_SAMPLE_16,
                        false,
                        LPCMAudioStreamDescriptor::NUMBER_OF_CHANNELS_STEREO,
                        Data("\x83\x02\x26\x2F"))));

}  // namespace
}  // namespace extensions
