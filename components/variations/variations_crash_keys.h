// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_VARIATIONS_CRASH_KEYS_H_
#define COMPONENTS_VARIATIONS_VARIATIONS_CRASH_KEYS_H_

#include <vector>

namespace variations {

struct SyntheticTrialGroup;

// Initializes crash keys that report the current set of active FieldTrial
// groups (aka variations) for crash reports. After initialization, an observer
// will be registered on FieldTrialList that will keep the crash keys up-to-date
// with newly-activated trials. Synthetic trials must be manually updated using
// the API below.
void InitCrashKeys();

// Updates variations crash keys by replacing the list of synthetic trials with
// the specified list. Does not affect non-synthetic trials.
void UpdateCrashKeysWithSyntheticTrials(
    const std::vector<SyntheticTrialGroup>& synthetic_trials);

// Clears the internal instance, for testing.
void ClearCrashKeysInstanceForTesting();

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_VARIATIONS_CRASH_KEYS_H_
