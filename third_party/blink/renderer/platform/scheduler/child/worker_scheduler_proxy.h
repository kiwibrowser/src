// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/child/page_visibility_state.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_origin_type.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {
namespace scheduler {
class WorkerThreadScheduler;

// Helper class for communication between frame scheduler (main thread) and
// worker scheduler (worker thread).
//
// It's owned by DedicatedWorkerThread and is created and destroyed
// on the main thread. It's passed to WorkerScheduler during its construction.
// Given that DedicatedWorkerThread object outlives worker thread, this class
// outlives worker thread too.
class PLATFORM_EXPORT WorkerSchedulerProxy : public FrameScheduler::Observer {
 public:
  explicit WorkerSchedulerProxy(FrameScheduler* scheduler);
  ~WorkerSchedulerProxy() override;

  void OnWorkerSchedulerCreated(
      base::WeakPtr<WorkerThreadScheduler> worker_scheduler);

  void OnThrottlingStateChanged(
      FrameScheduler::ThrottlingState throttling_state) override;

  // Should be accessed only from the main thread or during init.
  FrameScheduler::ThrottlingState throttling_state() const {
    DCHECK(IsMainThread() || !initialized_);
    return throttling_state_;
  }

  FrameOriginType parent_frame_type() const {
    DCHECK(IsMainThread() || !initialized_);
    return parent_frame_type_;
  }

 private:
  // Can be accessed only from the worker thread.
  base::WeakPtr<WorkerThreadScheduler> worker_scheduler_;

  // Const after init on the worker thread.
  scoped_refptr<base::SingleThreadTaskRunner> worker_thread_task_runner_;

  FrameScheduler::ThrottlingState throttling_state_ =
      FrameScheduler::ThrottlingState::kNotThrottled;

  std::unique_ptr<FrameScheduler::ThrottlingObserverHandle>
      throttling_observer_handle_;

  bool initialized_ = false;

  FrameOriginType parent_frame_type_;

  DISALLOW_COPY_AND_ASSIGN(WorkerSchedulerProxy);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_SCHEDULER_PROXY_H_
