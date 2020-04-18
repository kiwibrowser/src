// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_mode_manager.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "build/buildflag.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/testing_pref_store.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/common/content_features.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Checks that Dice migration happens when the reconcilor is created.
TEST(AccountConsistencyModeManagerTest, MigrateAtCreation) {
  content::TestBrowserThreadBundle test_thread_bundle;
  TestingProfile::Builder profile_builder;
  {
    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  ASSERT_FALSE(profile->IsNewProfile());
  PrefService* prefs = profile->GetPrefs();

  {
    // Migration does not happen if SetDiceMigrationOnStartup() is not called.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyModeManager manager(profile.get());
    EXPECT_FALSE(manager.IsReadyForDiceMigration());
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
  }

  AccountConsistencyModeManager::SetDiceMigrationOnStartup(prefs, true);

  {
    // Migration does not happen if Dice is not enabled.
    signin::ScopedAccountConsistencyDiceFixAuthErrors scoped_dice_fix_errors;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyModeManager manager(profile.get());
    EXPECT_TRUE(manager.IsReadyForDiceMigration());
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
  }

  {
    // Migration happens.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyModeManager manager(profile.get());
    EXPECT_TRUE(manager.IsReadyForDiceMigration());
    EXPECT_TRUE(signin::IsDiceEnabledForProfile(prefs));
  }
}

// Checks that new profiles are migrated at creation.
TEST(AccountConsistencyModeManagerTest, NewProfile) {
  content::TestBrowserThreadBundle test_thread_bundle;
  base::test::ScopedFeatureList scoped_site_isolation;
  scoped_site_isolation.InitAndEnableFeature(features::kSignInProcessIsolation);
  signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
  TestingProfile::Builder profile_builder;
  {
    TestingPrefStore* user_prefs = new TestingPrefStore();

    // Set the read error so that Profile::IsNewProfile() returns true.
    user_prefs->set_read_error(PersistentPrefStore::PREF_READ_ERROR_NO_FILE);

    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>(
            new TestingPrefStore(), new TestingPrefStore(), user_prefs,
            new TestingPrefStore(), new user_prefs::PrefRegistrySyncable(),
            new PrefNotifierImpl());
    RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  ASSERT_TRUE(profile->IsNewProfile());
  EXPECT_TRUE(signin::IsDiceEnabledForProfile(profile->GetPrefs()));
}

TEST(AccountConsistencyModeManagerTest, DiceOnlyForRegularProfile) {
  signin::ScopedAccountConsistencyDice scoped_dice;
  content::TestBrowserThreadBundle test_thread_bundle;

  {
    // Regular profile.
    TestingProfile profile;
    EXPECT_TRUE(
        AccountConsistencyModeManager::IsDiceEnabledForProfile(&profile));
    EXPECT_EQ(signin::AccountConsistencyMethod::kDice,
              AccountConsistencyModeManager::GetMethodForProfile(&profile));

    // Incognito profile.
    Profile* incognito_profile = profile.GetOffTheRecordProfile();
    EXPECT_FALSE(AccountConsistencyModeManager::IsDiceEnabledForProfile(
        incognito_profile));
    EXPECT_FALSE(
        AccountConsistencyModeManager::GetForProfile(incognito_profile));
    EXPECT_EQ(
        signin::AccountConsistencyMethod::kDiceFixAuthErrors,
        AccountConsistencyModeManager::GetMethodForProfile(incognito_profile));
  }

  {
    // Guest profile.
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    std::unique_ptr<Profile> profile = profile_builder.Build();
    ASSERT_TRUE(profile->IsGuestSession());
    EXPECT_FALSE(
        AccountConsistencyModeManager::IsDiceEnabledForProfile(profile.get()));
    EXPECT_EQ(
        signin::AccountConsistencyMethod::kDiceFixAuthErrors,
        AccountConsistencyModeManager::GetMethodForProfile(profile.get()));
  }

  {
    // Legacy supervised profile.
    TestingProfile::Builder profile_builder;
    profile_builder.SetSupervisedUserId("supervised_id");
    std::unique_ptr<Profile> profile = profile_builder.Build();
    ASSERT_TRUE(profile->IsLegacySupervised());
    EXPECT_FALSE(
        AccountConsistencyModeManager::IsDiceEnabledForProfile(profile.get()));
    EXPECT_EQ(
        signin::AccountConsistencyMethod::kDiceFixAuthErrors,
        AccountConsistencyModeManager::GetMethodForProfile(profile.get()));
  }
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

#if defined(OS_CHROMEOS)
TEST(AccountConsistencyModeManagerTest, MirrorDisabledForNonUnicorn) {
  // Creation of this object sets the current thread's id as UI thread.
  content::TestBrowserThreadBundle test_thread_bundle;

  TestingProfile profile;
  EXPECT_FALSE(
      AccountConsistencyModeManager::IsMirrorEnabledForProfile(&profile));
  EXPECT_EQ(signin::AccountConsistencyMethod::kDisabled,
            AccountConsistencyModeManager::GetMethodForProfile(&profile));
}

TEST(AccountConsistencyModeManagerTest, MirrorEnabledByPreference) {
  // Creation of this object sets the current thread's id as UI thread.
  content::TestBrowserThreadBundle test_thread_bundle;

  TestingProfile::Builder profile_builder;
  {
    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  profile->GetPrefs()->SetBoolean(prefs::kAccountConsistencyMirrorRequired,
                                  true);

  EXPECT_TRUE(
      AccountConsistencyModeManager::IsMirrorEnabledForProfile(profile.get()));
  EXPECT_EQ(signin::AccountConsistencyMethod::kMirror,
            AccountConsistencyModeManager::GetMethodForProfile(profile.get()));
}
#endif  // defined(OS_CHROMEOS)

#if BUILDFLAG(ENABLE_MIRROR)
TEST(AccountConsistencyModeManagerTest, MirrorEnabled) {
  // Creation of this object sets the current thread's id as UI thread.
  content::TestBrowserThreadBundle test_thread_bundle;

  // Test that Mirror is enabled for regular accounts.
  TestingProfile profile;
  EXPECT_TRUE(
      AccountConsistencyModeManager::IsMirrorEnabledForProfile(&profile));
  EXPECT_EQ(signin::AccountConsistencyMethod::kMirror,
            AccountConsistencyModeManager::GetMethodForProfile(&profile));

  // Test that Mirror is enabled for child accounts.
  profile.SetSupervisedUserId(supervised_users::kChildAccountSUID);
  EXPECT_TRUE(
      AccountConsistencyModeManager::IsMirrorEnabledForProfile(&profile));
  EXPECT_EQ(signin::AccountConsistencyMethod::kMirror,
            AccountConsistencyModeManager::GetMethodForProfile(&profile));
}
#endif  // BUILDFLAG(ENABLE_MIRROR)
