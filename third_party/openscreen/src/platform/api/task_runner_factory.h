// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TASK_RUNNER_FACTORY_H_
#define PLATFORM_API_TASK_RUNNER_FACTORY_H_

#include <memory>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"

namespace openscreen {
namespace platform {

class TaskRunnerFactory {
 public:
  // Creates an instantiated TaskRunner.
  static std::unique_ptr<TaskRunner> Create(
      platform::ClockNowFunctionPtr now_function);
};
}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_TASK_RUNNER_FACTORY_H_
