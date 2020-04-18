// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/task_scheduler_util/variations_util.h"

#include <map>
#include <string>

#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/initialization_util.h"
#include "base/time/time.h"

namespace task_scheduler_util {

namespace {

// Builds a SchedulerWorkerPoolParams from the pool descriptor in
// |variation_params[variation_param_prefix + pool_name]|. Returns an invalid
// SchedulerWorkerPoolParams on failure.
//
// The pool descriptor is a semi-colon separated value string with the following
// items:
// 0. Minimum Thread Count (int)
// 1. Maximum Thread Count (int)
// 2. Thread Count Multiplier (double)
// 3. Thread Count Offset (int)
// 4. Detach Time in Milliseconds (int)
// Additional values may appear as necessary and will be ignored.
std::unique_ptr<base::SchedulerWorkerPoolParams> GetWorkerPoolParams(
    base::StringPiece variation_param_prefix,
    base::StringPiece pool_name,
    const std::map<std::string, std::string>& variation_params) {
  auto pool_descriptor_it =
      variation_params.find(base::StrCat({variation_param_prefix, pool_name}));
  if (pool_descriptor_it == variation_params.end())
    return nullptr;
  const auto& pool_descriptor = pool_descriptor_it->second;

  const std::vector<base::StringPiece> tokens = SplitStringPiece(
      pool_descriptor, ";", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  // Normally, we wouldn't initialize the values below because we don't read
  // from them before we write to them. However, some compilers (like MSVC)
  // complain about uninitialized variables due to the as_string() call below.
  int min = 0;
  int max = 0;
  double cores_multiplier = 0.0;
  int offset = 0;
  int detach_milliseconds = 0;
  // Checking for a size greater than the expected amount allows us to be
  // forward compatible if we add more variation values.
  if (tokens.size() < 5 || !base::StringToInt(tokens[0], &min) ||
      !base::StringToInt(tokens[1], &max) ||
      !base::StringToDouble(tokens[2].as_string(), &cores_multiplier) ||
      !base::StringToInt(tokens[3], &offset) ||
      !base::StringToInt(tokens[4], &detach_milliseconds)) {
    DLOG(ERROR) << "Invalid Worker Pool Descriptor Format: " << pool_descriptor;
    return nullptr;
  }

  auto params = std::make_unique<base::SchedulerWorkerPoolParams>(
      base::RecommendedMaxNumberOfThreadsInPool(min, max, cores_multiplier,
                                                offset),
      base::TimeDelta::FromMilliseconds(detach_milliseconds));

  if (params->max_threads() <= 0) {
    DLOG(ERROR) << "Invalid max threads in the Worker Pool Descriptor: "
                << params->max_threads();
    return nullptr;
  }

  if (params->suggested_reclaim_time() < base::TimeDelta()) {
    DLOG(ERROR)
        << "Invalid suggested reclaim time in the Worker Pool Descriptor:"
        << params->suggested_reclaim_time();
    return nullptr;
  }

  return params;
}

}  // namespace

std::unique_ptr<base::TaskScheduler::InitParams> GetTaskSchedulerInitParams(
    base::StringPiece variation_param_prefix) {
  std::map<std::string, std::string> variation_params;
  if (!base::GetFieldTrialParams("BrowserScheduler", &variation_params))
    return nullptr;

  const auto background_worker_pool_params = GetWorkerPoolParams(
      variation_param_prefix, "Background", variation_params);
  const auto background_blocking_worker_pool_params = GetWorkerPoolParams(
      variation_param_prefix, "BackgroundBlocking", variation_params);
  const auto foreground_worker_pool_params = GetWorkerPoolParams(
      variation_param_prefix, "Foreground", variation_params);
  const auto foreground_blocking_worker_pool_params = GetWorkerPoolParams(
      variation_param_prefix, "ForegroundBlocking", variation_params);

  if (!background_worker_pool_params ||
      !background_blocking_worker_pool_params ||
      !foreground_worker_pool_params ||
      !foreground_blocking_worker_pool_params) {
    return nullptr;
  }

  return std::make_unique<base::TaskScheduler::InitParams>(
      *background_worker_pool_params, *background_blocking_worker_pool_params,
      *foreground_worker_pool_params, *foreground_blocking_worker_pool_params);
}

std::unique_ptr<base::TaskScheduler::InitParams>
GetTaskSchedulerInitParamsForBrowser() {
  // Variations params for the browser processes have no prefix.
  constexpr char kVariationParamPrefix[] = "";
  return GetTaskSchedulerInitParams(kVariationParamPrefix);
}

std::unique_ptr<base::TaskScheduler::InitParams>
GetTaskSchedulerInitParamsForRenderer() {
  // Variations params for renderer processes are prefixed with "Renderer".
  constexpr char kVariationParamPrefix[] = "Renderer";
  return GetTaskSchedulerInitParams(kVariationParamPrefix);
}

}  // namespace task_scheduler_util
