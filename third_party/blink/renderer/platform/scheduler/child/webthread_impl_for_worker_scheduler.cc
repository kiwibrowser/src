// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/webthread_impl_for_worker_scheduler.h"

#include <memory>
#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/time/default_tick_clock.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler_proxy.h"
#include "third_party/blink/renderer/platform/scheduler/worker/worker_thread_scheduler.h"

namespace blink {
namespace scheduler {

WebThreadImplForWorkerScheduler::WebThreadImplForWorkerScheduler(
    const WebThreadCreationParams& params)
    : thread_(new base::Thread(params.name ? params.name : std::string())),
      thread_type_(params.thread_type),
      worker_scheduler_proxy_(
          params.frame_scheduler
              ? std::make_unique<WorkerSchedulerProxy>(params.frame_scheduler)
              : nullptr) {
  bool started = thread_->StartWithOptions(params.thread_options);
  CHECK(started);
  thread_task_runner_ = thread_->task_runner();
}

void WebThreadImplForWorkerScheduler::Init() {
  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WebThreadImplForWorkerScheduler::InitOnThread,
                                base::Unretained(this), &completion));
  completion.Wait();
}

WebThreadImplForWorkerScheduler::~WebThreadImplForWorkerScheduler() {
  // We want to avoid blocking main thread when the thread was already
  // shut down, but calling ShutdownOnThread twice does not cause any problems.
  if (!was_shutdown_on_thread_.IsSet()) {
    base::WaitableEvent completion(
        base::WaitableEvent::ResetPolicy::AUTOMATIC,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&WebThreadImplForWorkerScheduler::ShutdownOnThread,
                       base::Unretained(this), &completion));
    completion.Wait();
  }
  thread_->Stop();
}

void WebThreadImplForWorkerScheduler::InitOnThread(
    base::WaitableEvent* completion) {
  // TODO(alexclarke): Do we need to unify virtual time for workers and the
  // main thread?
  non_main_thread_scheduler_ = CreateNonMainThreadScheduler();
  non_main_thread_scheduler_->Init();
  task_queue_ = non_main_thread_scheduler_->DefaultTaskQueue();
  task_runner_ = TaskQueueWithTaskType::Create(
      task_queue_, TaskType::kWorkerThreadTaskQueueDefault);
  idle_task_runner_ = non_main_thread_scheduler_->IdleTaskRunner();
  base::MessageLoopCurrent::Get()->AddDestructionObserver(this);
  completion->Signal();
}

void WebThreadImplForWorkerScheduler::ShutdownOnThread(
    base::WaitableEvent* completion) {
  was_shutdown_on_thread_.Set();

  task_queue_ = nullptr;
  task_runner_ = nullptr;
  idle_task_runner_ = nullptr;
  non_main_thread_scheduler_ = nullptr;

  if (completion)
    completion->Signal();
}

std::unique_ptr<NonMainThreadScheduler>
WebThreadImplForWorkerScheduler::CreateNonMainThreadScheduler() {
  return NonMainThreadScheduler::Create(thread_type_,
                                        worker_scheduler_proxy_.get());
}

void WebThreadImplForWorkerScheduler::WillDestroyCurrentMessageLoop() {
  ShutdownOnThread(nullptr);
}

blink::PlatformThreadId WebThreadImplForWorkerScheduler::ThreadId() const {
  return thread_->GetThreadId();
}

blink::ThreadScheduler* WebThreadImplForWorkerScheduler::Scheduler() const {
  return non_main_thread_scheduler_.get();
}

SingleThreadIdleTaskRunner* WebThreadImplForWorkerScheduler::GetIdleTaskRunner()
    const {
  return idle_task_runner_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
WebThreadImplForWorkerScheduler::GetTaskRunner() const {
  return task_runner_;
}

void WebThreadImplForWorkerScheduler::AddTaskObserverInternal(
    base::MessageLoop::TaskObserver* observer) {
  non_main_thread_scheduler_->AddTaskObserver(observer);
}

void WebThreadImplForWorkerScheduler::RemoveTaskObserverInternal(
    base::MessageLoop::TaskObserver* observer) {
  non_main_thread_scheduler_->RemoveTaskObserver(observer);
}

}  // namespace scheduler
}  // namespace blink
