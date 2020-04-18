// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/ordered_simple_task_runner.h"

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"

#define TRACE_TASK(function, task)                                       \
  TRACE_EVENT_INSTANT1("cc", function, TRACE_EVENT_SCOPE_THREAD, "task", \
                       task.AsValue());

#define TRACE_TASK_RUN(function, tag, task)

namespace cc {

// TestOrderablePendingTask implementation
TestOrderablePendingTask::TestOrderablePendingTask()
    : base::TestPendingTask(),
      task_id_(TestOrderablePendingTask::task_id_counter++) {}

TestOrderablePendingTask::TestOrderablePendingTask(
    const base::Location& location,
    base::OnceClosure task,
    base::TimeTicks post_time,
    base::TimeDelta delay,
    TestNestability nestability)
    : base::TestPendingTask(location,
                            std::move(task),
                            post_time,
                            delay,
                            nestability),
      task_id_(TestOrderablePendingTask::task_id_counter++) {}

TestOrderablePendingTask::TestOrderablePendingTask(TestOrderablePendingTask&&) =
    default;

TestOrderablePendingTask& TestOrderablePendingTask::operator=(
    TestOrderablePendingTask&&) = default;

size_t TestOrderablePendingTask::task_id_counter = 0;

TestOrderablePendingTask::~TestOrderablePendingTask() {}

bool TestOrderablePendingTask::operator==(
    const TestOrderablePendingTask& other) const {
  return task_id_ == other.task_id_;
}

bool TestOrderablePendingTask::operator<(
    const TestOrderablePendingTask& other) const {
  if (*this == other)
    return false;

  if (GetTimeToRun() == other.GetTimeToRun()) {
    return task_id_ < other.task_id_;
  }
  return ShouldRunBefore(other);
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
TestOrderablePendingTask::AsValue() const {
  std::unique_ptr<base::trace_event::TracedValue> state(
      new base::trace_event::TracedValue());
  AsValueInto(state.get());
  return std::move(state);
}

void TestOrderablePendingTask::AsValueInto(
    base::trace_event::TracedValue* state) const {
  state->SetInteger("id", base::saturated_cast<int>(task_id_));
  state->SetInteger("run_at", GetTimeToRun().since_origin().InMicroseconds());
  state->SetString("posted_from", location.ToString());
}

OrderedSimpleTaskRunner::OrderedSimpleTaskRunner(
    base::SimpleTestTickClock* now_src,
    bool advance_now)
    : advance_now_(advance_now),
      now_src_(now_src),
      max_tasks_(kAbsoluteMaxTasks),
      inside_run_tasks_until_(false) {}

OrderedSimpleTaskRunner::~OrderedSimpleTaskRunner() {}

// base::TestSimpleTaskRunner implementation
bool OrderedSimpleTaskRunner::PostDelayedTask(const base::Location& from_here,
                                              base::OnceClosure task,
                                              base::TimeDelta delay) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TestOrderablePendingTask pt(from_here, std::move(task), now_src_->NowTicks(),
                              delay, base::TestPendingTask::NESTABLE);

  TRACE_TASK("OrderedSimpleTaskRunner::PostDelayedTask", pt);
  pending_tasks_.insert(std::move(pt));
  return true;
}

bool OrderedSimpleTaskRunner::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TestOrderablePendingTask pt(from_here, std::move(task), now_src_->NowTicks(),
                              delay, base::TestPendingTask::NON_NESTABLE);

  TRACE_TASK("OrderedSimpleTaskRunner::PostNonNestableDelayedTask", pt);
  pending_tasks_.insert(std::move(pt));
  return true;
}

bool OrderedSimpleTaskRunner::RunsTasksInCurrentSequence() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

size_t OrderedSimpleTaskRunner::NumPendingTasks() const {
  return pending_tasks_.size();
}

bool OrderedSimpleTaskRunner::HasPendingTasks() const {
  return pending_tasks_.size() > 0;
}

