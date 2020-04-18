// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MODE_MANAGER_H_
#define CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MODE_MANAGER_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "build/buildflag.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_buildflags.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class Profile;

// Manages the account consistency mode for each profile.
class AccountConsistencyModeManager : public KeyedService {
 public:
  // Returns the AccountConsistencyModeManager associated with this profile.
  // May return nullptr if there is none (e.g. in incognito).
  static AccountConsistencyModeManager* GetForProfile(Profile* profile);

  explicit AccountConsistencyModeManager(Profile* profile);
  ~AccountConsistencyModeManager() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Helper method, shorthand for calling GetAccountConsistencyMethod().
  static signin::AccountConsistencyMethod GetMethodForProfile(Profile* profile);

  // Returns the account consistency method for the current profile. Can be
  // called from any thread, with a PrefMember created with
  // signin::CreateDicePrefMember().
  static signin::AccountConsistencyMethod GetMethodForPrefMember(
      BooleanPrefMember* dice_pref_member);

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  // Schedules migration to happen at next startup.
  void SetReadyForDiceMigration(bool is_ready);
#endif

  // If true, then account management is done through Gaia webpages.
  // Can only be used on the UI thread.
  // Returns false if |profile| is in Guest or Incognito mode.
  // A given |profile| will have only one of Mirror or Dice consistency
  // behaviour enabled.
  static bool IsDiceEnabledForProfile(Profile* profile);

  // Returns |true| if Mirror account consistency is enabled for |profile|.
  // Can only be used on the UI thread.
  // A given |profile| will have only one of Mirror or Dice consistency
  // behaviour enabled.
  static bool IsMirrorEnabledForProfile(Profile* profile);

  // By default, Deskotp Identity Consistency (aka Dice) is not enabled in
  // builds lacking an API key. For testing, set to have Dice enabled in tests.
  static void SetIgnoreMissingApiKeysForTesting();

 private:
  FRIEND_TEST_ALL_PREFIXES(AccountConsistencyModeManagerTest,
                           MigrateAtCreation);

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  // Schedules migration to happen at next startup. Exposed as a static function
  // for testing.
  static void SetDiceMigrationOnStartup(PrefService* prefs, bool migrate);

  // Returns true if migration can happen on the next startup.
  bool IsReadyForDiceMigration();
#endif

  // Returns the account consistency method for the current profile.
  signin::AccountConsistencyMethod GetAccountConsistencyMethod();

  Profile* profile_;

  // By default, DICE is not enabled in builds lacking an API key. Set to true
  // for tests.
  static bool ignore_missing_key_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(AccountConsistencyModeManager);
};

#endif  // CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_MODE_MANAGER_H_
