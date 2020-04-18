// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/origin_trial_prefs.h"

#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

// static
void OriginTrialPrefs::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kOriginTrialPublicKey, "");
  registry->RegisterListPref(prefs::kOriginTrialDisabledFeatures);
  registry->RegisterListPref(prefs::kOriginTrialDisabledTokens);
}
