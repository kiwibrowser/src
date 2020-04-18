// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_FIELD_TRIAL_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_FIELD_TRIAL_H_

#include "base/feature_list.h"

class Profile;

namespace prerender {

// These Finch feature and parameter strings exposed for for testing.
extern const base::Feature kNoStatePrefetchFeature;
extern const char kNoStatePrefetchFeatureModeParameterName[];
extern const char kNoStatePrefetchFeatureOmniboxModeParameterName[];
extern const char kNoStatePrefetchFeatureModeParameterPrefetch[];
extern const char kNoStatePrefetchFeatureModeParameterPrerender[];
extern const char kNoStatePrefetchFeatureModeParameterSimpleLoad[];
extern const char kNoStatePrefetchFeatureModeParameterDisabled[];

// Configures prerender using kNoStatePrefetchFeature and field trial
// parameters.
void ConfigurePrerender();

// Returns true if the user has opted in or has been opted in to the
// prerendering from Omnibox experiment.
bool IsOmniboxEnabled(Profile* profile);

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_FIELD_TRIAL_H_
