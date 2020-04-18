// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_prefs.h"

#include "base/base64.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "components/metrics/daily_event.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/public/rappor_parameters.h"
#include "components/rappor/rappor_pref_names.h"

namespace rappor {

namespace internal {

const char kLoadCohortHistogramName[] = "Rappor.LoadCohortResult";
const char kLoadSecretHistogramName[] = "Rappor.LoadSecretResult";

namespace {

void RecordLoadCohortResult(LoadResult reason) {
  UMA_HISTOGRAM_ENUMERATION(kLoadCohortHistogramName,
                            reason,
                            NUM_LOAD_RESULTS);
}

void RecordLoadSecretResult(LoadResult reason) {
  UMA_HISTOGRAM_ENUMERATION(kLoadSecretHistogramName,
                            reason,
                            NUM_LOAD_RESULTS);
}

} // namespace

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kRapporSecret, std::string());
  registry->RegisterIntegerPref(prefs::kRapporCohortDeprecated, -1);
  registry->RegisterIntegerPref(prefs::kRapporCohortSeed, -1);
  metrics::DailyEvent::RegisterPref(registry, prefs::kRapporLastDailySample);
}

int32_t LoadCohort(PrefService* pref_service) {
  // Ignore and delete old cohort parameter.
  pref_service->ClearPref(prefs::kRapporCohortDeprecated);

  int32_t cohort = pref_service->GetInteger(prefs::kRapporCohortSeed);
  // If the user is already assigned to a valid cohort, we're done.
  if (cohort >= 0 && cohort < RapporParameters::kMaxCohorts) {
    RecordLoadCohortResult(LOAD_SUCCESS);
    DVLOG(2) << "Rappor cohort loaded.";
    return cohort;
  }

  // This is the first time the client has started the service (or their
  // preferences were corrupted).  Randomly assign them to a cohort.
  RecordLoadCohortResult(cohort == -1 ? LOAD_EMPTY_VALUE : LOAD_CORRUPT_VALUE);
  cohort = base::RandGenerator(RapporParameters::kMaxCohorts);
  DVLOG(2) << "Selected a new Rappor cohort: " << cohort;
  pref_service->SetInteger(prefs::kRapporCohortSeed, cohort);
  return cohort;
}

std::string LoadSecret(PrefService* pref_service) {
  std::string secret;
  std::string secret_base64 = pref_service->GetString(prefs::kRapporSecret);
  if (!secret_base64.empty()) {
    bool decoded = base::Base64Decode(secret_base64, &secret);
    if (decoded &&
        secret.size() == HmacByteVectorGenerator::kEntropyInputSize) {
      DVLOG(2) << "Rappor secret loaded.";
      RecordLoadSecretResult(LOAD_SUCCESS);
      return secret;
    }
    // If the preference fails to decode, or is the wrong size, it must be
    // corrupt, so continue as though it didn't exist yet and generate a new
    // one.
    DVLOG(2) << "Corrupt Rappor secret found.";
    RecordLoadSecretResult(LOAD_CORRUPT_VALUE);
  } else {
    DVLOG(2) << "No Rappor secret found.";
    RecordLoadSecretResult(LOAD_EMPTY_VALUE);
  }

  DVLOG(2) << "Generated a new Rappor secret.";
  secret = HmacByteVectorGenerator::GenerateEntropyInput();
  base::Base64Encode(secret, &secret_base64);
  pref_service->SetString(prefs::kRapporSecret, secret_base64);
  return secret;
}

}  // namespace internal

}  // namespace rappor
