// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_METRICS_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_METRICS_HELPER_H_

#include "base/optional.h"
#include "base/time/time.h"
#include "third_party/blink/public/platform/web_thread_type.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/util/task_duration_metric_reporter.h"

namespace base {
namespace sequence_manager {
class TaskQueue;
}
}  // namespace base

namespace blink {
namespace scheduler {

// Helper class to take care of task metrics shared between main thread
// and worker threads of the renderer process, including per-thread
// task metrics.
//
// Each thread-specific scheduler should have its own subclass of MetricsHelper
// (MainThreadMetricsHelper, WorkerMetricsHelper, etc) and should call
// RecordCommonTaskMetrics manually.
// Note that this is code reuse, not data reuse -- each thread should have its
// own instantiation of this class.
class PLATFORM_EXPORT MetricsHelper {
 public:
  explicit MetricsHelper(WebThreadType thread_type);
  ~MetricsHelper();

 protected:
  bool ShouldDiscardTask(base::sequence_manager::TaskQueue* queue,
                         const base::sequence_manager::TaskQueue::Task& task,
                         base::TimeTicks start_time,
                         base::TimeTicks end_time,
                         base::Optional<base::TimeDelta> thread_time);

  // Record task metrics which are shared between threads.
  void RecordCommonTaskMetrics(
      base::sequence_manager::TaskQueue* queue,
      const base::sequence_manager::TaskQueue::Task& task,
      base::TimeTicks start_time,
      base::TimeTicks end_time,
      base::Optional<base::TimeDelta> thread_time);

 protected:
  WebThreadType thread_type_;

 private:
  TaskDurationMetricReporter<WebThreadType> thread_task_duration_reporter_;
  TaskDurationMetricReporter<WebThreadType> thread_task_cpu_duration_reporter_;
  TaskDurationMetricReporter<WebThreadType>
      foreground_thread_task_duration_reporter_;
  TaskDurationMetricReporter<WebThreadType>
      foreground_thread_task_cpu_duration_reporter_;
  TaskDurationMetricReporter<WebThreadType>
      background_thread_task_duration_reporter_;
  TaskDurationMetricReporter<WebThreadType>
      background_thread_task_cpu_duration_reporter_;

  DISALLOW_COPY_AND_ASSIGN(MetricsHelper);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_METRICS_HELPER_H_
