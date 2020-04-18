// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processors/post_processor_benchmark.h"
#include "chromecast/media/cma/backend/post_processors/post_processor_wrapper.h"

#include <cmath>
#include <ctime>

#include "base/logging.h"

namespace chromecast {
namespace media {
namespace post_processor_test {

namespace {

const float kTestDurationSec = 1.0;
const int kBlockSizeFrames = 256;

}  // namespace

void AudioProcessorBenchmark(AudioPostProcessor2* pp,
                             int sample_rate,
                             int num_input_channels) {
  int test_size_frames = kTestDurationSec * sample_rate;
  // Make test_size multiple of kBlockSizeFrames and calculate effective
  // duration.
  test_size_frames -= test_size_frames % kBlockSizeFrames;
  float effective_duration = static_cast<float>(test_size_frames) / sample_rate;
  std::vector<float> data_in = LinearChirp(
      test_size_frames, std::vector<double>(num_input_channels, 0.0),
      std::vector<double>(num_input_channels, 1.0));
  clock_t start_clock = clock();
  for (int i = 0; i < test_size_frames; i += kBlockSizeFrames * kNumChannels) {
    pp->ProcessFrames(&data_in[i], kBlockSizeFrames, 1.0, 0.0);
  }
  clock_t stop_clock = clock();
  LOG(INFO) << "At " << sample_rate
            << " frames per second CPU usage: " << std::defaultfloat
            << 100.0 * (stop_clock - start_clock) /
                   (CLOCKS_PER_SEC * effective_duration)
            << "%";
}

void AudioProcessorBenchmark(AudioPostProcessor* pp, int sample_rate) {
  AudioPostProcessorWrapper wrapper(pp, kNumChannels);
  AudioProcessorBenchmark(&wrapper, sample_rate, kNumChannels);
}

}  // namespace post_processor_test
}  // namespace media
}  // namespace chromecast
