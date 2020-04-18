// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/compositor_metrics_helper.h"

namespace blink {
namespace scheduler {

CompositorMetricsHelper::CompositorMetricsHelper()
    : MetricsHelper(WebThreadType::kCompositorThread) {}

CompositorMetricsHelper::~CompositorMetricsHelper() {}

void CompositorMetricsHelper::RecordTaskMetrics(
    WorkerTaskQueue* queue,
    const base::sequence_manager::TaskQueue::Task& task,
    base::TimeTicks start_time,
    base::TimeTicks end_time,
    base::Optional<base::TimeDelta> thread_time) {
  if (ShouldDiscardTask(queue, task, start_time, end_time, thread_time))
    return;

  MetricsHelper::RecordCommonTaskMetrics(queue, task, start_time, end_time,
                                         thread_time);
}

}  // namespace scheduler
}  // namespace blink