base::TimeTicks OrderedSimpleTaskRunner::NextTaskTime() {
  RemoveCancelledTasks();

  if (pending_tasks_.size() <= 0) {
    return base::TimeTicks::Max();
  }

  return pending_tasks_.begin()->GetTimeToRun();
}

base::TimeDelta OrderedSimpleTaskRunner::DelayToNextTaskTime() {
  DCHECK(thread_checker_.CalledOnValidThread());
  RemoveCancelledTasks();

  if (pending_tasks_.size() <= 0) {
    return base::TimeTicks::Max().since_origin();
  }

  base::TimeDelta delay = NextTaskTime() - now_src_->NowTicks();
  if (delay > base::TimeDelta())
    return delay;
  return base::TimeDelta();
}

const size_t OrderedSimpleTaskRunner::kAbsoluteMaxTasks =
    std::numeric_limits<size_t>::max();

bool OrderedSimpleTaskRunner::RunTasksWhile(
    base::Callback<bool(void)> condition) {
  std::vector<base::Callback<bool(void)>> conditions(1);
  conditions[0] = condition;
  return RunTasksWhile(conditions);
}

bool OrderedSimpleTaskRunner::RunTasksWhile(
    const std::vector<base::Callback<bool(void)>>& conditions) {
  TRACE_EVENT2("viz", "OrderedSimpleTaskRunner::RunPendingTasks", "this",
               AsValue(), "nested", inside_run_tasks_until_);
  DCHECK(thread_checker_.CalledOnValidThread());

  if (inside_run_tasks_until_)
    return true;

  base::AutoReset<bool> reset_inside_run_tasks_until_(&inside_run_tasks_until_,
                                                      true);

  // Make a copy so we can append some extra run checks.
  std::vector<base::Callback<bool(void)>> modifiable_conditions(conditions);

  // Provide a timeout base on number of tasks run so this doesn't loop
  // forever.
  modifiable_conditions.push_back(TaskRunCountBelow(max_tasks_));

  // If to advance now or not
  if (!advance_now_) {
    modifiable_conditions.push_back(NowBefore(now_src_->NowTicks()));
  } else {
    modifiable_conditions.push_back(AdvanceNow());
  }

  while (pending_tasks_.size() > 0) {
    // Skip canceled tasks.
    if (pending_tasks_.begin()->task.IsCancelled()) {
      pending_tasks_.erase(pending_tasks_.begin());
      continue;
    }
    // Check if we should continue to run pending tasks.
    bool condition_success = true;
    for (std::vector<base::Callback<bool(void)>>::iterator it =
             modifiable_conditions.begin();
         it != modifiable_conditions.end(); it++) {
      condition_success = it->Run();
      if (!condition_success)
        break;
    }

    // Conditions could modify the pending task length, so we need to recheck
    // that there are tasks to run.
    if (!condition_success || !HasPendingTasks()) {
      break;
    }

    std::set<TestOrderablePendingTask>::iterator task_to_run =
        pending_tasks_.begin();
    {
      TRACE_EVENT1("viz", "OrderedSimpleTaskRunner::RunPendingTasks running",
                   "task", task_to_run->AsValue());
      // It's safe to remove const and consume |task| here, since |task| is not
      // used for ordering the item.
      base::OnceClosure& task =
          const_cast<base::OnceClosure&>(task_to_run->task);
      std::move(task).Run();
    }

    pending_tasks_.erase(task_to_run);
  }

  return HasPendingTasks();
}

bool OrderedSimpleTaskRunner::RunPendingTasks() {
  return RunTasksWhile(TaskExistedInitially());
}

bool OrderedSimpleTaskRunner::RunUntilIdle() {
  return RunTasksWhile(std::vector<base::Callback<bool(void)>>());
}

bool OrderedSimpleTaskRunner::RunUntilTime(base::TimeTicks time) {
  // If we are not auto advancing, force now forward to the time.
  if (!advance_now_ && now_src_->NowTicks() < time)
    now_src_->Advance(time - now_src_->NowTicks());

  // Run tasks
  bool result = RunTasksWhile(NowBefore(time));

  bool has_reached_task_limit = HasPendingTasks() && NextTaskTime() <= time;

  // If the next task is after the stopping time and auto-advancing now, then
  // force time to be the stopping time.
  if (!has_reached_task_limit && advance_now_ && now_src_->NowTicks() < time) {
    now_src_->Advance(time - now_src_->NowTicks());
  }

  return result;
}

