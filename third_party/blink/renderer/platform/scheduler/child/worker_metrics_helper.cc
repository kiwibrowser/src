// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/worker_metrics_helper.h"

#include "third_party/blink/renderer/platform/scheduler/child/process_state.h"

namespace blink {
namespace scheduler {

WorkerMetricsHelper::WorkerMetricsHelper(WebThreadType thread_type)
    : MetricsHelper(thread_type),
      dedicated_worker_per_task_type_duration_reporter_(
          "RendererScheduler.TaskDurationPerTaskType.DedicatedWorker"),
      dedicated_worker_per_task_type_cpu_duration_reporter_(
          "RendererScheduler.TaskCPUDurationPerTaskType.DedicatedWorker"),
      dedicated_worker_per_parent_frame_status_duration_reporter_(
          "RendererScheduler.TaskDurationPerFrameOriginType.DedicatedWorker"),
      background_dedicated_worker_per_parent_frame_status_duration_reporter_(
          "RendererScheduler.TaskDurationPerFrameOriginType.DedicatedWorker."
          "Background") {}

WorkerMetricsHelper::~WorkerMetricsHelper() {}

void WorkerMetricsHelper::SetParentFrameType(FrameOriginType frame_type) {
  parent_frame_type_ = frame_type;
}

void WorkerMetricsHelper::RecordTaskMetrics(
    WorkerTaskQueue* queue,
    const base::sequence_manager::TaskQueue::Task& task,
    base::TimeTicks start_time,
    base::TimeTicks end_time,
    base::Optional<base::TimeDelta> thread_time) {
  if (ShouldDiscardTask(queue, task, start_time, end_time, thread_time))
    return;

  MetricsHelper::RecordCommonTaskMetrics(queue, task, start_time, end_time,
                                         thread_time);

  bool backgrounded = internal::ProcessState::Get()->is_process_backgrounded;

  if (thread_type_ == WebThreadType::kDedicatedWorkerThread) {
    TaskType task_type = static_cast<TaskType>(task.task_type());
    dedicated_worker_per_task_type_duration_reporter_.RecordTask(
        task_type, end_time - start_time);
    if (thread_time) {
      dedicated_worker_per_task_type_cpu_duration_reporter_.RecordTask(
          task_type, thread_time.value());
    }

    if (parent_frame_type_) {
      dedicated_worker_per_parent_frame_status_duration_reporter_.RecordTask(
          parent_frame_type_.value(), end_time - start_time);

      if (backgrounded) {
        background_dedicated_worker_per_parent_frame_status_duration_reporter_
            .RecordTask(parent_frame_type_.value(), end_time - start_time);
      }
    }
  }
}

}  // namespace scheduler
}  // namespace blink
