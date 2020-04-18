// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/one_shot_event.h"

#include <stddef.h>

#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

using base::SingleThreadTaskRunner;

namespace extensions {

struct OneShotEvent::TaskInfo {
  TaskInfo() {}
  TaskInfo(const base::Location& from_here,
           const scoped_refptr<SingleThreadTaskRunner>& runner,
           const base::Closure& task,
           const base::TimeDelta& delay)
      : from_here(from_here), runner(runner), task(task), delay(delay) {
    CHECK(runner.get());  // Detect mistakes with a decent stack frame.
  }
  base::Location from_here;
  scoped_refptr<SingleThreadTaskRunner> runner;
  base::Closure task;
  base::TimeDelta delay;
};

OneShotEvent::OneShotEvent() : signaled_(false) {
  // It's acceptable to construct the OneShotEvent on one thread, but
  // immediately move it to another thread.
  thread_checker_.DetachFromThread();
}
OneShotEvent::OneShotEvent(bool signaled) : signaled_(signaled) {
  thread_checker_.DetachFromThread();
}
OneShotEvent::~OneShotEvent() {}

void OneShotEvent::Post(const base::Location& from_here,
                        const base::Closure& task) const {
  PostImpl(from_here, task, base::ThreadTaskRunnerHandle::Get(),
           base::TimeDelta());
}

void OneShotEvent::Post(
    const base::Location& from_here,
    const base::Closure& task,
    const scoped_refptr<SingleThreadTaskRunner>& runner) const {
  PostImpl(from_here, task, runner, base::TimeDelta());
}

void OneShotEvent::PostDelayed(const base::Location& from_here,
                               const base::Closure& task,
                               const base::TimeDelta& delay) const {
  PostImpl(from_here, task, base::ThreadTaskRunnerHandle::Get(), delay);
}

void OneShotEvent::Signal() {
  DCHECK(thread_checker_.CalledOnValidThread());

  CHECK(!signaled_) << "Only call Signal once.";

  signaled_ = true;
  // After this point, a call to Post() from one of the queued tasks
  // could proceed immediately, but the fact that this object is
  // single-threaded prevents that from being relevant.

  // We could randomize tasks_ in debug mode in order to check that
  // the order doesn't matter...
  for (size_t i = 0; i < tasks_.size(); ++i) {
    const TaskInfo& task = tasks_[i];
    if (task.delay.is_zero())
      task.runner->PostTask(task.from_here, task.task);
    else
      task.runner->PostDelayedTask(task.from_here, task.task, task.delay);
  }
}

void OneShotEvent::PostImpl(const base::Location& from_here,
                            const base::Closure& task,
                            const scoped_refptr<SingleThreadTaskRunner>& runner,
                            const base::TimeDelta& delay) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (is_signaled()) {
    if (delay.is_zero())
      runner->PostTask(from_here, task);
    else
      runner->PostDelayedTask(from_here, task, delay);
  } else {
    tasks_.push_back(TaskInfo(from_here, runner, task, delay));
  }
}

}  // namespace extensions
