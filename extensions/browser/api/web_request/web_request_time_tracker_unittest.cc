// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_time_tracker.h"

#include <stddef.h>
#include <stdint.h>

#include "base/test/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const base::TimeDelta kRequestDelta = base::TimeDelta::FromMilliseconds(100);
const base::TimeDelta kTinyDelay = base::TimeDelta::FromMilliseconds(1);
const base::TimeDelta kModerateDelay = base::TimeDelta::FromMilliseconds(25);
const base::TimeDelta kExcessiveDelay = base::TimeDelta::FromMilliseconds(75);
}  // namespace

// Test the basis recording of histograms.
TEST(ExtensionWebRequestTimeTrackerTest, Histograms) {
  base::HistogramTester histogram_tester;

  ExtensionWebRequestTimeTracker tracker;
  base::Time start;

  tracker.LogRequestStartTime(1, start);
  tracker.LogRequestStartTime(2, start);
  tracker.LogRequestStartTime(3, start);
  tracker.IncrementTotalBlockTime(1, kTinyDelay);
  tracker.IncrementTotalBlockTime(2, kModerateDelay);
  tracker.IncrementTotalBlockTime(2, kModerateDelay);
  tracker.IncrementTotalBlockTime(3, kExcessiveDelay);
  tracker.LogRequestEndTime(1, start + kRequestDelta);
  tracker.LogRequestEndTime(2, start + kRequestDelta);
  tracker.LogRequestEndTime(3, start + kRequestDelta);

  histogram_tester.ExpectTimeBucketCount("Extensions.NetworkDelay", kTinyDelay,
                                         1);
  histogram_tester.ExpectTimeBucketCount("Extensions.NetworkDelay",
                                         2 * kModerateDelay, 1);
  histogram_tester.ExpectTimeBucketCount("Extensions.NetworkDelay",
                                         kExcessiveDelay, 1);
  histogram_tester.ExpectTotalCount("Extensions.NetworkDelay", 3);

  histogram_tester.ExpectBucketCount("Extensions.NetworkDelayPercentage", 1, 1);
  histogram_tester.ExpectBucketCount("Extensions.NetworkDelayPercentage",
                                     2 * 25, 1);
  histogram_tester.ExpectBucketCount("Extensions.NetworkDelayPercentage", 75,
                                     1);
  histogram_tester.ExpectTotalCount("Extensions.NetworkDelayPercentage", 3);

  EXPECT_TRUE(tracker.request_time_logs_.empty());
}
