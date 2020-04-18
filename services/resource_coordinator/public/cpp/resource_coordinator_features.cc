// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

#include <string>

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"

namespace {

constexpr char kUkmPageLoadCPUUsageProfilingTrialName[] =
    "UkmPageLoadCPUUsageProfiling";
constexpr char kIntervalInMsParameterName[] = "intervalInMs";
constexpr char kDurationInMsParameterName[] = "durationInMs";
constexpr char kMainThreadTaskLoadLowThresholdParameterName[] =
    "mainThreadTaskLoadLowThreshold";

int64_t GetIntegerFieldTrialParam(const std::string& trial_name,
                                  const std::string& parameter_name,
                                  int64_t default_val) {
  std::string parameter_str =
      base::GetFieldTrialParamValue(trial_name, parameter_name);

  int64_t parameter_value;
  if (parameter_str.empty() ||
      !base::StringToInt64(parameter_str, &parameter_value)) {
    return default_val;
  }

  return parameter_value;
}

}  // namespace

namespace features {

// Globally enable the GRC.
const base::Feature kGlobalResourceCoordinator{
    "GlobalResourceCoordinator", base::FEATURE_ENABLED_BY_DEFAULT};

// Enable render process CPU profiling for GRC.
const base::Feature kGRCRenderProcessCPUProfiling{
    "GRCRenderProcessCPUProfiling", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kPageAlmostIdle{"PageAlmostIdle",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features

namespace resource_coordinator {

bool IsResourceCoordinatorEnabled() {
  return base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

bool IsGRCRenderProcessCPUProfilingEnabled() {
  return base::FeatureList::IsEnabled(features::kGRCRenderProcessCPUProfiling);
}

int64_t GetGRCRenderProcessCPUProfilingDurationInMs() {
  return GetIntegerFieldTrialParam(kUkmPageLoadCPUUsageProfilingTrialName,
                                   kDurationInMsParameterName, -1);
}

int64_t GetGRCRenderProcessCPUProfilingIntervalInMs() {
  return GetIntegerFieldTrialParam(kUkmPageLoadCPUUsageProfilingTrialName,
                                   kIntervalInMsParameterName, -1);
}

bool IsPageAlmostIdleSignalEnabled() {
  return base::FeatureList::IsEnabled(features::kPageAlmostIdle);
}

int GetMainThreadTaskLoadLowThreshold() {
  static const int kDefaultThreshold = 30;

  std::string value_str = base::GetFieldTrialParamValueByFeature(
      features::kPageAlmostIdle, kMainThreadTaskLoadLowThresholdParameterName);
  int main_thread_task_load_low_threshold;
  if (value_str.empty() ||
      !base::StringToInt(value_str, &main_thread_task_load_low_threshold)) {
    return kDefaultThreshold;
  }
  return main_thread_task_load_low_threshold;
}

}  // namespace resource_coordinator
