/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/numerics/samples_stats_counter.h"

#include <algorithm>
#include <vector>

#include "test/gtest.h"

namespace webrtc {
namespace {
SamplesStatsCounter CreateStatsFilledWithIntsFrom1ToN(int n) {
  std::vector<double> data;
  for (int i = 1; i <= n; i++) {
    data.push_back(i);
  }
  std::random_shuffle(data.begin(), data.end());

  SamplesStatsCounter stats;
  for (double v : data) {
    stats.AddSample(v);
  }
  return stats;
}
}  // namespace

TEST(SamplesStatsCounter, FullSimpleTest) {
  SamplesStatsCounter stats = CreateStatsFilledWithIntsFrom1ToN(100);

  ASSERT_TRUE(!stats.IsEmpty());
  ASSERT_DOUBLE_EQ(stats.GetMin(), 1.0);
  ASSERT_DOUBLE_EQ(stats.GetMax(), 100.0);
  ASSERT_DOUBLE_EQ(stats.GetAverage(), 50.5);
  ASSERT_DOUBLE_EQ(stats.GetPercentile(0), 1);
  for (int i = 1; i <= 100; i++) {
    double p = i / 100.0;
    ASSERT_GE(stats.GetPercentile(p), i);
    ASSERT_LT(stats.GetPercentile(p), i + 1);
  }
}

TEST(SamplesStatsCounter, FractionPercentile) {
  SamplesStatsCounter stats = CreateStatsFilledWithIntsFrom1ToN(5);

  ASSERT_DOUBLE_EQ(stats.GetPercentile(0.5), 3);
}

TEST(SamplesStatsCounter, TestBorderValues) {
  SamplesStatsCounter stats = CreateStatsFilledWithIntsFrom1ToN(5);

  ASSERT_GE(stats.GetPercentile(0.01), 1);
  ASSERT_LT(stats.GetPercentile(0.01), 2);
  ASSERT_DOUBLE_EQ(stats.GetPercentile(1.0), 5);
}

}  // namespace webrtc
