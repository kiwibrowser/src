// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/graceful_queue_shutdown_helper.h"
#include "third_party/blink/renderer/platform/scheduler/base/moveable_auto_lock.h"

namespace base {
namespace trace_event {
class BlameContext;
}
}  // namespace base

namespace base {
namespace sequence_manager {

namespace internal {
class TaskQueueImpl;
}

class TimeDomain;
class TaskQueueManagerImpl;

class PLATFORM_EXPORT TaskQueue : public SingleThreadTaskRunner {
 public:
  class PLATFORM_EXPORT Observer {
   public:
    virtual ~Observer() = default;

    // Notify observer that the time at which this queue wants to run
    // the next task has changed. |next_wakeup| can be in the past
    // (e.g. TimeTicks() can be used to notify about immediate work).
    // Can be called on any thread
    // All methods but SetObserver, SetTimeDomain and GetTimeDomain can be
    // called on |queue|.
    //
    // TODO(altimin): Make it Optional<TimeTicks> to tell
    // observer about cancellations.
    virtual void OnQueueNextWakeUpChanged(TaskQueue* queue,
                                          TimeTicks next_wake_up) = 0;
  };

  // A wrapper around OnceClosure with additional metadata to be passed
  // to PostTask and plumbed until PendingTask is created.
  struct PLATFORM_EXPORT PostedTask {
    PostedTask(OnceClosure callback,
               Location posted_from,
               TimeDelta delay = TimeDelta(),
               Nestable nestable = Nestable::kNestable,
               int task_type = 0);
    PostedTask(PostedTask&& move_from);
    PostedTask(const PostedTask& copy_from) = delete;
    ~PostedTask();

    OnceClosure callback;
    Location posted_from;
    TimeDelta delay;
    Nestable nestable;
    int task_type;
  };

  // Unregisters the task queue after which no tasks posted to it will run and
  // the TaskQueueManagerImpl's reference to it will be released soon.
  virtual void ShutdownTaskQueue();

  enum QueuePriority {
    // Queues with control priority will run before any other queue, and will
    // explicitly starve other queues. Typically this should only be used for
    // private queues which perform control operations.
    kControlPriority,

    // The selector will prioritize highest over high, normal and low; and
    // high over normal and low; and normal over low. However it will ensure
    // neither of the lower priority queues can be completely starved by higher
    // priority tasks. All three of these queues will always take priority over
    // and can starve the best effort queue.
    kHighestPriority,

    kHighPriority,

    // Queues with normal priority are the default.
    kNormalPriority,
    kLowPriority,

    // Queues with best effort priority will only be run if all other queues are
    // empty. They can be starved by the other queues.
    kBestEffortPriority,
    // Must be the last entry.
    kQueuePriorityCount,
    kFirstQueuePriority = kControlPriority,
  };

  // Can be called on any thread.
  static const char* PriorityToString(QueuePriority priority);

  // Options for constructing a TaskQueue.
  struct Spec {
    explicit Spec(const char* name)
        : name(name),
          should_monitor_quiescence(false),
          time_domain(nullptr),
          should_notify_observers(true) {}

    Spec SetShouldMonitorQuiescence(bool should_monitor) {
      should_monitor_quiescence = should_monitor;
      return *this;
    }

    Spec SetShouldNotifyObservers(bool run_observers) {
      should_notify_observers = run_observers;
      return *this;
    }

    Spec SetTimeDomain(TimeDomain* domain) {
      time_domain = domain;
      return *this;
    }

    const char* name;
    bool should_monitor_quiescence;
    TimeDomain* time_domain;
    bool should_notify_observers;
  };

  // Interface to pass per-task metadata to RendererScheduler.
  class PLATFORM_EXPORT Task : public PendingTask {
   public:
    Task(PostedTask posted_task, TimeTicks desired_run_time);

    int task_type() const { return task_type_; }

   private:
    int task_type_;
  };

  // An interface that lets the owner vote on whether or not the associated
  // TaskQueue should be enabled.
  class QueueEnabledVoter {
   public:
    QueueEnabledVoter() = default;
    virtual ~QueueEnabledVoter() = default;

    // Votes to enable or disable the associated TaskQueue. The TaskQueue will
    // only be enabled if all the voters agree it should be enabled, or if there
    // are no voters.
    // NOTE this must be called on the thread the associated TaskQueue was
    // created on.
    virtual void SetQueueEnabled(bool enabled) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(QueueEnabledVoter);
  };

  // Returns an interface that allows the caller to vote on whether or not this
  // TaskQueue is enabled. The TaskQueue will be enabled if there are no voters
  // or if all agree it should be enabled.
  // NOTE this must be called on the thread this TaskQueue was created by.
  std::unique_ptr<QueueEnabledVoter> CreateQueueEnabledVoter();

  // NOTE this must be called on the thread this TaskQueue was created by.
  bool IsQueueEnabled() const;

