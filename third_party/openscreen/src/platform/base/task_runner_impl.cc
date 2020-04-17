// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "platform/base/task_runner_impl.h"

#include "platform/api/logging.h"

namespace openscreen {
namespace platform {

TaskRunnerImpl::TaskRunnerImpl(platform::ClockNowFunctionPtr now_function,
                               TaskWaiter* event_waiter,
                               Clock::duration waiter_timeout)
    : now_function_(now_function),
      is_running_(false),
      task_waiter_(event_waiter),
      waiter_timeout_(waiter_timeout) {}

TaskRunnerImpl::~TaskRunnerImpl() = default;

void TaskRunnerImpl::PostTask(Task task) {
  std::lock_guard<std::mutex> lock(task_mutex_);
  tasks_.push_back(std::move(task));
  if (task_waiter_) {
    task_waiter_->OnTaskPosted();
  } else {
    run_loop_wakeup_.notify_one();
  }
}

void TaskRunnerImpl::PostTaskWithDelay(Task task, Clock::duration delay) {
  std::lock_guard<std::mutex> lock(task_mutex_);
  delayed_tasks_.emplace(std::move(task), now_function_() + delay);
  if (task_waiter_) {
    task_waiter_->OnTaskPosted();
  } else {
    run_loop_wakeup_.notify_one();
  }
}

void TaskRunnerImpl::RunUntilStopped() {
  const bool was_running = is_running_.exchange(true);
  OSP_CHECK(!was_running);

  RunTasksUntilStopped();
}

void TaskRunnerImpl::RequestStopSoon() {
  const bool was_running = is_running_.exchange(false);

  if (was_running) {
    OSP_DVLOG << "Requesting stop...";
    if (task_waiter_) {
      task_waiter_->OnTaskPosted();
    } else {
      std::lock_guard<std::mutex> lock(task_mutex_);
      run_loop_wakeup_.notify_one();
    }
  }
}

void TaskRunnerImpl::RunUntilIdleForTesting() {
  ScheduleDelayedTasks();
  RunCurrentTasksForTesting();
}

void TaskRunnerImpl::RunCurrentTasksForTesting() {
  std::deque<Task> current_tasks;
  {
    // Unlike in the RunCurrentTasksBlocking method, here we just immediately
    // take the lock and drain the tasks_ queue. This allows tests to avoid
    // having to do any multithreading to interact with the queue.
    std::unique_lock<std::mutex> lock(task_mutex_);
    tasks_.swap(current_tasks);
  }

  for (Task& task : current_tasks) {
    task();
  }
}

void TaskRunnerImpl::RunCurrentTasksBlocking() {
  std::deque<Task> current_tasks;
  {
    // Wait for the lock. If there are no current tasks, we will wait until
    // a delayed task is ready or a task gets added to the queue.
    auto lock = WaitForWorkAndAcquireLock();
    if (!lock) {
      return;
    }

    tasks_.swap(current_tasks);
  }

  for (Task& task : current_tasks) {
    OSP_DVLOG << "Running " << current_tasks.size() << " current tasks...";
    task();
  }
}

void TaskRunnerImpl::RunTasksUntilStopped() {
  while (is_running_) {
    ScheduleDelayedTasks();
    RunCurrentTasksBlocking();
  }
}

void TaskRunnerImpl::ScheduleDelayedTasks() {
  std::lock_guard<std::mutex> lock(task_mutex_);

  // Getting the time can be expensive on some platforms, so only get it once.
  const auto current_time = now_function_();
  while (!delayed_tasks_.empty() &&
         (delayed_tasks_.top().runnable_after <= current_time)) {
    tasks_.push_back(std::move(delayed_tasks_.top().task));
    delayed_tasks_.pop();
  }
}

bool TaskRunnerImpl::ShouldWakeUpRunLoop() {
  if (!is_running_) {
    return true;
  }

  if (!tasks_.empty()) {
    return true;
  }

  return !delayed_tasks_.empty() &&
         (delayed_tasks_.top().runnable_after <= now_function_());
}

std::unique_lock<std::mutex> TaskRunnerImpl::WaitForWorkAndAcquireLock() {
  // These checks are redundant, as they are a subset of predicates in the
  // below wait predicate. However, this is more readable and a slight
  // optimization, as we don't need to take a lock if we aren't running.
  if (!is_running_) {
    OSP_DVLOG << "TaskRunner not running. Returning empty lock.";
    return {};
  }

  std::unique_lock<std::mutex> lock(task_mutex_);
  if (!tasks_.empty()) {
    OSP_DVLOG << "TaskRunner lock acquired.";
    return lock;
  }

  if (task_waiter_) {
    do {
      Clock::duration timeout = waiter_timeout_;
      if (!delayed_tasks_.empty()) {
        Clock::duration next_task_delta =
            delayed_tasks_.top().runnable_after - now_function_();
        if (next_task_delta < timeout) {
          timeout = next_task_delta;
        }
      }
      lock.unlock();
      task_waiter_->WaitForTaskToBePosted(timeout);
      lock.lock();
    } while (!ShouldWakeUpRunLoop());
  } else {
    // Pass a wait predicate to avoid lost or spurious wakeups.
    if (!delayed_tasks_.empty()) {
      // We don't have any work to do currently, but have some in the
      // pipe.
      const auto wait_predicate = [this] { return ShouldWakeUpRunLoop(); };
      OSP_DVLOG << "TaskRunner waiting for lock until delayed task ready...";
      run_loop_wakeup_.wait_for(
          lock, delayed_tasks_.top().runnable_after - now_function_(),
          wait_predicate);
    } else {
      // We don't have any work queued.
      const auto wait_predicate = [this] {
        return !delayed_tasks_.empty() || ShouldWakeUpRunLoop();
      };
      OSP_DVLOG << "TaskRunner waiting for lock...";
      run_loop_wakeup_.wait(lock, wait_predicate);
    }
  }

  OSP_DVLOG << "TaskRunner lock acquired.";
  return lock;
}
}  // namespace platform
}  // namespace openscreen
