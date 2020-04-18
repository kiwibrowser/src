// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler_proxy.h"

#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/worker/worker_thread_scheduler.h"

namespace blink {
namespace scheduler {

WorkerSchedulerProxy::WorkerSchedulerProxy(FrameScheduler* frame_scheduler) {
  throttling_observer_handle_ = frame_scheduler->AddThrottlingObserver(
      FrameScheduler::ObserverType::kWorkerScheduler, this);
  parent_frame_type_ = GetFrameOriginType(frame_scheduler);
}

WorkerSchedulerProxy::~WorkerSchedulerProxy() {
  DCHECK(IsMainThread());
}

void WorkerSchedulerProxy::OnWorkerSchedulerCreated(
    base::WeakPtr<WorkerThreadScheduler> worker_scheduler) {
  DCHECK(!IsMainThread())
      << "OnWorkerSchedulerCreated should be called from the worker thread";
  DCHECK(!worker_scheduler_) << "OnWorkerSchedulerCreated is called twice";
  DCHECK(worker_scheduler) << "WorkerScheduler is expected to exist";
  worker_scheduler_ = std::move(worker_scheduler);
  worker_thread_task_runner_ = worker_scheduler_->ControlTaskQueue();
  initialized_ = true;
}

void WorkerSchedulerProxy::OnThrottlingStateChanged(
    FrameScheduler::ThrottlingState throttling_state) {
  DCHECK(IsMainThread());
  if (throttling_state_ == throttling_state)
    return;
  throttling_state_ = throttling_state;

  if (!initialized_)
    return;

  worker_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WorkerThreadScheduler::OnThrottlingStateChanged,
                     worker_scheduler_, throttling_state));
}

}  // namespace scheduler
}  // namespace blink
