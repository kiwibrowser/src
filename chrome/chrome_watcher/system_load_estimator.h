// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_WATCHER_SYSTEM_LOAD_ESTIMATOR_H_
#define CHROME_CHROME_WATCHER_SYSTEM_LOAD_ESTIMATOR_H_

#include <windows.h>
#include <pdh.h>
#include <stdint.h>

#include <memory>
#include <type_traits>
#include <vector>

#include "base/time/time.h"

namespace chrome_watcher {

struct QueryDeleter {
  void operator()(PDH_HQUERY query) const;
};
struct CounterDeleter {
  void operator()(PDH_HCOUNTER counter) const;
};

// A very basic estimator of a system's cpu and disk load. It can be
// called to synchronously estimate load over short time duration.
// Note: the estimate of disk load is based on the logical C drive, thereby
//     assuming the user has a C drive and that it is the drive of interest.
// TODO(manzagop): determine whether to refine the disk load estimate.
// TODO(manzagop): consider revising the measurement approach to continuous
//     observation.
// TODO(manzagop): consider relocating outside of chrome_watcher.
class SystemLoadEstimator {
 public:
  // An estimate of system load.
  struct Estimate {
    // An integer between 0 and 100, representing the system's cpu load
    // percentage.
    int cpu_load_pct;
    // Percentage of time the disk was idle.
    int disk_idle_pct;
    // Average disk queue length.
    int avg_disk_queue_len;
  };

  using QueryHandle =
      std::unique_ptr<std::remove_pointer<PDH_HQUERY>::type, QueryDeleter>;
  using CounterHandle =
      std::unique_ptr<std::remove_pointer<PDH_HCOUNTER>::type, CounterDeleter>;

  // Estimates system load over a short duration. On success, returns true and
  // estimate contains the measure of system load. Otherwise, returns false.
  // Note: sleeps for some amount of time.
  static bool Measure(Estimate* load_estimate);

  SystemLoadEstimator();
  virtual ~SystemLoadEstimator();

  // Initializes the estimator. Must be successfully invoked prior to performing
  // measurements. Returns true on success, false otherwise.
  bool Initialize();

  // Estimates system load over a short duration. The estimator must have been
  // sucessfully initialized. On success, returns true and estimate contains the
  // measure of system load. Otherwise, returns false.
  // Note: sleeps for some amount of time.
  virtual bool PerformMeasurements(Estimate* estimate);

 protected:
  // Protected for unittests.
  bool PerformMeasurements(base::TimeDelta duration, Estimate* estimate);
  virtual bool GetTotalAndIdleTimes(uint64_t* total, uint64_t* idle);

 private:
  // Note: query must be destroyed after the counters below.
  QueryHandle pdh_query_;
  CounterHandle pdh_disk_idle_counter_;
  CounterHandle pdh_disk_queue_counter_;
};

}  // namespace chrome_watcher

#endif  // CHROME_CHROME_WATCHER_SYSTEM_LOAD_ESTIMATOR_H_
