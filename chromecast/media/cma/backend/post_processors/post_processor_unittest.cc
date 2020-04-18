// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processors/post_processor_unittest.h"
#include "chromecast/media/cma/backend/post_processors/post_processor_wrapper.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "base/logging.h"

namespace chromecast {
namespace media {

namespace {

const float kEpsilon = std::numeric_limits<float>::epsilon();

}  // namespace

namespace post_processor_test {

std::vector<float> LinearChirp(int frames,
                               const std::vector<double>& start_frequencies,
                               const std::vector<double>& end_frequencies) {
  DCHECK_EQ(start_frequencies.size(), end_frequencies.size());
  std::vector<float> chirp(frames * start_frequencies.size());
  for (size_t ch = 0; ch < start_frequencies.size(); ++ch) {
    double angle = 0.0;
    for (int f = 0; f < frames; ++f) {
      angle +=
          start_frequencies[ch] +
          (end_frequencies[ch] - start_frequencies[ch]) * f * M_PI / frames;
      chirp[ch + f * start_frequencies.size()] = sin(angle);
    }
  }
  return chirp;
}

std::vector<float> GetStereoChirp(int frames,
                                  float start_frequency_left,
                                  float end_frequency_left,
                                  float start_frequency_right,
                                  float end_frequency_right) {
  std::vector<double> start_frequencies(2);
  std::vector<double> end_frequencies(2);
  start_frequencies[0] = start_frequency_left;
  start_frequencies[1] = start_frequency_right;
  end_frequencies[0] = end_frequency_left;
  end_frequencies[1] = end_frequency_right;

  return LinearChirp(frames, start_frequencies, end_frequencies);
}

void TestDelay(AudioPostProcessor2* pp,
               int sample_rate,
               int num_input_channels) {
  EXPECT_TRUE(pp->SetSampleRate(sample_rate));

  const int num_output_channels = pp->NumOutputChannels();
  const int test_size_frames = kBufSizeFrames * 100;
  std::vector<float> data_in = LinearChirp(
      test_size_frames, std::vector<double>(num_input_channels, 0.0),
      std::vector<double>(num_input_channels, 1.0));

  const std::vector<float> data_copy = data_in;
  std::vector<float> data_out(data_in.size());
  int expected_delay;
  for (int i = 0; i < test_size_frames; i += kBufSizeFrames) {
    expected_delay = pp->ProcessFrames(&data_in[i * num_input_channels],
                                       kBufSizeFrames, 1.0, 0.0);
    std::memcpy(&data_out[i * num_output_channels], pp->GetOutputBuffer(),
                kBufSizeFrames * sizeof(data_out[0]));
  }

  double max_sum = 0;
  int max_idx = -1;  // index (offset), corresponding to maximum x-correlation.
  // Find the offset of maximum x-correlation of in/out.
  // Search range should be larger than post-processor's expected delay.
  int search_range = expected_delay + kBufSizeFrames;
  for (int offset = 0; offset < search_range; ++offset) {
    double sum = 0.0;
    int upper_search_limit_frames = test_size_frames - search_range;
    for (int f = 0; f < upper_search_limit_frames; ++f) {
      for (int ch = 0; ch < num_output_channels; ++ch) {
        sum += data_copy[f * num_input_channels] *
               data_out[(f + offset) * num_output_channels + ch];
      }
    }

    // No need to normalize because every dot product is the same length.
    if (sum > max_sum) {
      max_sum = sum;
      max_idx = offset;
    }
  }
  EXPECT_EQ(max_idx, expected_delay);
}

void TestRingingTime(AudioPostProcessor2* pp,
                     int sample_rate,
                     int num_input_channels) {
  EXPECT_TRUE(pp->SetSampleRate(sample_rate));

  const int num_output_channels = pp->NumOutputChannels();
  const int kNumFrames = GetMaximumFrames(sample_rate);
  const int kSinFreq = 2000;
  int ringing_time_frames = pp->GetRingingTimeInFrames();
  std::vector<float> data;

  // Send a second of data to excite the filter.
  for (int i = 0; i < sample_rate; i += kNumFrames) {
    data = GetSineData(kNumFrames, kSinFreq, sample_rate, num_input_channels);
    pp->ProcessFrames(data.data(), kNumFrames, 1.0, 0.0);
  }
  // Compute the amplitude of the last buffer
  float original_amplitude =
      SineAmplitude(pp->GetOutputBuffer(), num_input_channels * kNumFrames);

  EXPECT_GE(original_amplitude, 0)
      << "Output of nonzero data is 0; cannot test ringing";

  // Feed |ringing_time_frames| of silence.
  if (ringing_time_frames > 0) {
    int frames_remaining = ringing_time_frames;
    int frames_to_process = std::min(ringing_time_frames, kNumFrames);
    while (frames_remaining > 0) {
      frames_to_process = std::min(frames_to_process, frames_remaining);
      data.assign(frames_to_process * num_input_channels, 0);
      pp->ProcessFrames(data.data(), frames_to_process, 1.0, 0.0);
      frames_remaining -= frames_to_process;
    }
  }

  // Send a little more data and ensure the amplitude is < 1% the original.
  const int probe_frames = 100;
  data.assign(probe_frames * num_input_channels, 0);
  pp->ProcessFrames(data.data(), probe_frames, 1.0, 0.0);

  EXPECT_LE(
      SineAmplitude(pp->GetOutputBuffer(), probe_frames * num_output_channels) /
          original_amplitude,
      0.01)
      << "Output level after " << ringing_time_frames << " is more than 1%.";
}

void TestPassthrough(AudioPostProcessor2* pp,
                     int sample_rate,
                     int num_input_channels) {
  EXPECT_TRUE(pp->SetSampleRate(sample_rate));
  const int num_output_channels = pp->NumOutputChannels();
  ASSERT_EQ(num_output_channels, num_input_channels)
      << "\"Passthrough\" is not well defined for "
      << "num_input_channels != num_output_channels";

  const int kNumFrames = GetMaximumFrames(sample_rate);
  const int kSinFreq = 2000;

  std::vector<float> data =
      GetSineData(kNumFrames, kSinFreq, sample_rate, num_input_channels);
  std::vector<float> expected = data;

  int delay_frames = pp->ProcessFrames(data.data(), kNumFrames, 1.0, 0.0);
  int delayed_frames = 0;

  // PostProcessors that run in dedicated threads may need to delay
  // until they get data processed asyncronously.
  while (delay_frames >= delayed_frames + kNumFrames) {
    delayed_frames += kNumFrames;
    for (int i = 0; i < kNumFrames * num_input_channels; ++i) {
      EXPECT_EQ(0.0f, data[i]) << i;
    }
    data = expected;
    delay_frames = pp->ProcessFrames(data.data(), kNumFrames, 1.0, 0.0);

    ASSERT_GE(delay_frames, delayed_frames);
  }

  int delay_samples = (delay_frames - delayed_frames) * num_output_channels;
  ASSERT_LE(delay_samples, num_output_channels * kNumFrames);

  const float* output_data = pp->GetOutputBuffer();

  CheckArraysEqual(expected.data(), output_data + delay_samples,
                   data.size() - delay_samples);
}

int GetMaximumFrames(int sample_rate) {
  return kMaxAudioWriteTimeMilliseconds * sample_rate / 1000;
}

template <typename T>
void CheckArraysEqual(const T* expected, const T* actual, size_t size) {
  std::vector<int> differing_values = CompareArray(expected, actual, size);
  if (differing_values.empty()) {
    return;
  }

  size_t size_to_print =
      std::min(static_cast<size_t>(differing_values[0] + 8), size);
  EXPECT_EQ(differing_values.size(), 0u)
      << "Arrays differ at indices "
      << ::testing::PrintToString(differing_values)
      << "\n  Expected: " << ArrayToString(expected, size_to_print)
      << "\n  Actual:   " << ArrayToString(actual, size_to_print);
}

template <typename T>
std::vector<int> CompareArray(const T* expected, const T* actual, size_t size) {
  std::vector<int> diffs;
  for (size_t i = 0; i < size; ++i) {
    if (std::abs(expected[i] - actual[i]) > kEpsilon) {
      diffs.push_back(i);
    }
  }
  return diffs;
}

template <typename T>
std::string ArrayToString(const T* array, size_t size) {
  std::string result;
  for (size_t i = 0; i < size; ++i) {
    result += ::testing::PrintToString(array[i]) + " ";
  }
  return result;
}

float SineAmplitude(const float* data, int num_frames) {
  double power = 0;
  for (int i = 0; i < num_frames; ++i) {
    power += std::pow(data[i], 2);
  }
  return std::sqrt(power / num_frames) * sqrt(2);
}

std::vector<float> GetSineData(int frames,
                               float frequency,
                               int sample_rate,
                               int num_channels) {
  std::vector<float> sine(frames * num_channels);
  for (int f = 0; f < frames; ++f) {
    for (int ch = 0; ch < num_channels; ++ch) {
      // Offset by a little so that first value is non-zero
      sine[f + ch * num_channels] =
          sin(static_cast<double>(f + ch) * frequency * 2 * M_PI / sample_rate);
    }
  }
  return sine;
}

void TestDelay(AudioPostProcessor* pp, int sample_rate) {
  AudioPostProcessorWrapper ppw(pp, kNumChannels);
  TestDelay(&ppw, sample_rate, kNumChannels);
}

void TestRingingTime(AudioPostProcessor* pp, int sample_rate) {
  AudioPostProcessorWrapper ppw(pp, kNumChannels);
  TestRingingTime(&ppw, sample_rate, kNumChannels);
}
void TestPassthrough(AudioPostProcessor* pp, int sample_rate) {
  AudioPostProcessorWrapper ppw(pp, kNumChannels);
  TestPassthrough(&ppw, sample_rate, kNumChannels);
}

PostProcessorTest::PostProcessorTest() : sample_rate_(GetParam()) {}
PostProcessorTest::~PostProcessorTest() = default;

INSTANTIATE_TEST_CASE_P(SampleRates,
                        PostProcessorTest,
                        ::testing::Values(44100, 48000));

}  // namespace post_processor_test
}  // namespace media
}  // namespace chromecast
