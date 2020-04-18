// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_field_trial.h"

#include <string>

#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/common/chrome_switches.h"

namespace prerender {

namespace {

PrerenderManager::PrerenderManagerMode ParsePrerenderMode(
    const char* parameter_name,
    PrerenderManager::PrerenderManagerMode default_mode) {
  PrerenderManager::PrerenderManagerMode mode = default_mode;
  if (!base::FeatureList::IsEnabled(kNoStatePrefetchFeature)) {
    mode = PrerenderManager::PRERENDER_MODE_DISABLED;
  } else {
    std::string mode_value = base::GetFieldTrialParamValueByFeature(
        kNoStatePrefetchFeature, parameter_name);
    if (mode_value.empty()) {
      mode = default_mode;
    } else if (mode_value == kNoStatePrefetchFeatureModeParameterPrefetch) {
      mode = PrerenderManager::PRERENDER_MODE_NOSTATE_PREFETCH;
    } else if (mode_value == kNoStatePrefetchFeatureModeParameterPrerender) {
      mode = PrerenderManager::PRERENDER_MODE_ENABLED;
    } else if (mode_value == kNoStatePrefetchFeatureModeParameterSimpleLoad) {
      mode = PrerenderManager::PRERENDER_MODE_SIMPLE_LOAD_EXPERIMENT;
    } else if (mode_value == kNoStatePrefetchFeatureModeParameterDisabled) {
      mode = PrerenderManager::PRERENDER_MODE_DISABLED;
    } else {
      LOG(ERROR) << "Invalid prerender mode: " << mode_value << " for "
                 << parameter_name;
      LOG(ERROR) << "Using default mode " << default_mode << " for "
                 << parameter_name << "!";
      mode = default_mode;
    }
  }
  return mode;
}

}  // namespace

// NoStatePrefetch feature parameters, to control the PrerenderManager mode
// using the base::Feature API and field trials.

// The general prerender mode for most origins.
const char kNoStatePrefetchFeatureModeParameterName[] = "mode";

// The origins can have prerendering overridden differently than the rest of the
// experiment.
const char kNoStatePrefetchFeatureOmniboxModeParameterName[] = "omnibox_mode";

// Mode values.
const char kNoStatePrefetchFeatureModeParameterPrefetch[] = "no_state_prefetch";
const char kNoStatePrefetchFeatureModeParameterPrerender[] = "prerender";
const char kNoStatePrefetchFeatureModeParameterSimpleLoad[] = "simple_load";
const char kNoStatePrefetchFeatureModeParameterDisabled[] = "disabled";

const base::Feature kNoStatePrefetchFeature{"NoStatePrefetch",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

void ConfigurePrerender() {
  PrerenderManager::PrerenderManagerMode overall_mode =
      ParsePrerenderMode(kNoStatePrefetchFeatureModeParameterName,
                         PrerenderManager::PRERENDER_MODE_NOSTATE_PREFETCH);

  PrerenderManager::SetMode(overall_mode);
  PrerenderManager::SetOmniboxMode(ParsePrerenderMode(
      kNoStatePrefetchFeatureOmniboxModeParameterName, overall_mode));
}

bool IsOmniboxEnabled(Profile* profile) {
  if (!profile)
    return false;

  if (!PrerenderManager::IsAnyPrerenderingPossible())
    return false;

  return true;
}

}  // namespace prerender