bool OrderedSimpleTaskRunner::RunForPeriod(base::TimeDelta period) {
  return RunUntilTime(now_src_->NowTicks() + period);
}

// base::trace_event tracing functionality
std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
OrderedSimpleTaskRunner::AsValue() const {
  std::unique_ptr<base::trace_event::TracedValue> state(
      new base::trace_event::TracedValue());
  AsValueInto(state.get());
  return std::move(state);
}

void OrderedSimpleTaskRunner::AsValueInto(
    base::trace_event::TracedValue* state) const {
  state->SetInteger("pending_tasks",
                    base::saturated_cast<int>(pending_tasks_.size()));

  state->BeginArray("tasks");
  for (std::set<TestOrderablePendingTask>::const_iterator it =
           pending_tasks_.begin();
       it != pending_tasks_.end(); ++it) {
    state->BeginDictionary();
    it->AsValueInto(state);
    state->EndDictionary();
  }
  state->EndArray();

  state->BeginDictionary("now_src");
  state->SetDouble("now_in_ms",
                   now_src_->NowTicks().since_origin().InMillisecondsF());
  state->EndDictionary();

  state->SetBoolean("advance_now", advance_now_);
  state->SetBoolean("inside_run_tasks_until", inside_run_tasks_until_);
  state->SetString("max_tasks", base::NumberToString(max_tasks_));
}

base::Callback<bool(void)> OrderedSimpleTaskRunner::TaskRunCountBelow(
    size_t max_tasks) {
  return base::Bind(&OrderedSimpleTaskRunner::TaskRunCountBelowCallback,
                    max_tasks, base::Owned(new size_t(0)));
}

bool OrderedSimpleTaskRunner::TaskRunCountBelowCallback(size_t max_tasks,
                                                        size_t* tasks_run) {
  return (*tasks_run)++ < max_tasks;
}

base::Callback<bool(void)> OrderedSimpleTaskRunner::TaskExistedInitially() {
  std::set<size_t> task_ids;
  for (const auto& task : pending_tasks_)
    task_ids.insert(task.task_id());

  return base::Bind(&OrderedSimpleTaskRunner::TaskExistedInitiallyCallback,
                    base::Unretained(this), std::move(task_ids));
}

bool OrderedSimpleTaskRunner::TaskExistedInitiallyCallback(
    const std::set<size_t>& existing_tasks) {
  return existing_tasks.find(pending_tasks_.begin()->task_id()) !=
         existing_tasks.end();
}

base::Callback<bool(void)> OrderedSimpleTaskRunner::NowBefore(
    base::TimeTicks stop_at) {
  return base::Bind(&OrderedSimpleTaskRunner::NowBeforeCallback,
                    base::Unretained(this), stop_at);
}
bool OrderedSimpleTaskRunner::NowBeforeCallback(base::TimeTicks stop_at) {
  return NextTaskTime() <= stop_at;
}

base::Callback<bool(void)> OrderedSimpleTaskRunner::AdvanceNow() {
  return base::Bind(&OrderedSimpleTaskRunner::AdvanceNowCallback,
                    base::Unretained(this));
}

bool OrderedSimpleTaskRunner::AdvanceNowCallback() {
  base::TimeTicks next_task_time = NextTaskTime();
  if (now_src_->NowTicks() < next_task_time) {
    now_src_->Advance(next_task_time - now_src_->NowTicks());
  }
  return true;
}

void OrderedSimpleTaskRunner::RemoveCancelledTasks() {
  std::set<TestOrderablePendingTask>::iterator it = pending_tasks_.begin();
  while (it != pending_tasks_.end()) {
    if (it->task.IsCancelled()) {
      it = pending_tasks_.erase(it);
    } else {
      it++;
    }
  }
}

}  // namespace cc
