// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TASK_RUNNER_H_
#define PLATFORM_API_TASK_RUNNER_H_

#include <functional>

#include "platform/api/time.h"

namespace openscreen {
namespace platform {

// A thread-safe API surface that allows for posting tasks. The underlying
// implementation may be single or multi-threaded, and all complication should
// be handled by either the implementation class or the TaskRunnerFactory
// method. It is the expectation of this API that the underlying impl gives
// the following guarantees:
// (1) Tasks shall not overlap in time/CPU.
// (2) Tasks shall run sequentially, e.g. posting task A then B implies
//     that A shall run before B.
// NOTE: we do not make any assumptions about what thread tasks shall run on.
class TaskRunner {
 public:
  using Task = std::function<void()>;

  virtual ~TaskRunner() = default;

  // Takes a Task that should be run at the first convenient time.
  virtual void PostTask(Task task) = 0;

  // Takes a Task that should be run no sooner than "delay" time from now. Note
  // that we do not guarantee it will run precisely "delay" later, merely that
  // it will run no sooner than "delay" time from now.
  virtual void PostTaskWithDelay(Task task, Clock::duration delay) = 0;
};
}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_TASK_RUNNER_H_
