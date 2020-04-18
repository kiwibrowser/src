// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H
#define CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H

#include "chromecast/media/cma/backend/post_processors/post_processor_unittest.h"
#include "chromecast/public/media/audio_post_processor_shlib.h"

namespace chromecast {
namespace media {
namespace post_processor_test {

// Measure amount of CPU time |pp| takes to run [x] seconds of stereo audio at
// |sample_rate|.
void AudioProcessorBenchmark(AudioPostProcessor2* pp,
                             int sample_rate,
                             int num_input_channels = kNumChannels);
void AudioProcessorBenchmark(AudioPostProcessor* pp, int sample_rate);

}  // namespace post_processor_test
}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H
