// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_bubble_experiment.h"

#include <string>

#include "base/metrics/field_trial.h"
#include "base/strings/string_number_conversions.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/variations/variations_associated_data.h"

namespace password_bubble_experiment {

const char kSmartBubbleExperimentName[] = "PasswordSmartBubble";
const char kSmartBubbleThresholdParam[] = "dismissal_count";

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(
      password_manager::prefs::kWasSignInPasswordPromoClicked, false);

  registry->RegisterIntegerPref(
      password_manager::prefs::kNumberSignInPasswordPromoShown, 0);
}

int GetSmartBubbleDismissalThreshold() {
  std::string param = variations::GetVariationParamValue(
      kSmartBubbleExperimentName, kSmartBubbleThresholdParam);
  int threshold = 0;
  // 3 is the default magic number that proved to show the best result.
  return base::StringToInt(param, &threshold) ? threshold : 3;
}

bool IsSmartLockUser(const syncer::SyncService* sync_service) {
  return password_manager_util::GetPasswordSyncState(sync_service) ==
         password_manager::SYNCING_NORMAL_ENCRYPTION;
}

bool ShouldShowAutoSignInPromptFirstRunExperience(PrefService* prefs) {
  return !prefs->GetBoolean(
      password_manager::prefs::kWasAutoSignInFirstRunExperienceShown);
}

void RecordAutoSignInPromptFirstRunExperienceWasShown(PrefService* prefs) {
  prefs->SetBoolean(
      password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, true);
}

void TurnOffAutoSignin(PrefService* prefs) {
  prefs->SetBoolean(password_manager::prefs::kCredentialsEnableAutosignin,
                    false);
}

bool ShouldShowChromeSignInPasswordPromo(
    PrefService* prefs,
    const syncer::SyncService* sync_service) {
  if (!sync_service || !sync_service->IsSyncAllowed() ||
      sync_service->IsFirstSetupComplete())
    return false;
  // Don't show the promo more than 3 times.
  constexpr int kThreshold = 3;
  return !prefs->GetBoolean(
             password_manager::prefs::kWasSignInPasswordPromoClicked) &&
         prefs->GetInteger(
             password_manager::prefs::kNumberSignInPasswordPromoShown) <
             kThreshold;
}

}  // namespace password_bubble_experiment
