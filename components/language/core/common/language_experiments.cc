// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/common/language_experiments.h"

#include <map>
#include <string>

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"

namespace language {
// Features:
const base::Feature kUseHeuristicLanguageModel{
    "UseHeuristicLanguageModel", base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kOverrideTranslateTriggerInIndia{
    "OverrideTranslateTriggerInIndia", base::FEATURE_DISABLED_BY_DEFAULT};

// Params:
const char kBackoffThresholdKey[] = "backoff_threshold";
const char kOverrideModelKey[] = "override_model";
const char kEnforceRankerKey[] = "enforce_ranker";
const char kOverrideModelHeuristicValue[] = "heuristic";
const char kOverrideModelGeoValue[] = "geo";

OverrideLanguageModel GetOverrideLanguageModel() {
  std::map<std::string, std::string> params;
  bool should_override_model = base::GetFieldTrialParamsByFeature(
      kOverrideTranslateTriggerInIndia, &params);

  if (base::FeatureList::IsEnabled(kUseHeuristicLanguageModel) ||
      (should_override_model &&
       params[kOverrideModelKey] == kOverrideModelHeuristicValue)) {
    return OverrideLanguageModel::HEURISTIC;
  }

  if (should_override_model &&
      params[kOverrideModelKey] == kOverrideModelGeoValue) {
    return OverrideLanguageModel::GEO;
  }

  return OverrideLanguageModel::DEFAULT;
}

bool ShouldForceTriggerTranslateOnEnglishPages(int force_trigger_count) {
  return base::FeatureList::IsEnabled(kOverrideTranslateTriggerInIndia) &&
         !IsForceTriggerBackoffThresholdReached(force_trigger_count);
}

bool ShouldPreventRankerEnforcementInIndia(int force_trigger_count) {
  std::map<std::string, std::string> params;
  return ShouldForceTriggerTranslateOnEnglishPages(force_trigger_count) &&
         base::GetFieldTrialParamsByFeature(kOverrideTranslateTriggerInIndia,
                                            &params) &&
         params[kEnforceRankerKey] == "false";
}

bool IsForceTriggerBackoffThresholdReached(int force_trigger_count) {
  int threshold;
  std::map<std::string, std::string> params;
  if (!base::GetFieldTrialParamsByFeature(kOverrideTranslateTriggerInIndia,
                                          &params) ||
      !base::StringToInt(params[kBackoffThresholdKey], &threshold)) {
    return false;
  }

  return force_trigger_count >= threshold;
}

}  // namespace language
