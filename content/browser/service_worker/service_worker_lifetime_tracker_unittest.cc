// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_lifetime_tracker.h"

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/tick_clock.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class ServiceWorkerLifetimeTrackerTest : public testing::Test {
 public:
  ServiceWorkerLifetimeTrackerTest() : tracker_(&tick_clock_) {}

  base::SimpleTestTickClock* tick_clock() { return &tick_clock_; }
  ServiceWorkerLifetimeTracker* tracker() { return &tracker_; }

 private:
  base::SimpleTestTickClock tick_clock_;

  ServiceWorkerLifetimeTracker tracker_;
};

TEST_F(ServiceWorkerLifetimeTrackerTest, Metrics) {
  int64_t kVersion1 = 13;  // dummy value
  int64_t kVersion2 = 14;  // dummy value

  tick_clock()->SetNowTicks(base::TimeTicks::Now());

  // Start a worker.
  tracker()->StartTiming(kVersion2);

  // Run a worker for 10 seconds.
  {
    base::HistogramTester metrics;
    tracker()->StartTiming(kVersion1);
    tick_clock()->Advance(base::TimeDelta::FromSeconds(10));
    tracker()->StopTiming(kVersion1);
    metrics.ExpectTimeBucketCount("ServiceWorker.Runtime",
                                  base::TimeDelta::FromSeconds(10), 1);
  }

  // Advance 10 minutes and stop the worker. It should record Runtime.
  {
    base::HistogramTester metrics;
    tick_clock()->Advance(base::TimeDelta::FromMinutes(10));
    tracker()->StopTiming(kVersion2);
    metrics.ExpectTimeBucketCount(
        "ServiceWorker.Runtime",
        base::TimeDelta::FromMinutes(10) + base::TimeDelta::FromSeconds(10), 1);
  }

  {
    base::HistogramTester metrics;
    // Start a worker and abort the timing.
    tracker()->StartTiming(kVersion1);
    tick_clock()->Advance(base::TimeDelta::FromSeconds(10));
    tracker()->AbortTiming(kVersion1);

    // Aborting multiple times should be fine.
    tracker()->AbortTiming(kVersion1);
    tracker()->AbortTiming(kVersion1);
    // StopTiming should not record a timing.
    tracker()->StopTiming(kVersion1);
    tracker()->StopTiming(kVersion1);

    metrics.ExpectTotalCount("ServiceWorker.Runtime", 0);
  }
}

}  // namespace content
