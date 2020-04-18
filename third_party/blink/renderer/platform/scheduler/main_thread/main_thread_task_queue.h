// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_MAIN_THREAD_TASK_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_MAIN_THREAD_TASK_QUEUE_H_

#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"

namespace base {
namespace sequence_manager {
class TaskQueueManager;
}
}  // namespace base

namespace blink {

class FrameScheduler;

namespace scheduler {

class MainThreadSchedulerImpl;

class PLATFORM_EXPORT MainThreadTaskQueue
    : public base::sequence_manager::TaskQueue {
 public:
  enum class QueueType {
    // Keep MainThreadTaskQueue::NameForQueueType in sync.
    // This enum is used for a histogram and it should not be re-numbered.
    // TODO(altimin): Clean up obsolete names and use a new histogram when
    // the situation settles.
    kControl = 0,
    kDefault = 1,

    // 2 was used for default loading task runner but this was deprecated.

    // 3 was used for default timer task runner but this was deprecated.

    kUnthrottled = 4,
    kFrameLoading = 5,
    // 6 : kFrameThrottleable, replaced with FRAME_THROTTLEABLE.
    // 7 : kFramePausable, replaced with kFramePausable
    kCompositor = 8,
    kIdle = 9,
    kTest = 10,
    kFrameLoadingControl = 11,
    kFrameThrottleable = 12,
    kFrameDeferrable = 13,
    kFramePausable = 14,
    kFrameUnpausable = 15,
    kV8 = 16,
    kIPC = 17,
    kInput = 18,

    // Detached is used in histograms for tasks which are run after frame
    // is detached and task queue is gracefully shutdown.
    // TODO(altimin): Move to the top when histogram is renumbered.
    kDetached = 19,

    // Used to group multiple types when calculating Expected Queueing Time.
    kOther = 20,
    kCount = 21
  };

  // Returns name of the given queue type. Returned string has application
  // lifetime.
  static const char* NameForQueueType(QueueType queue_type);

  // High-level category used by MainThreadScheduler to make scheduling
  // decisions.
  enum class QueueClass {
    kNone = 0,
    kLoading = 1,
    kTimer = 2,
    kCompositor = 4,

    kCount = 5,
  };

  static QueueClass QueueClassForQueueType(QueueType type);

  struct QueueCreationParams {
    explicit QueueCreationParams(QueueType queue_type)
        : queue_type(queue_type),
          spec(NameForQueueType(queue_type)),
          frame_scheduler(nullptr),
          can_be_deferred(false),
          can_be_throttled(false),
          can_be_paused(false),
          can_be_frozen(false),
          freeze_when_keep_active(false),
          used_for_important_tasks(false) {}

    QueueCreationParams SetFixedPriority(
        base::Optional<base::sequence_manager::TaskQueue::QueuePriority>
            priority) {
      fixed_priority = priority;
      return *this;
    }

    QueueCreationParams SetCanBeDeferred(bool value) {
      can_be_deferred = value;
      return *this;
    }

    QueueCreationParams SetCanBeThrottled(bool value) {
      can_be_throttled = value;
      return *this;
    }

    QueueCreationParams SetCanBePaused(bool value) {
      can_be_paused = value;
      return *this;
    }

    QueueCreationParams SetCanBeFrozen(bool value) {
      can_be_frozen = value;
      return *this;
    }

    QueueCreationParams SetFreezeWhenKeepActive(bool value) {
      freeze_when_keep_active = value;
      return *this;
    }

    QueueCreationParams SetUsedForImportantTasks(bool value) {
      used_for_important_tasks = value;
      return *this;
    }

    // Forwarded calls to |spec|.

    QueueCreationParams SetFrameScheduler(FrameSchedulerImpl* scheduler) {
      frame_scheduler = scheduler;
      return *this;
    }
    QueueCreationParams SetShouldMonitorQuiescence(bool should_monitor) {
      spec = spec.SetShouldMonitorQuiescence(should_monitor);
      return *this;
    }

    QueueCreationParams SetShouldNotifyObservers(bool run_observers) {
      spec = spec.SetShouldNotifyObservers(run_observers);
      return *this;
    }

    QueueCreationParams SetTimeDomain(
        base::sequence_manager::TimeDomain* domain) {
      spec = spec.SetTimeDomain(domain);
      return *this;
    }

    QueueType queue_type;
    base::sequence_manager::TaskQueue::Spec spec;
    base::Optional<base::sequence_manager::TaskQueue::QueuePriority>
        fixed_priority;
    FrameScheduler* frame_scheduler;
    bool can_be_deferred;
    bool can_be_throttled;
    bool can_be_paused;
    bool can_be_frozen;
    bool freeze_when_keep_active;
    bool used_for_important_tasks;
  };

  ~MainThreadTaskQueue() override;

  QueueType queue_type() const { return queue_type_; }

  QueueClass queue_class() const { return queue_class_; }

  base::Optional<base::sequence_manager::TaskQueue::QueuePriority>
  FixedPriority() const {
    return fixed_priority_;
  }

  bool CanBeDeferred() const { return can_be_deferred_; }

  bool CanBeThrottled() const { return can_be_throttled_; }

  bool CanBePaused() const { return can_be_paused_; }

  bool CanBeFrozen() const { return can_be_frozen_; }

  bool FreezeWhenKeepActive() const { return freeze_when_keep_active_; }

  bool UsedForImportantTasks() const { return used_for_important_tasks_; }

  void OnTaskStarted(const base::sequence_manager::TaskQueue::Task& task,
                     base::TimeTicks start);

  void OnTaskCompleted(const base::sequence_manager::TaskQueue::Task& task,
                       base::TimeTicks start,
                       base::TimeTicks end,
                       base::Optional<base::TimeDelta> thread_time);

  void DetachFromMainThreadScheduler();

  // Override base method to notify MainThreadScheduler about shutdown queue.
  void ShutdownTaskQueue() override;

  FrameScheduler* GetFrameScheduler() const;
  void DetachFromFrameScheduler();

 protected:
  void SetFrameSchedulerForTest(FrameScheduler* frame);

  MainThreadTaskQueue(
      std::unique_ptr<base::sequence_manager::internal::TaskQueueImpl> impl,
      const Spec& spec,
      const QueueCreationParams& params,
      MainThreadSchedulerImpl* main_thread_scheduler);

 private:
  friend class base::sequence_manager::TaskQueueManager;

  // Clear references to main thread scheduler and frame scheduler and dispatch
  // appropriate notifications. This is the common part of ShutdownTaskQueue and
  // DetachFromMainThreadScheduler.
  void ClearReferencesToSchedulers();

  const QueueType queue_type_;
  const QueueClass queue_class_;
  const base::Optional<base::sequence_manager::TaskQueue::QueuePriority>
      fixed_priority_;
  const bool can_be_deferred_;
  const bool can_be_throttled_;
  const bool can_be_paused_;
  const bool can_be_frozen_;
  const bool freeze_when_keep_active_;
  const bool used_for_important_tasks_;

  // Needed to notify renderer scheduler about completed tasks.
  MainThreadSchedulerImpl* main_thread_scheduler_;  // NOT OWNED

  FrameScheduler* frame_scheduler_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(MainThreadTaskQueue);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_MAIN_THREAD_TASK_QUEUE_H_
