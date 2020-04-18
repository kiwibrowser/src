// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_watcher/system_load_estimator.h"

#include <memory>
#include <utility>

#include "base/containers/circular_deque.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using SystemLoadEstimator = chrome_watcher::SystemLoadEstimator;
using TimePair = std::pair<uint64_t, uint64_t>;
using TimePairQueue = base::circular_deque<TimePair>;

class SystemLoadEstimatorForTest : public SystemLoadEstimator {
 public:
  SystemLoadEstimatorForTest() {}

  bool PerformMeasurements(Estimate* estimate) override {
    return SystemLoadEstimator::PerformMeasurements(base::TimeDelta(),
                                                    estimate);
  }

  void SetTotalAndIdleTimes(TimePairQueue* times) {
    DCHECK(times);

    times_.swap(*times);
  }

 private:
  bool GetTotalAndIdleTimes(uint64_t* total, uint64_t* idle) override {
    DCHECK(total);
    DCHECK(idle);

    if (times_.empty())
      return false;

    *total = times_.front().first;
    *idle = times_.front().second;

    times_.pop_front();
    return true;
  }

  TimePairQueue times_;
};

}  // namespace

TEST(SystemLoadEstimatorTest, GetCpuLoad) {
  SystemLoadEstimatorForTest estimator;
  ASSERT_TRUE(estimator.Initialize());

  chrome_watcher::SystemLoadEstimator::Estimate estimate{};

  // Fails due to GetTotalAndIdleTimes failure.
  TimePairQueue times;
  estimator.SetTotalAndIdleTimes(&times);
  ASSERT_FALSE(estimator.PerformMeasurements(&estimate));

  // No load.
  times.clear();
  times.push_back(std::make_pair(0ULL, 0ULL));
  times.push_back(std::make_pair(20ULL, 20ULL));
  estimator.SetTotalAndIdleTimes(&times);
  ASSERT_TRUE(estimator.PerformMeasurements(&estimate));
  ASSERT_EQ(0, estimate.cpu_load_pct);

  // Full load.
  times.clear();
  times.push_back(std::make_pair(30ULL, 30ULL));
  times.push_back(std::make_pair(40ULL, 30ULL));
  estimator.SetTotalAndIdleTimes(&times);
  ASSERT_TRUE(estimator.PerformMeasurements(&estimate));
  ASSERT_EQ(100, estimate.cpu_load_pct);

  // 75% load.
  times.clear();
  times.push_back(std::make_pair(5ULL, 5ULL));
  times.push_back(std::make_pair(25ULL, 10ULL));
  estimator.SetTotalAndIdleTimes(&times);
  ASSERT_TRUE(estimator.PerformMeasurements(&estimate));
  ASSERT_EQ(75, estimate.cpu_load_pct);
}

TEST(SystemLoadEstimatorTest, GetDiskLoad) {
  SystemLoadEstimatorForTest estimator;
  ASSERT_TRUE(estimator.Initialize());

  TimePairQueue times;
  times.push_back(std::make_pair(0ULL, 0ULL));
  times.push_back(std::make_pair(20ULL, 20ULL));
  estimator.SetTotalAndIdleTimes(&times);
  chrome_watcher::SystemLoadEstimator::Estimate estimate{};
  ASSERT_TRUE(estimator.PerformMeasurements(&estimate));

  ASSERT_GE(estimate.disk_idle_pct, 0);
  ASSERT_GE(estimate.avg_disk_queue_len, 0);
}
