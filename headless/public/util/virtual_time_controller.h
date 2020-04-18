// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_VIRTUAL_TIME_CONTROLLER_H_
#define HEADLESS_PUBLIC_UTIL_VIRTUAL_TIME_CONTROLLER_H_

#include "base/callback.h"
#include "base/time/time.h"
#include "headless/public/devtools/domains/emulation.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_export.h"

namespace headless {

// Controls how virtual time progresses. RepeatingTasks can register their
// interest to be periodically notified about changes to the current virtual
// time.
class HEADLESS_EXPORT VirtualTimeController
    : public emulation::ExperimentalObserver {
 public:
  VirtualTimeController(HeadlessDevToolsClient* devtools_client,
                        int max_task_starvation_count = 0);
  ~VirtualTimeController() override;

  // Signals that virtual time should start advancing. If virtual time is
  // already running, this does nothing.  When virtual time is ready to start
  // the observers will be notified.
  virtual void StartVirtualTime();

  class RepeatingTask {
   public:
    // This policy controls whether or not StartVirtualTime() should wait for a
    // navigation first.
    enum StartPolicy {
      WAIT_FOR_NAVIGATION,
      START_IMMEDIATELY,
    };

    explicit RepeatingTask(StartPolicy start_policy, int priority)
        : start_policy_(start_policy), priority_(priority) {}

    virtual ~RepeatingTask() {}

    enum class ContinuePolicy {
      CONTINUE_MORE_TIME_NEEDED,
      NOT_REQUIRED,
      STOP,  // Note STOP trumps CONTINUE_MORE_TIME_NEEDED.
    };

    // Called when the tasks's requested virtual time interval has elapsed.
    // |virtual_time_offset| is the virtual time duration that has advanced
    // since the page started loading (millisecond granularity). When the task
    // has completed it's perioodic work it should call |continue_callback|
    // with CONTINUE_MORE_TIME_NEEDED if it wants virtual time to continue
    // advancing, or NOT_REQUIRED otherwise.  Virtual time will continue to
    // advance until all RepeatingTasks want it to stop.
    virtual void IntervalElapsed(
        base::TimeDelta virtual_time_offset,
        base::OnceCallback<void(ContinuePolicy policy)> continue_callback) = 0;

    StartPolicy start_policy() const { return start_policy_; }

    int priority() const { return priority_; }

   private:
    const StartPolicy start_policy_;

    // If more than one RepeatingTask is scheduled to run at any instant they
    // are run in order of ascending |priority_|.
    const int priority_;
  };

  // An API used by the CompositorController to defer the start and resumption
  // of virtual time until it's ready.
  class ResumeDeferrer {
   public:
    virtual ~ResumeDeferrer() {}

    // Called before virtual time progression resumes after it was stopped or
    // paused to execute repeating tasks.
    virtual void DeferResume(base::OnceClosure ready_callback) = 0;
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Called when StartVirtualTime was called. May be delayed by a
    // StartDeferrer.
    virtual void VirtualTimeStarted(base::TimeDelta virtual_time_offset) = 0;

    // Called when all RepeatingTasks have either voted for virtual time to stop
    // advancing, or all have been removed.
    virtual void VirtualTimeStopped(base::TimeDelta virtual_time_offset) = 0;
  };

  // Interleaves execution of the provided |task| with progression of virtual
  // time. The task will be notified whenever another |interval| of virtual time
  // have elapsed, as well as when the last granted budget has been used up.
  //
  // To ensure that the task is notified of elapsed intervals accurately, it
  // should be added while virtual time is paused.
  virtual void ScheduleRepeatingTask(RepeatingTask* task,
                                     base::TimeDelta interval);
  virtual void CancelRepeatingTask(RepeatingTask* task);

  // Adds an observer which is notified when virtual time starts and stops.
  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);

  // Returns the time that virtual time offsets are relative to.
  virtual base::TimeTicks GetVirtualTimeBase() const;

  // Returns the current virtual time offset. Only accurate while virtual time
  // is paused.
  virtual base::TimeDelta GetCurrentVirtualTimeOffset() const;

  // Returns the current virtual time stamp. Only accurate while virtual time
  // is paused.
  base::TimeTicks GetCurrentVirtualTime() const {
    return GetVirtualTimeBase() + GetCurrentVirtualTimeOffset();
  }

  virtual void SetResumeDeferrer(ResumeDeferrer* resume_deferrer);

 private:
  struct TaskEntry {
    base::TimeDelta interval;
    base::TimeDelta next_execution_time;
    bool ready_to_advance = true;
    RepeatingTask::ContinuePolicy continue_policy =
        RepeatingTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED;
  };

  void ObserverReadyToStart();

  // emulation::Observer implementation:
  void OnVirtualTimeBudgetExpired(
      const emulation::VirtualTimeBudgetExpiredParams& params) override;

  void NotifyTasksAndAdvance();
  void NotifyTaskIntervalElapsed(TaskEntry* entry);
  void NotifyTaskVirtualTimeStarted(TaskEntry* entry);
  void TaskReadyToAdvance(TaskEntry* entry,
                          RepeatingTask::ContinuePolicy continue_policy);

  void SetVirtualTimePolicy(base::TimeDelta next_budget,
                            bool wait_for_navigation);
  void SetVirtualTimePolicyDone(
      std::unique_ptr<emulation::SetVirtualTimePolicyResult>);

  HeadlessDevToolsClient* const devtools_client_;  // NOT OWNED
  ResumeDeferrer* resume_deferrer_ = nullptr;      // NOT OWNED
  const int max_task_starvation_count_;

  base::TimeDelta total_elapsed_time_offset_;
  base::TimeDelta last_budget_;
  // Initial virtual time that virtual time offsets are relative to.
  base::TimeTicks virtual_time_base_;

  struct RepeatingTaskOrdering {
    bool operator()(RepeatingTask* a, RepeatingTask* b) const {
      if (a->priority() == b->priority())
        return a < b;
      return a->priority() < b->priority();
    };
  };

  std::map<RepeatingTask*, TaskEntry, RepeatingTaskOrdering> tasks_;
  std::set<Observer*> observers_;
  bool in_notify_tasks_and_advance_ = false;
  bool virtual_time_started_ = false;
  bool virtual_time_paused_ = true;
  bool should_send_start_notification_ = false;

  base::WeakPtrFactory<VirtualTimeController> weak_ptr_factory_;
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_VIRTUAL_TIME_CONTROLLER_H_
