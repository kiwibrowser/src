// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_RENDERER_WEBTHREAD_IMPL_FOR_RENDERER_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_RENDERER_WEBTHREAD_IMPL_FOR_RENDERER_SCHEDULER_H_

#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/scheduler/child/webthread_base.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {
class ThreadScheduler;
};

namespace blink {
namespace scheduler {
class MainThreadSchedulerImpl;

class PLATFORM_EXPORT WebThreadImplForRendererScheduler : public WebThreadBase {
 public:
  explicit WebThreadImplForRendererScheduler(
      MainThreadSchedulerImpl* scheduler);
  ~WebThreadImplForRendererScheduler() override;

  // WebThread implementation.
  ThreadScheduler* Scheduler() const override;
  PlatformThreadId ThreadId() const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() const override;

  // WebThreadBase implementation.
  SingleThreadIdleTaskRunner* GetIdleTaskRunner() const override;
  void Init() override;

 private:
  void AddTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer) override;
  void RemoveTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer) override;

  void AddTaskTimeObserverInternal(
      base::sequence_manager::TaskTimeObserver*) override;
  void RemoveTaskTimeObserverInternal(
      base::sequence_manager::TaskTimeObserver*) override;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<SingleThreadIdleTaskRunner> idle_task_runner_;
  MainThreadSchedulerImpl* scheduler_;  // Not owned.
  PlatformThreadId thread_id_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_RENDERER_WEBTHREAD_IMPL_FOR_RENDERER_SCHEDULER_H_
