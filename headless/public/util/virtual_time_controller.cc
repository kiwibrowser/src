// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/virtual_time_controller.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/logging.h"

namespace headless {

using base::TimeDelta;

VirtualTimeController::VirtualTimeController(
    HeadlessDevToolsClient* devtools_client,
    int max_task_starvation_count)
    : devtools_client_(devtools_client),
      max_task_starvation_count_(max_task_starvation_count),
      weak_ptr_factory_(this) {
  devtools_client_->GetEmulation()->GetExperimental()->AddObserver(this);
}

VirtualTimeController::~VirtualTimeController() {
  devtools_client_->GetEmulation()->GetExperimental()->RemoveObserver(this);
}

void VirtualTimeController::StartVirtualTime() {
  if (virtual_time_started_)
    return;

  TimeDelta next_budget;
  bool wait_for_navigation = false;
  for (auto& entry_pair : tasks_) {
    entry_pair.second.ready_to_advance = true;
    if (entry_pair.first->start_policy() ==
        RepeatingTask::StartPolicy::WAIT_FOR_NAVIGATION) {
      wait_for_navigation = true;
    }
    if (next_budget.is_zero()) {
      next_budget =
          entry_pair.second.next_execution_time - total_elapsed_time_offset_;
    } else {
      next_budget =
          std::min(next_budget, entry_pair.second.next_execution_time -
                                    total_elapsed_time_offset_);
    }
  }

  // If there's no budget, then don't do anything!
  if (next_budget.is_zero())
    return;

  virtual_time_started_ = true;
  should_send_start_notification_ = true;

  if (resume_deferrer_) {
    resume_deferrer_->DeferResume(base::BindOnce(
        &VirtualTimeController::SetVirtualTimePolicy,
        weak_ptr_factory_.GetWeakPtr(), next_budget, wait_for_navigation));
  } else {
    SetVirtualTimePolicy(next_budget, wait_for_navigation);
  }
}

void VirtualTimeController::NotifyTasksAndAdvance() {
  // The task may call its continue callback synchronously. Prevent re-entrance.
  if (in_notify_tasks_and_advance_)
    return;

  base::AutoReset<bool> reset(&in_notify_tasks_and_advance_, true);

  for (auto iter = tasks_.begin(); iter != tasks_.end();) {
    auto entry_pair = iter++;
    if (entry_pair->second.next_execution_time <= total_elapsed_time_offset_) {
      entry_pair->second.ready_to_advance = false;
      entry_pair->second.next_execution_time =
          total_elapsed_time_offset_ + entry_pair->second.interval;

      // This may delete itself.
      entry_pair->first->IntervalElapsed(
          total_elapsed_time_offset_,
          base::BindOnce(&VirtualTimeController::TaskReadyToAdvance,
                         weak_ptr_factory_.GetWeakPtr(),
                         base::Unretained(&entry_pair->second)));
    }
  }

  // Give at most as much virtual time as available until the next callback.
  bool advance_virtual_time = false;
  bool stop_virtual_time = false;
  bool ready_to_advance = true;
  TimeDelta next_budget;
  for (const auto& entry_pair : tasks_) {
    ready_to_advance &= entry_pair.second.ready_to_advance;
    if (next_budget.is_zero()) {
      next_budget =
          entry_pair.second.next_execution_time - total_elapsed_time_offset_;
    } else {
      next_budget =
          std::min(next_budget, entry_pair.second.next_execution_time -
                                    total_elapsed_time_offset_);
    }
    if (entry_pair.second.continue_policy ==
        RepeatingTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED) {
      advance_virtual_time = true;
    } else if (entry_pair.second.continue_policy ==
               RepeatingTask::ContinuePolicy::STOP) {
      stop_virtual_time = true;
    }
  }

  if (!ready_to_advance)
    return;

  if (!advance_virtual_time || stop_virtual_time) {
    for (auto& entry_pair : tasks_) {
      entry_pair.second.ready_to_advance = false;
    }

    for (auto iter = observers_.begin(); iter != observers_.end();) {
      Observer* observer = *iter++;
      // |observer| may delete itself.
      observer->VirtualTimeStopped(total_elapsed_time_offset_);
    }
    virtual_time_started_ = false;
    return;
  }

  DCHECK(!next_budget.is_zero());
  if (resume_deferrer_) {
    resume_deferrer_->DeferResume(
        base::BindOnce(&VirtualTimeController::SetVirtualTimePolicy,
                       weak_ptr_factory_.GetWeakPtr(), next_budget,
                       false /* wait_for_navigation */));
  } else {
    SetVirtualTimePolicy(next_budget, false /* wait_for_navigation */);
  }
}

void VirtualTimeController::TaskReadyToAdvance(
    TaskEntry* entry,
    RepeatingTask::ContinuePolicy continue_policy) {
  entry->ready_to_advance = true;
  entry->continue_policy = continue_policy;
  NotifyTasksAndAdvance();
}

void VirtualTimeController::SetVirtualTimePolicy(base::TimeDelta next_budget,
                                                 bool wait_for_navigation) {
  last_budget_ = next_budget;
  devtools_client_->GetEmulation()->GetExperimental()->SetVirtualTimePolicy(
      emulation::SetVirtualTimePolicyParams::Builder()
          .SetPolicy(
              emulation::VirtualTimePolicy::PAUSE_IF_NETWORK_FETCHES_PENDING)
          .SetBudget(next_budget.InMillisecondsF())
          .SetMaxVirtualTimeTaskStarvationCount(max_task_starvation_count_)
          .SetWaitForNavigation(wait_for_navigation)
          .Build(),
      base::BindOnce(&VirtualTimeController::SetVirtualTimePolicyDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void VirtualTimeController::SetVirtualTimePolicyDone(
    std::unique_ptr<emulation::SetVirtualTimePolicyResult> result) {
  if (result) {
    virtual_time_base_ =
        base::TimeTicks() +
        base::TimeDelta::FromMillisecondsD(result->GetVirtualTimeTicksBase());
  } else {
    LOG(WARNING) << "SetVirtualTimePolicy did not succeed";
  }

  if (should_send_start_notification_) {
    should_send_start_notification_ = false;
    for (auto iter = observers_.begin(); iter != observers_.end();) {
      Observer* observer = *iter++;
      // |observer| may delete itself.
      observer->VirtualTimeStarted(total_elapsed_time_offset_);
    }
  }
}

void VirtualTimeController::OnVirtualTimeBudgetExpired(
    const emulation::VirtualTimeBudgetExpiredParams& params) {
  total_elapsed_time_offset_ += last_budget_;
  virtual_time_paused_ = true;
  NotifyTasksAndAdvance();
}

void VirtualTimeController::ScheduleRepeatingTask(RepeatingTask* task,
                                                  base::TimeDelta interval) {
  if (!virtual_time_paused_) {
    // We cannot accurately modify any previously granted virtual time budget.
    LOG(WARNING) << "VirtualTimeController tasks should be added while "
                    "virtual time is paused.";
  }

  TaskEntry entry;
  entry.interval = interval;
  entry.next_execution_time = total_elapsed_time_offset_ + entry.interval;
  tasks_.insert(std::make_pair(task, entry));
}

void VirtualTimeController::CancelRepeatingTask(RepeatingTask* task) {
  tasks_.erase(task);
}

void VirtualTimeController::AddObserver(Observer* observer) {
  observers_.insert(observer);
}

void VirtualTimeController::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}

base::TimeTicks VirtualTimeController::GetVirtualTimeBase() const {
  return virtual_time_base_;
}

base::TimeDelta VirtualTimeController::GetCurrentVirtualTimeOffset() const {
  return total_elapsed_time_offset_;
}

void VirtualTimeController::SetResumeDeferrer(ResumeDeferrer* resume_deferrer) {
  resume_deferrer_ = resume_deferrer;
}

}  // namespace headless
