// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/loading_predictor_config.h"

#include <string>

#include "base/metrics/field_trial_params.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/predictors/resource_prefetch_common.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

namespace predictors {

namespace {

bool IsPreconnectEnabledInternal(Profile* profile, int mode, int mask) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if ((mode & mask) == 0)
    return false;

  if (!profile || !profile->GetPrefs() ||
      !chrome_browser_net::CanPreresolveAndPreconnectUI(profile->GetPrefs())) {
    return false;
  }

  return true;
}

}  // namespace

const char kSpeculativePreconnectFeatureName[] = "SpeculativePreconnect";
const char kModeParamName[] = "mode";
const char kLearningMode[] = "learning";
const char kPreconnectMode[] = "preconnect";
const char kNoPreconnectMode[] = "no-preconnect";

const base::Feature kSpeculativePreconnectFeature{
  kSpeculativePreconnectFeatureName,
// TODO(https://crbug.com/839886): Enable the feature on ChromeOS after disk I/O
// flakes are fixed.
#if defined(OS_CHROMEOS)
      base::FEATURE_DISABLED_BY_DEFAULT
#else
      base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

bool MaybeEnableSpeculativePreconnect(LoadingPredictorConfig* config) {
  if (!base::FeatureList::IsEnabled(kSpeculativePreconnectFeature))
    return false;

  std::string mode_value = base::GetFieldTrialParamValueByFeature(
      kSpeculativePreconnectFeature, kModeParamName);
  if (mode_value.empty())
    mode_value = kPreconnectMode;

  if (mode_value == kLearningMode) {
    if (config) {
      config->mode |= LoadingPredictorConfig::LEARNING;
      config->is_origin_learning_enabled = true;
    }
    return true;
  } else if (mode_value == kPreconnectMode) {
    if (config) {
      config->mode |=
          LoadingPredictorConfig::LEARNING | LoadingPredictorConfig::PRECONNECT;
      config->is_origin_learning_enabled = true;
      config->should_disable_other_preconnects = true;
    }
    return true;
  } else if (mode_value == kNoPreconnectMode) {
    if (config) {
      config->should_disable_other_preconnects = true;
    }
    return false;
  }

  return false;
}

bool ShouldDisableOtherPreconnects() {
  LoadingPredictorConfig config;
  MaybeEnableSpeculativePreconnect(&config);
  return config.should_disable_other_preconnects;
}

bool IsLoadingPredictorEnabled(Profile* profile,
                               LoadingPredictorConfig* config) {
  // Disabled for of-the-record. Policy choice, not a technical limitation.
  if (!profile || profile->IsOffTheRecord())
    return false;

  // Compute both statements because they have side effects.
  bool preconnect_enabled = MaybeEnableSpeculativePreconnect(config);
  return preconnect_enabled;
}

LoadingPredictorConfig::LoadingPredictorConfig()
    : mode(0),
      max_navigation_lifetime_seconds(60),
      max_hosts_to_track(chrome_browser_net::Predictor::kMaxReferrers),
      max_origins_per_entry(50),
      max_consecutive_misses(3),
      max_redirect_consecutive_misses(5),
      is_origin_learning_enabled(false),
      should_disable_other_preconnects(false),
      flush_data_to_disk_delay_seconds(30) {}

LoadingPredictorConfig::LoadingPredictorConfig(
    const LoadingPredictorConfig& other) = default;

LoadingPredictorConfig::~LoadingPredictorConfig() = default;

bool LoadingPredictorConfig::IsLearningEnabled() const {
  return (mode & LEARNING) > 0;
}

bool LoadingPredictorConfig::IsPreconnectEnabledForSomeOrigin(
    Profile* profile) const {
  return IsPreconnectEnabledInternal(profile, mode, PRECONNECT);
}

bool LoadingPredictorConfig::IsPreconnectEnabledForOrigin(
    Profile* profile,
    HintOrigin origin) const {
  return IsPreconnectEnabledInternal(profile, mode, PRECONNECT);
}

bool LoadingPredictorConfig::IsSmallDBEnabledForTest() const {
  return max_hosts_to_track == 100;
}

}  // namespace predictors
