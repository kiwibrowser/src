// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler.h"

#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/wake_up_budget_pool.h"
#include "third_party/blink/renderer/platform/scheduler/public/non_main_thread_scheduler.h"

namespace blink {
namespace scheduler {

WorkerScheduler::WorkerScheduler(
    NonMainThreadScheduler* non_main_thread_scheduler)
    : default_task_queue_(non_main_thread_scheduler->CreateTaskRunner()),
      throttleable_task_queue_(non_main_thread_scheduler->CreateTaskRunner()),
      thread_scheduler_(non_main_thread_scheduler) {
  thread_scheduler_->RegisterWorkerScheduler(this);
  if (WakeUpBudgetPool* wake_up_budget_pool =
          thread_scheduler_->wake_up_budget_pool()) {
    wake_up_budget_pool->AddQueue(thread_scheduler_->GetTickClock()->NowTicks(),
                                  throttleable_task_queue_.get());
  }
}

WorkerScheduler::~WorkerScheduler() {
#if DCHECK_IS_ON()
  DCHECK(is_disposed_);
#endif
}

std::unique_ptr<FrameOrWorkerScheduler::ActiveConnectionHandle>
WorkerScheduler::OnActiveConnectionCreated() {
  return nullptr;
}

void WorkerScheduler::Dispose() {
  if (TaskQueueThrottler* throttler =
          thread_scheduler_->task_queue_throttler()) {
    throttler->ShutdownTaskQueue(throttleable_task_queue_.get());
  }

  thread_scheduler_->UnregisterWorkerScheduler(this);

  default_task_queue_->ShutdownTaskQueue();
  throttleable_task_queue_->ShutdownTaskQueue();

#if DCHECK_IS_ON()
  is_disposed_ = true;
#endif
}

scoped_refptr<base::SingleThreadTaskRunner> WorkerScheduler::GetTaskRunner(
    TaskType type) const {
  switch (type) {
    case TaskType::kJavascriptTimer:
    case TaskType::kPostedMessage:
      return TaskQueueWithTaskType::Create(throttleable_task_queue_, type);
    case TaskType::kDeprecatedNone:
    case TaskType::kDOMManipulation:
    case TaskType::kUserInteraction:
    case TaskType::kNetworking:
    case TaskType::kNetworkingControl:
    case TaskType::kHistoryTraversal:
    case TaskType::kEmbed:
    case TaskType::kMediaElementEvent:
    case TaskType::kCanvasBlobSerialization:
    case TaskType::kMicrotask:
    case TaskType::kRemoteEvent:
    case TaskType::kWebSocket:
    case TaskType::kUnshippedPortMessage:
    case TaskType::kFileReading:
    case TaskType::kDatabaseAccess:
    case TaskType::kPresentation:
    case TaskType::kSensor:
    case TaskType::kPerformanceTimeline:
    case TaskType::kWebGL:
    case TaskType::kIdleTask:
    case TaskType::kMiscPlatformAPI:
    case TaskType::kInternalDefault:
    case TaskType::kInternalLoading:
    case TaskType::kUnthrottled:
    case TaskType::kInternalTest:
    case TaskType::kInternalWebCrypto:
    case TaskType::kInternalIndexedDB:
    case TaskType::kInternalMedia:
    case TaskType::kInternalMediaRealTime:
    case TaskType::kInternalIPC:
    case TaskType::kInternalUserInteraction:
    case TaskType::kInternalInspector:
    case TaskType::kInternalWorker:
    case TaskType::kInternalIntersectionObserver:
      // UnthrottledTaskRunner is generally discouraged in future.
      // TODO(nhiroki): Identify which tasks can be throttled / suspendable and
      // move them into other task runners. See also comments in
      // Get(LocalFrame). (https://crbug.com/670534)
      return TaskQueueWithTaskType::Create(default_task_queue_, type);
    case TaskType::kMainThreadTaskQueueV8:
    case TaskType::kMainThreadTaskQueueCompositor:
    case TaskType::kMainThreadTaskQueueDefault:
    case TaskType::kMainThreadTaskQueueInput:
    case TaskType::kMainThreadTaskQueueIdle:
    case TaskType::kMainThreadTaskQueueIPC:
    case TaskType::kMainThreadTaskQueueControl:
    case TaskType::kCompositorThreadTaskQueueDefault:
    case TaskType::kWorkerThreadTaskQueueDefault:
    case TaskType::kCount:
      NOTREACHED();
      break;
  }
  NOTREACHED();
  return nullptr;
}

void WorkerScheduler::OnThrottlingStateChanged(
    FrameScheduler::ThrottlingState throttling_state) {
  if (throttling_state_ == throttling_state)
    return;
  throttling_state_ = throttling_state;

  if (TaskQueueThrottler* throttler =
          thread_scheduler_->task_queue_throttler()) {
    if (throttling_state_ == FrameScheduler::ThrottlingState::kThrottled) {
      throttler->IncreaseThrottleRefCount(throttleable_task_queue_.get());
    } else {
      throttler->DecreaseThrottleRefCount(throttleable_task_queue_.get());
    }
  }
}

scoped_refptr<base::sequence_manager::TaskQueue>
WorkerScheduler::DefaultTaskQueue() {
  return default_task_queue_.get();
}

scoped_refptr<base::sequence_manager::TaskQueue>
WorkerScheduler::ThrottleableTaskQueue() {
  return throttleable_task_queue_.get();
}

}  // namespace scheduler
}  // namespace blink
