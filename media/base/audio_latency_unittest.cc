// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/audio_latency.h"

#include <stdint.h>

#include "base/logging.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "media/base/limits.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

// TODO(olka): extend unit tests, use real-world sample rates.

TEST(AudioLatency, HighLatencyBufferSizes) {
#if defined(OS_WIN)
  for (int i = 6400; i <= 204800; i *= 2) {
    EXPECT_EQ(2 * (i / 100),
              AudioLatency::GetHighLatencyBufferSize(i, i / 100));
  }
#else
  for (int i = 6400; i <= 204800; i *= 2)
    EXPECT_EQ(2 * (i / 100), AudioLatency::GetHighLatencyBufferSize(i, 32));
#endif  // defined(OS_WIN)
}

TEST(AudioLatency, InteractiveBufferSizes) {
  for (int i = 6400; i <= 204800; i *= 2)
    EXPECT_EQ(i / 100, AudioLatency::GetInteractiveBufferSize(i / 100));
}

TEST(AudioLatency, RtcBufferSizes) {
  for (int i = 6400; i < 204800; i *= 2) {
    EXPECT_EQ(i / 100, AudioLatency::GetRtcBufferSize(i, 0));
#if defined(OS_WIN)
    EXPECT_EQ(500, AudioLatency::GetRtcBufferSize(i, 500));
#elif defined(OS_ANDROID)
    EXPECT_EQ(i / 50, AudioLatency::GetRtcBufferSize(i, i / 50 - 1));
    EXPECT_EQ(i / 50 + 1, AudioLatency::GetRtcBufferSize(i, i / 50 + 1));
#else
    EXPECT_EQ(i / 100, AudioLatency::GetRtcBufferSize(i, 500));
#endif  // defined(OS_WIN)
  }
}

TEST(AudioLatency, ExactBufferSizes) {
  const int hardware_buffer_size = 256;
  const int hardware_sample_rate = 44100;
  const int max_webaudio_buffer_size = 8192;

#if defined(OS_MACOSX) || defined(USE_CRAS)
  const int minimum_buffer_size = limits::kMinAudioBufferSize;
#else
  const int minimum_buffer_size = hardware_buffer_size;
#endif

  EXPECT_EQ(minimum_buffer_size,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(0.0), hardware_sample_rate,
                hardware_buffer_size));
  EXPECT_EQ(
      minimum_buffer_size,
      media::AudioLatency::GetExactBufferSize(
          base::TimeDelta::FromSecondsD(
              minimum_buffer_size / static_cast<double>(hardware_sample_rate)),
          hardware_sample_rate, hardware_buffer_size));
  EXPECT_EQ(minimum_buffer_size * 2,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(
                    (minimum_buffer_size * 2) /
                    static_cast<double>(hardware_sample_rate)),
                hardware_sample_rate, hardware_buffer_size));
  EXPECT_EQ(minimum_buffer_size * 2,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(
                    (minimum_buffer_size * 1.1) /
                    static_cast<double>(hardware_sample_rate)),
                hardware_sample_rate, hardware_buffer_size));
  EXPECT_EQ(max_webaudio_buffer_size,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(10.0), hardware_sample_rate,
                hardware_buffer_size));

#if defined(OS_MACOSX)
  EXPECT_EQ(limits::kMaxAudioBufferSize,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(
                    limits::kMaxAudioBufferSize /
                    static_cast<double>(hardware_sample_rate)),
                hardware_sample_rate, hardware_buffer_size));
  EXPECT_EQ(max_webaudio_buffer_size,
            media::AudioLatency::GetExactBufferSize(
                base::TimeDelta::FromSecondsD(
                    (limits::kMaxAudioBufferSize * 1.1) /
                    static_cast<double>(hardware_sample_rate)),
                hardware_sample_rate, hardware_buffer_size));
#endif

  int previous_buffer_size = 0;
  for (int i = 0; i < 1000; i++) {
    int buffer_size = media::AudioLatency::GetExactBufferSize(
        base::TimeDelta::FromSecondsD(i / 1000.0), hardware_sample_rate,
        hardware_buffer_size);
    EXPECT_GE(buffer_size, previous_buffer_size);
    EXPECT_EQ(buffer_size,
              buffer_size / minimum_buffer_size * minimum_buffer_size);
    previous_buffer_size = buffer_size;
  }
}
}  // namespace media
