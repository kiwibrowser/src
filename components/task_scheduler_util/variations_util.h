// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TASK_SCHEDULER_UTIL_VARIATIONS_UTIL_H_
#define COMPONENTS_TASK_SCHEDULER_UTIL_VARIATIONS_UTIL_H_

#include <memory>

#include "base/strings/string_piece.h"
#include "base/task_scheduler/task_scheduler.h"

namespace task_scheduler_util {

// Builds a TaskScheduler::InitParams from variations params that are prefixed
// with |variation_param_prefix| in the BrowserScheduler field trial. Returns
// nullptr on failure.
//
// TODO(fdoray): Move this to the anonymous namespace in the .cc file.
// https://crbug.com/810049
std::unique_ptr<base::TaskScheduler::InitParams> GetTaskSchedulerInitParams(
    base::StringPiece variation_param_prefix);

// Builds a TaskScheduler::InitParams to use in the browser process from
// variation params in the BrowserScheduler field trial.
std::unique_ptr<base::TaskScheduler::InitParams>
GetTaskSchedulerInitParamsForBrowser();

// Builds a TaskScheduler::InitParams to use in renderer processes from
// variation params in the BrowserScheduler field trial.
std::unique_ptr<base::TaskScheduler::InitParams>
GetTaskSchedulerInitParamsForRenderer();

}  // namespace task_scheduler_util

#endif  // COMPONENTS_TASK_SCHEDULER_UTIL_VARIATIONS_UTIL_H_
