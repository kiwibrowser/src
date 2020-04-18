// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_
#define GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_

#include "v8/include/v8-platform.h"

namespace gin {

// A V8IdleTaskRunner is a task runner for running idle tasks.
// Idle tasks have an unbound argument which is bound to a deadline in
// (v8::Platform::MonotonicallyIncreasingTime) when they are run.
// The idle task is expected to complete by this deadline.
class GIN_EXPORT V8IdleTaskRunner {
 public:
  virtual void PostIdleTask(v8::IdleTask* task) = 0;

  virtual ~V8IdleTaskRunner() {}
};

}  // namespace gin

#endif  // GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_
