// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/service_metrics.h"

#include <memory>

#include "base/metrics/histogram_macros.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

TEST(AudioServiceMetricsTest, CreateDestroy_LogsUptime) {
  base::SimpleTestTickClock test_clock;
  test_clock.SetNowTicks(base::TimeTicks::Now());

  base::HistogramTester histogram_tester;
  std::unique_ptr<ServiceMetrics> metrics =
      std::make_unique<ServiceMetrics>(&test_clock);
  test_clock.Advance(base::TimeDelta::FromDays(6));
  metrics.reset();
  histogram_tester.ExpectTimeBucketCount("Media.AudioService.Uptime",
                                         base::TimeDelta::FromDays(6), 1);
  histogram_tester.ExpectTotalCount("Media.AudioService.Uptime", 1);
}

TEST(AudioServiceMetricsTest, AddRemoveConnection_LogsHasConnectionDuration) {
  base::SimpleTestTickClock test_clock;
  test_clock.SetNowTicks(base::TimeTicks::Now());

  base::HistogramTester histogram_tester;
  ServiceMetrics metrics(&test_clock);
  metrics.HasConnections();
  test_clock.Advance(base::TimeDelta::FromMinutes(42));
  metrics.HasNoConnections();
  histogram_tester.ExpectTimeBucketCount(
      "Media.AudioService.HasConnectionsDuration",
      base::TimeDelta::FromMinutes(42), 1);
  histogram_tester.ExpectTotalCount("Media.AudioService.HasConnectionsDuration",
                                    1);
}

TEST(AudioServiceMetricsTest, RemoveAddConnection_LogsHasNoConnectionDuration) {
  base::SimpleTestTickClock test_clock;
  test_clock.SetNowTicks(base::TimeTicks::Now());

  base::HistogramTester histogram_tester;
  ServiceMetrics metrics(&test_clock);
  metrics.HasConnections();
  test_clock.Advance(base::TimeDelta::FromMinutes(5));
  metrics.HasNoConnections();
  test_clock.Advance(base::TimeDelta::FromMilliseconds(10));
  metrics.HasConnections();
  histogram_tester.ExpectTimeBucketCount(
      "Media.AudioService.HasNoConnectionsDuration",
      base::TimeDelta::FromMilliseconds(10), 1);
  histogram_tester.ExpectTotalCount(
      "Media.AudioService.HasNoConnectionsDuration", 1);
}

}  // namespace audio
