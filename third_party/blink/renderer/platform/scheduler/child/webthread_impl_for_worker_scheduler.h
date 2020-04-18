// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_IMPL_FOR_WORKER_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_IMPL_FOR_WORKER_SCHEDULER_H_

#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/atomic_flag.h"
#include "base/threading/thread.h"
#include "third_party/blink/public/platform/scheduler/child/webthread_base.h"
#include "third_party/blink/public/platform/web_private_ptr.h"

namespace base {
namespace sequence_manager {
class TaskQueue;
}
class WaitableEvent;
}

namespace blink {
class ThreadScheduler;
}

namespace blink {
namespace scheduler {

class SingleThreadIdleTaskRunner;
class NonMainThreadScheduler;
class WorkerSchedulerProxy;

class PLATFORM_EXPORT WebThreadImplForWorkerScheduler
    : public WebThreadBase,
      public base::MessageLoopCurrent::DestructionObserver {
 public:
  explicit WebThreadImplForWorkerScheduler(
      const WebThreadCreationParams& params);
  ~WebThreadImplForWorkerScheduler() override;

  // WebThread implementation.
  ThreadScheduler* Scheduler() const override;
  PlatformThreadId ThreadId() const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() const override;

  // WebThreadBase implementation.
  scheduler::SingleThreadIdleTaskRunner* GetIdleTaskRunner() const override;
  void Init() override;

  // base::MessageLoopCurrent::DestructionObserver implementation.
  void WillDestroyCurrentMessageLoop() override;

  scheduler::NonMainThreadScheduler* GetNonMainThreadScheduler() {
    return non_main_thread_scheduler_.get();
  }

 protected:
  virtual std::unique_ptr<NonMainThreadScheduler>
  CreateNonMainThreadScheduler();

  base::Thread* GetThread() const { return thread_.get(); }

  scheduler::WorkerSchedulerProxy* worker_scheduler_proxy() const {
    return worker_scheduler_proxy_.get();
  }

 private:
  void AddTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer) override;
  void RemoveTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer) override;

  void InitOnThread(base::WaitableEvent* completion);
  void ShutdownOnThread(base::WaitableEvent* completion);

  std::unique_ptr<base::Thread> thread_;
  const WebThreadType thread_type_;
  std::unique_ptr<scheduler::WorkerSchedulerProxy> worker_scheduler_proxy_;
  std::unique_ptr<scheduler::NonMainThreadScheduler> non_main_thread_scheduler_;
  scoped_refptr<base::SingleThreadTaskRunner> thread_task_runner_;
  scoped_refptr<base::sequence_manager::TaskQueue> task_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<scheduler::SingleThreadIdleTaskRunner> idle_task_runner_;

  base::AtomicFlag was_shutdown_on_thread_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_IMPL_FOR_WORKER_SCHEDULER_H_
