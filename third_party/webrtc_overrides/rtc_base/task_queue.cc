/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "third_party/webrtc/rtc_base/task_queue.h"

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_local.h"
#include "build/build_config.h"
#include "third_party/webrtc/rtc_base/refcount.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"

using base::WaitableEvent;

namespace rtc {
namespace {

// A lazily created thread local storage for quick access to a TaskQueue.
base::LazyInstance<base::ThreadLocalPointer<TaskQueue>>::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

base::TaskTraits TaskQueuePriority2Traits(TaskQueue::Priority priority) {
  // The content/renderer/media/webrtc/rtc_video_encoder.* code
  // employs a PostTask/Wait pattern that uses TQ in a way that makes it
  // blocking and synchronous, which is why we allow WithBaseSyncPrimitives()
  // for OS_ANDROID.
  switch (priority) {
    case TaskQueue::Priority::HIGH:
#if defined(OS_ANDROID)
      return {base::WithBaseSyncPrimitives(), base::TaskPriority::HIGHEST};
#else
      return {base::TaskPriority::HIGHEST};
#endif
      break;
    case TaskQueue::Priority::LOW:
      return {base::MayBlock(), base::TaskPriority::BACKGROUND};
    case TaskQueue::Priority::NORMAL:
    default:
#if defined(OS_ANDROID)
      return {base::WithBaseSyncPrimitives()};
#else
      return {};
#endif
  }
}

}  // namespace

bool TaskQueue::IsCurrent() const {
  return Current() == this;
}

class TaskQueue::Impl : public RefCountInterface {
 public:
  Impl(const char* queue_name,
       TaskQueue* queue,
       const base::TaskTraits& traits);
  ~Impl() override;

  // To maintain functional compatibility with WebRTC's TaskQueue, we flush
  // and deactivate the task queue here, synchronously.
  // This has some drawbacks and will likely change in the future, but for now
  // is necessary.
  void Stop();

  void PostTask(std::unique_ptr<QueuedTask> task);
  void PostDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds);
  void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                        std::unique_ptr<QueuedTask> reply,
                        TaskQueue* reply_queue);
  void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                        std::unique_ptr<QueuedTask> reply);

 private:
  void RunTask(std::unique_ptr<QueuedTask> task);
  void Deactivate(WaitableEvent* event);

  class PostAndReplyTask : public QueuedTask {
   public:
    PostAndReplyTask(std::unique_ptr<QueuedTask> task,
                     ::scoped_refptr<TaskQueue::Impl> target_queue,
                     std::unique_ptr<QueuedTask> reply,
                     ::scoped_refptr<TaskQueue::Impl> reply_queue)
        : task_(std::move(task)),
          target_queue_(std::move(target_queue)),
          reply_(std::move(reply)),
          reply_queue_(std::move(reply_queue)) {}

    ~PostAndReplyTask() override {}

   private:
    bool Run() override {
      if (task_) {
        target_queue_->RunTask(std::move(task_));
        std::unique_ptr<QueuedTask> t = std::unique_ptr<QueuedTask>(this);
        reply_queue_->PostTask(std::move(t));
        return false;  // Don't delete, ownership lies with reply_queue_.
      }

      reply_queue_->RunTask(std::move(reply_));

      return true;  // OK to delete.
    }

    std::unique_ptr<QueuedTask> task_;
    ::scoped_refptr<TaskQueue::Impl> target_queue_;
    std::unique_ptr<QueuedTask> reply_;
    ::scoped_refptr<TaskQueue::Impl> reply_queue_;
  };

  TaskQueue* const queue_;
  const ::scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool is_active_ = true;  // Checked and set on |task_runner_|.
};

// TaskQueue::Impl.

TaskQueue::Impl::Impl(const char* queue_name,
                      TaskQueue* queue,
                      const base::TaskTraits& traits)
    : queue_(queue),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(traits)) {
  DCHECK(task_runner_);
}

TaskQueue::Impl::~Impl() {}

void TaskQueue::Impl::Stop() {
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&TaskQueue::Impl::Deactivate, this, &event));
  event.Wait();
}

void TaskQueue::Impl::PostTask(std::unique_ptr<QueuedTask> task) {
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&TaskQueue::Impl::RunTask,
                                                   this, base::Passed(&task)));
}

void TaskQueue::Impl::PostDelayedTask(std::unique_ptr<QueuedTask> task,
                                      uint32_t milliseconds) {
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TaskQueue::Impl::RunTask, this, base::Passed(&task)),
      base::TimeDelta::FromMilliseconds(milliseconds));
}

void TaskQueue::Impl::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                       std::unique_ptr<QueuedTask> reply,
                                       TaskQueue* reply_queue) {
  std::unique_ptr<QueuedTask> t =
      std::unique_ptr<QueuedTask>(new PostAndReplyTask(
          std::move(task), this, std::move(reply), reply_queue->impl_.get()));
  PostTask(std::move(t));
}

void TaskQueue::Impl::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                       std::unique_ptr<QueuedTask> reply) {
  PostTaskAndReply(std::move(task), std::move(reply), queue_);
}

void TaskQueue::Impl::RunTask(std::unique_ptr<QueuedTask> task) {
  if (!is_active_)
    return;

  auto* prev = lazy_tls_ptr.Pointer()->Get();
  lazy_tls_ptr.Pointer()->Set(queue_);
  if (!task->Run())
    task.release();
  lazy_tls_ptr.Pointer()->Set(prev);
}

void TaskQueue::Impl::Deactivate(WaitableEvent* event) {
  is_active_ = false;
  event->Signal();
}

// TaskQueue.

TaskQueue::TaskQueue(const char* queue_name,
                     Priority priority /*= Priority::NORMAL*/)
    : impl_(new RefCountedObject<Impl>(queue_name,
                                       this,
                                       TaskQueuePriority2Traits(priority))) {
  DCHECK(queue_name);
}

TaskQueue::~TaskQueue() {
  DCHECK(!IsCurrent());
  impl_->Stop();
}

// static
TaskQueue* TaskQueue::Current() {
  return lazy_tls_ptr.Pointer()->Get();
}

void TaskQueue::PostTask(std::unique_ptr<QueuedTask> task) {
  impl_->PostTask(std::move(task));
}

void TaskQueue::PostDelayedTask(std::unique_ptr<QueuedTask> task,
                                uint32_t milliseconds) {
  impl_->PostDelayedTask(std::move(task), milliseconds);
}

void TaskQueue::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                 std::unique_ptr<QueuedTask> reply,
                                 TaskQueue* reply_queue) {
  impl_->PostTaskAndReply(std::move(task), std::move(reply), reply_queue);
}

void TaskQueue::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                 std::unique_ptr<QueuedTask> reply) {
  impl_->PostTaskAndReply(std::move(task), std::move(reply));
}

}  // namespace rtc
