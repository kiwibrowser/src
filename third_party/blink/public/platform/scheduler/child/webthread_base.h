// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_BASE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_BASE_H_

#include <map>
#include <memory>

#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_thread.h"

namespace base {
namespace sequence_manager {
class TaskTimeObserver;
}
}  // namespace base

namespace blink {
namespace scheduler {
class SingleThreadIdleTaskRunner;

// TODO(scheduler-dev): Do not expose this class in Blink public API.
class BLINK_PLATFORM_EXPORT WebThreadBase : public WebThread {
 public:
  ~WebThreadBase() override;

  static std::unique_ptr<WebThreadBase> CreateWorkerThread(
      const WebThreadCreationParams& params);
  static std::unique_ptr<WebThreadBase> CreateCompositorThread(
      const WebThreadCreationParams& params);
  // Must be called on utility thread.
  static std::unique_ptr<WebThreadBase> InitializeUtilityThread();

  // WebThread implementation.
  bool IsCurrentThread() const override;
  PlatformThreadId ThreadId() const override = 0;

  virtual void PostIdleTask(const base::Location& location, IdleTask idle_task);

  void AddTaskObserver(TaskObserver* observer) override;
  void RemoveTaskObserver(TaskObserver* observer) override;

  void AddTaskTimeObserver(
      base::sequence_manager::TaskTimeObserver* task_time_observer) override;
  void RemoveTaskTimeObserver(
      base::sequence_manager::TaskTimeObserver* task_time_observer) override;

  // Returns the base::Bind-compatible task runner for posting idle tasks to
  // this thread. Can be called from any thread.
  virtual scheduler::SingleThreadIdleTaskRunner* GetIdleTaskRunner() const = 0;

  virtual void Init() = 0;

 protected:
  class TaskObserverAdapter;

  WebThreadBase();

  virtual void AddTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer);
  virtual void RemoveTaskObserverInternal(
      base::MessageLoop::TaskObserver* observer);

  virtual void AddTaskTimeObserverInternal(
      base::sequence_manager::TaskTimeObserver*) {}
  virtual void RemoveTaskTimeObserverInternal(
      base::sequence_manager::TaskTimeObserver*) {}

  static void RunWebThreadIdleTask(WebThread::IdleTask idle_task,
                                   base::TimeTicks deadline);

 private:
  typedef std::map<TaskObserver*, TaskObserverAdapter*> TaskObserverMap;
  TaskObserverMap task_observer_map_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_CHILD_WEBTHREAD_BASE_H_
