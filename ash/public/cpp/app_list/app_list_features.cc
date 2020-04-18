// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/app_list_features.h"

#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace app_list {
namespace features {

const base::Feature kEnableAnswerCard{"EnableAnswerCard",
                                      base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kEnableAppShortcutSearch{"EnableAppShortcutSearch",
                                             base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kEnableBackgroundBlur{"EnableBackgroundBlur",
                                          base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kEnablePlayStoreAppSearch{"EnablePlayStoreAppSearch",
                                              base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kEnableHomeLauncher{"EnableHomeLauncher",
                                        base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kEnableSettingsShortcutSearch{
    "EnableSettingsShortcutSearch", base::FEATURE_DISABLED_BY_DEFAULT};

bool IsAnswerCardEnabled() {
  // Not using local static variable to allow tests to change this value.
  return base::FeatureList::IsEnabled(kEnableAnswerCard);
}

bool IsAppShortcutSearchEnabled() {
  return base::FeatureList::IsEnabled(kEnableAppShortcutSearch);
}

bool IsBackgroundBlurEnabled() {
  return base::FeatureList::IsEnabled(kEnableBackgroundBlur);
}

bool IsPlayStoreAppSearchEnabled() {
  // Not using local static variable to allow tests to change this value.
  return base::FeatureList::IsEnabled(kEnablePlayStoreAppSearch);
}

bool IsHomeLauncherEnabled() {
  return base::FeatureList::IsEnabled(kEnableHomeLauncher);
}

bool IsSettingsShortcutSearchEnabled() {
  return base::FeatureList::IsEnabled(kEnableSettingsShortcutSearch);
}

std::string AnswerServerUrl() {
  const std::string experiment_url =
      base::GetFieldTrialParamValueByFeature(kEnableAnswerCard, "ServerUrl");
  if (!experiment_url.empty())
    return experiment_url;
  return "https://www.google.com/coac";
}

std::string AnswerServerQuerySuffix() {
  return base::GetFieldTrialParamValueByFeature(kEnableAnswerCard,
                                                "QuerySuffix");
}

}  // namespace features
}  // namespace app_list
