// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_RAPPOR_PREFS_H_
#define COMPONENTS_RAPPOR_RAPPOR_PREFS_H_

#include <stdint.h>

#include <string>


class PrefService;
class PrefRegistrySimple;

namespace rappor {

namespace internal {

enum LoadResult {
  LOAD_SUCCESS = 0,
  LOAD_EMPTY_VALUE,
  LOAD_CORRUPT_VALUE,
  NUM_LOAD_RESULTS,
};

extern const char kLoadCohortHistogramName[];
extern const char kLoadSecretHistogramName[];

// Registers all rappor preferences.
void RegisterPrefs(PrefRegistrySimple* registry);

// Retrieves the cohort number this client was assigned to, generating it if
// doesn't already exist. The cohort should be persistent.
int32_t LoadCohort(PrefService* pref_service);

// Retrieves the value for secret from preferences, generating it if doesn't
// already exist. The secret should be persistent, so that additional bits
// from the client do not get exposed over time.
std::string LoadSecret(PrefService* pref_service);

}  // namespace internal

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_RAPPOR_PREFS_H_
