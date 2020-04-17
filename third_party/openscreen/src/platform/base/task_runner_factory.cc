// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "platform/api/task_runner_factory.h"

#include "platform/api/logging.h"
#include "platform/base/task_runner_impl.h"

namespace openscreen {
namespace platform {

// static
std::unique_ptr<TaskRunner> TaskRunnerFactory::Create(
    platform::ClockNowFunctionPtr now_function) {
  return std::unique_ptr<TaskRunner>(new TaskRunnerImpl(now_function));
}

}  // namespace platform
}  // namespace openscreen
