// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTIL_TASK_DURATION_METRIC_REPORTER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTIL_TASK_DURATION_METRIC_REPORTER_H_

#include <array>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/histogram.h"
#include "base/time/time.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/util/aggregated_metric_reporter.h"

namespace base {
class HistogramBase;
}

namespace blink {
namespace scheduler {
namespace internal {
PLATFORM_EXPORT int TakeFullMilliseconds(base::TimeDelta& duration);
}  // namespace internal

// A helper class to report task duration split by a specific type.
// Aggregates small tasks internally and reports only whole milliseconds.
//
// |TaskClass| is an enum which should have COUNT field.
// All values reported to RecordTask should have lower values.
template <class TaskClass>
class TaskDurationMetricReporter
    : public AggregatedMetricReporter<TaskClass, base::TimeDelta> {
 public:
  explicit TaskDurationMetricReporter(const char* metric_name)
      : AggregatedMetricReporter<TaskClass, base::TimeDelta>(
            metric_name,
            &internal::TakeFullMilliseconds) {}

  ~TaskDurationMetricReporter() = default;

 private:
  FRIEND_TEST_ALL_PREFIXES(TaskDurationMetricReporterTest, Test);

  TaskDurationMetricReporter(base::HistogramBase* histogram)
      : AggregatedMetricReporter<TaskClass, base::TimeDelta>(
            histogram,
            &internal::TakeFullMilliseconds) {}

  DISALLOW_COPY_AND_ASSIGN(TaskDurationMetricReporter);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTIL_TASK_DURATION_METRIC_REPORTER_H_
