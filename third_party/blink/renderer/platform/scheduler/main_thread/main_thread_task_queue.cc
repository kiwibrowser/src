// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_task_queue.h"

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

namespace internal {
using base::sequence_manager::internal::TaskQueueImpl;
}

// static
const char* MainThreadTaskQueue::NameForQueueType(
    MainThreadTaskQueue::QueueType queue_type) {
  switch (queue_type) {
    case MainThreadTaskQueue::QueueType::kControl:
      return "control_tq";
    case MainThreadTaskQueue::QueueType::kDefault:
      return "default_tq";
    case MainThreadTaskQueue::QueueType::kUnthrottled:
      return "unthrottled_tq";
    case MainThreadTaskQueue::QueueType::kFrameLoading:
      return "frame_loading_tq";
    case MainThreadTaskQueue::QueueType::kFrameThrottleable:
      return "frame_throttleable_tq";
    case MainThreadTaskQueue::QueueType::kFrameDeferrable:
      return "frame_deferrable_tq";
    case MainThreadTaskQueue::QueueType::kFramePausable:
      return "frame_pausable_tq";
    case MainThreadTaskQueue::QueueType::kFrameUnpausable:
      return "frame_unpausable_tq";
    case MainThreadTaskQueue::QueueType::kCompositor:
      return "compositor_tq";
    case MainThreadTaskQueue::QueueType::kIdle:
      return "idle_tq";
    case MainThreadTaskQueue::QueueType::kTest:
      return "test_tq";
    case MainThreadTaskQueue::QueueType::kFrameLoadingControl:
      return "frame_loading_control_tq";
    case MainThreadTaskQueue::QueueType::kV8:
      return "v8_tq";
    case MainThreadTaskQueue::QueueType::kIPC:
      return "ipc_tq";
    case MainThreadTaskQueue::QueueType::kInput:
      return "input_tq";
    case MainThreadTaskQueue::QueueType::kDetached:
      return "detached_tq";
    case MainThreadTaskQueue::QueueType::kOther:
      return "other_tq";
    case MainThreadTaskQueue::QueueType::kCount:
      NOTREACHED();
      return nullptr;
  }
  NOTREACHED();
  return nullptr;
}

MainThreadTaskQueue::QueueClass MainThreadTaskQueue::QueueClassForQueueType(
    QueueType type) {
  switch (type) {
    case QueueType::kControl:
    case QueueType::kDefault:
    case QueueType::kIdle:
    case QueueType::kTest:
    case QueueType::kV8:
    case QueueType::kIPC:
      return QueueClass::kNone;
    case QueueType::kFrameLoading:
    case QueueType::kFrameLoadingControl:
      return QueueClass::kLoading;
    case QueueType::kUnthrottled:
    case QueueType::kFrameThrottleable:
    case QueueType::kFrameDeferrable:
    case QueueType::kFramePausable:
    case QueueType::kFrameUnpausable:
      return QueueClass::kTimer;
    case QueueType::kCompositor:
    case QueueType::kInput:
      return QueueClass::kCompositor;
    case QueueType::kDetached:
    case QueueType::kOther:
    case QueueType::kCount:
      DCHECK(false);
      return QueueClass::kCount;
  }
  NOTREACHED();
  return QueueClass::kNone;
}

MainThreadTaskQueue::MainThreadTaskQueue(
    std::unique_ptr<internal::TaskQueueImpl> impl,
    const TaskQueue::Spec& spec,
    const QueueCreationParams& params,
    MainThreadSchedulerImpl* main_thread_scheduler)
    : TaskQueue(std::move(impl), spec),
      queue_type_(params.queue_type),
      queue_class_(QueueClassForQueueType(params.queue_type)),
      fixed_priority_(params.fixed_priority),
      can_be_deferred_(params.can_be_deferred),
      can_be_throttled_(params.can_be_throttled),
      can_be_paused_(params.can_be_paused),
      can_be_frozen_(params.can_be_frozen),
      freeze_when_keep_active_(params.freeze_when_keep_active),
      used_for_important_tasks_(params.used_for_important_tasks),
      main_thread_scheduler_(main_thread_scheduler),
      frame_scheduler_(params.frame_scheduler) {
  if (GetTaskQueueImpl()) {
    // TaskQueueImpl may be null for tests.
    // TODO(scheduler-dev): Consider mapping directly to
    // MainThreadSchedulerImpl::OnTaskStarted/Completed. At the moment this
    // is not possible due to task queue being created inside
    // MainThreadScheduler's constructor.
    GetTaskQueueImpl()->SetOnTaskStartedHandler(base::BindRepeating(
        &MainThreadTaskQueue::OnTaskStarted, base::Unretained(this)));
    GetTaskQueueImpl()->SetOnTaskCompletedHandler(base::BindRepeating(
        &MainThreadTaskQueue::OnTaskCompleted, base::Unretained(this)));
  }
}

MainThreadTaskQueue::~MainThreadTaskQueue() = default;

void MainThreadTaskQueue::OnTaskStarted(const TaskQueue::Task& task,
                                        base::TimeTicks start) {
  if (main_thread_scheduler_)
    main_thread_scheduler_->OnTaskStarted(this, task, start);
}

void MainThreadTaskQueue::OnTaskCompleted(
    const TaskQueue::Task& task,
    base::TimeTicks start,
    base::TimeTicks end,
    base::Optional<base::TimeDelta> thread_time) {
  if (main_thread_scheduler_) {
    main_thread_scheduler_->OnTaskCompleted(this, task, start, end,
                                            thread_time);
  }
}

void MainThreadTaskQueue::DetachFromMainThreadScheduler() {
  // Frame has already been detached.
  if (!main_thread_scheduler_)
    return;

  if (GetTaskQueueImpl()) {
    GetTaskQueueImpl()->SetOnTaskStartedHandler(
        base::BindRepeating(&MainThreadSchedulerImpl::OnTaskStarted,
                            main_thread_scheduler_->GetWeakPtr(), nullptr));
    GetTaskQueueImpl()->SetOnTaskCompletedHandler(
        base::BindRepeating(&MainThreadSchedulerImpl::OnTaskCompleted,
                            main_thread_scheduler_->GetWeakPtr(), nullptr));
  }

  ClearReferencesToSchedulers();
}

void MainThreadTaskQueue::ShutdownTaskQueue() {
  ClearReferencesToSchedulers();
  TaskQueue::ShutdownTaskQueue();
}

void MainThreadTaskQueue::ClearReferencesToSchedulers() {
  if (main_thread_scheduler_)
    main_thread_scheduler_->OnShutdownTaskQueue(this);
  main_thread_scheduler_ = nullptr;
  frame_scheduler_ = nullptr;
}

FrameScheduler* MainThreadTaskQueue::GetFrameScheduler() const {
  return frame_scheduler_;
}

void MainThreadTaskQueue::DetachFromFrameScheduler() {
  frame_scheduler_ = nullptr;
}

void MainThreadTaskQueue::SetFrameSchedulerForTest(
    FrameScheduler* frame_scheduler) {
  frame_scheduler_ = frame_scheduler;
}

}  // namespace scheduler
}  // namespace blink
