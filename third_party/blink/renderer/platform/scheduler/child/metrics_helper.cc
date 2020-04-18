// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/metrics_helper.h"

#include "third_party/blink/renderer/platform/scheduler/child/process_state.h"

namespace blink {
namespace scheduler {

namespace {

// Threshold for discarding ultra-long tasks. It is assumed that ultra-long
// tasks are reporting glitches (e.g. system falling asleep on the middle of the
// task).
constexpr base::TimeDelta kLongTaskDiscardingThreshold =
    base::TimeDelta::FromSeconds(30);

}  // namespace

MetricsHelper::MetricsHelper(WebThreadType thread_type)
    : thread_type_(thread_type),
      thread_task_duration_reporter_(
          "RendererScheduler.TaskDurationPerThreadType"),
      thread_task_cpu_duration_reporter_(
          "RendererScheduler.TaskCPUDurationPerThreadType"),
      foreground_thread_task_duration_reporter_(
          "RendererScheduler.TaskDurationPerThreadType.Foreground"),
      foreground_thread_task_cpu_duration_reporter_(
          "RendererScheduler.TaskCPUDurationPerThreadType.Foreground"),
      background_thread_task_duration_reporter_(
          "RendererScheduler.TaskDurationPerThreadType.Background"),
      background_thread_task_cpu_duration_reporter_(
          "RendererScheduler.TaskCPUDurationPerThreadType.Background") {}

MetricsHelper::~MetricsHelper() {}

bool MetricsHelper::ShouldDiscardTask(
    base::sequence_manager::TaskQueue* queue,
    const base::sequence_manager::TaskQueue::Task& task,
    base::TimeTicks start_time,
    base::TimeTicks end_time,
    base::Optional<base::TimeDelta> thread_time) {
  // TODO(altimin): Investigate the relationship between thread time and
  // wall time for discarded tasks.
  return end_time - start_time > kLongTaskDiscardingThreshold;
}

void MetricsHelper::RecordCommonTaskMetrics(
    base::sequence_manager::TaskQueue* queue,
    const base::sequence_manager::TaskQueue::Task& task,
    base::TimeTicks start_time,
    base::TimeTicks end_time,
    base::Optional<base::TimeDelta> thread_time) {
  base::TimeDelta wall_time = end_time - start_time;

  thread_task_duration_reporter_.RecordTask(thread_type_, wall_time);

  bool backgrounded = internal::ProcessState::Get()->is_process_backgrounded;

  if (backgrounded) {
    background_thread_task_duration_reporter_.RecordTask(thread_type_,
                                                         wall_time);
  } else {
    foreground_thread_task_duration_reporter_.RecordTask(thread_type_,
                                                         wall_time);
  }

  if (!thread_time)
    return;
  thread_task_cpu_duration_reporter_.RecordTask(thread_type_,
                                                thread_time.value());
  if (backgrounded) {
    background_thread_task_cpu_duration_reporter_.RecordTask(
        thread_type_, thread_time.value());
  } else {
    foreground_thread_task_cpu_duration_reporter_.RecordTask(
        thread_type_, thread_time.value());
  }
}

}  // namespace scheduler
}  // namespace blink
