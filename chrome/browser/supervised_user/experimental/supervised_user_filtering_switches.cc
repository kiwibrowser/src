// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/experimental/supervised_user_filtering_switches.h"

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {

enum class SafeSitesState {
  ENABLED,
  DISABLED,
  BLACKLIST_ONLY,
  ONLINE_CHECK_ONLY
};

const char kSafeSitesFieldTrialName[] = "SafeSites";

SafeSitesState GetState(const Profile* profile) {
  // SafeSites is only supported for child accounts.
  if (!profile->IsChild())
    return SafeSitesState::DISABLED;

  // Note: It's important to query the field trial state first, to ensure that
  // UMA reports the correct group.
  std::string trial_group =
      base::FieldTrialList::FindFullName(kSafeSitesFieldTrialName);

  if (!profile->GetPrefs()->GetBoolean(prefs::kSupervisedUserSafeSites))
    return SafeSitesState::DISABLED;

  // If no cmdline arg is specified, evaluate the field trial.
  if (trial_group == "Disabled")
    return SafeSitesState::DISABLED;
  if (trial_group == "BlacklistOnly")
    return SafeSitesState::BLACKLIST_ONLY;
  if (trial_group == "OnlineCheckOnly")
    return SafeSitesState::ONLINE_CHECK_ONLY;

  return SafeSitesState::ENABLED;
}

}  // namespace

namespace supervised_users {

bool IsSafeSitesBlacklistEnabled(const Profile* profile) {
  SafeSitesState state = GetState(profile);
  return state == SafeSitesState::ENABLED ||
         state == SafeSitesState::BLACKLIST_ONLY;
}

bool IsSafeSitesOnlineCheckEnabled(const Profile* profile) {
  SafeSitesState state = GetState(profile);
  return state == SafeSitesState::ENABLED ||
         state == SafeSitesState::ONLINE_CHECK_ONLY;
}

}  // namespace supervised_users
