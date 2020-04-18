// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_COMPOSITOR_METRICS_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_COMPOSITOR_METRICS_HELPER_H_

#include "third_party/blink/renderer/platform/scheduler/child/metrics_helper.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_task_queue.h"

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT CompositorMetricsHelper : public MetricsHelper {
 public:
  CompositorMetricsHelper();
  ~CompositorMetricsHelper();

  void RecordTaskMetrics(WorkerTaskQueue* queue,
                         const base::sequence_manager::TaskQueue::Task& task,
                         base::TimeTicks start_time,
                         base::TimeTicks end_time,
                         base::Optional<base::TimeDelta> thread_time);

 private:
  DISALLOW_COPY_AND_ASSIGN(CompositorMetricsHelper);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_COMPOSITOR_METRICS_HELPER_H_