  // Returns true if the queue is completely empty.
  bool IsEmpty() const;

  // Returns the number of pending tasks in the queue.
  size_t GetNumberOfPendingTasks() const;

  // Returns true if the queue has work that's ready to execute now.
  // NOTE: this must be called on the thread this TaskQueue was created by.
  bool HasTaskToRunImmediately() const;

  // Returns requested run time of next scheduled wake-up for a delayed task
  // which is not ready to run. If there are no such tasks or the queue is
  // disabled (by a QueueEnabledVoter) it returns nullopt.
  // NOTE: this must be called on the thread this TaskQueue was created by.
  Optional<TimeTicks> GetNextScheduledWakeUp();

  // Can be called on any thread.
  virtual const char* GetName() const;

  // Set the priority of the queue to |priority|. NOTE this must be called on
  // the thread this TaskQueue was created by.
  void SetQueuePriority(QueuePriority priority);

  // Returns the current queue priority.
  QueuePriority GetQueuePriority() const;

  // These functions can only be called on the same thread that the task queue
  // manager executes its tasks on.
  void AddTaskObserver(MessageLoop::TaskObserver* task_observer);
  void RemoveTaskObserver(MessageLoop::TaskObserver* task_observer);

  // Set the blame context which is entered and left while executing tasks from
  // this task queue. |blame_context| must be null or outlive this task queue.
  // Must be called on the thread this TaskQueue was created by.
  void SetBlameContext(trace_event::BlameContext* blame_context);

  // Removes the task queue from the previous TimeDomain and adds it to
  // |domain|.  This is a moderately expensive operation.
  void SetTimeDomain(TimeDomain* domain);

  // Returns the queue's current TimeDomain.  Can be called from any thread.
  TimeDomain* GetTimeDomain() const;

  enum class InsertFencePosition {
    kNow,  // Tasks posted on the queue up till this point further may run.
           // All further tasks are blocked.
    kBeginningOfTime,  // No tasks posted on this queue may run.
  };

  // Inserts a barrier into the task queue which prevents tasks with an enqueue
  // order greater than the fence from running until either the fence has been
  // removed or a subsequent fence has unblocked some tasks within the queue.
  // Note: delayed tasks get their enqueue order set once their delay has
  // expired, and non-delayed tasks get their enqueue order set when posted.
  //
  // Fences come in three flavours:
  // - Regular (InsertFence(NOW)) - all tasks posted after this moment
  //   are blocked.
  // - Fully blocking (InsertFence(kBeginningOfTime)) - all tasks including
  //   already posted are blocked.
  // - Delayed (InsertFenceAt(timestamp)) - blocks all tasks posted after given
  //   point in time (must be in the future).
  //
  // Only one fence can be scheduled at a time. Inserting a new fence
  // will automatically remove the previous one, regardless of fence type.
  void InsertFence(InsertFencePosition position);
  void InsertFenceAt(TimeTicks time);

  // Removes any previously added fence and unblocks execution of any tasks
  // blocked by it.
  void RemoveFence();

  bool HasActiveFence();

  // Returns true if the queue has a fence which is blocking execution of tasks.
  bool BlockedByFence() const;

  void SetObserver(Observer* observer);

  // SingleThreadTaskRunner implementation
  bool RunsTasksInCurrentSequence() const override;
  bool PostDelayedTask(const Location& from_here,
                       OnceClosure task,
                       TimeDelta delay) override;
  bool PostNonNestableDelayedTask(const Location& from_here,
                                  OnceClosure task,
                                  TimeDelta delay) override;

  bool PostTaskWithMetadata(PostedTask task);

  // TODO(kraynov): Make protected.
  internal::TaskQueueImpl* GetTaskQueueImpl() const { return impl_.get(); }

 protected:
  TaskQueue(std::unique_ptr<internal::TaskQueueImpl> impl,
            const TaskQueue::Spec& spec);
  ~TaskQueue() override;

 private:
  friend class internal::TaskQueueImpl;
  friend class TaskQueueManagerImpl;

  bool IsOnMainThread() const;

  Optional<MoveableAutoLock> AcquireImplReadLockIfNeeded() const;

  // Take |impl_| and untie it from the enclosing task queue.
  std::unique_ptr<internal::TaskQueueImpl> TakeTaskQueueImpl();

  // |impl_| can be written to on the main thread but can be read from
  // any thread.
  // |impl_lock_| must be acquired when writing to |impl_| or when accessing
  // it from non-main thread. Reading from the main thread does not require
  // a lock.
  mutable Lock impl_lock_;
  std::unique_ptr<internal::TaskQueueImpl> impl_;

  const PlatformThreadId thread_id_;

  const WeakPtr<TaskQueueManagerImpl> task_queue_manager_;

  const scoped_refptr<internal::GracefulQueueShutdownHelper>
      graceful_queue_shutdown_helper_;

  THREAD_CHECKER(main_thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(TaskQueue);
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TASK_QUEUE_H_
