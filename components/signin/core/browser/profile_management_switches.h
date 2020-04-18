// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These are functions to access various profile-management flags but with
// possible overrides from Experiements.  This is done inside chrome/common
// because it is accessed by files through the chrome/ directory tree.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_MANAGEMENT_SWITCHES_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_MANAGEMENT_SWITCHES_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/feature_list.h"
#include "components/prefs/pref_member.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace signin {

// Account consistency feature. Only used on platforms where Mirror is not
// always enabled (ENABLE_MIRROR is false).
extern const base::Feature kAccountConsistencyFeature;

// The account consistency method feature parameter name.
extern const char kAccountConsistencyFeatureMethodParameter[];

// Account consistency method feature values.
extern const char kAccountConsistencyFeatureMethodMirror[];
extern const char kAccountConsistencyFeatureMethodDiceFixAuthErrors[];
extern const char kAccountConsistencyFeatureMethodDicePrepareMigration[];
extern const char kAccountConsistencyFeatureMethodDiceMigration[];
extern const char kAccountConsistencyFeatureMethodDice[];

// Improved and unified consent for privacy-related features.
extern const base::Feature kUnifiedConsent;
extern const char kUnifiedConsentShowBumpParameter[];

// State of the "Unified Consent" feature.
enum class UnifiedConsentFeatureState {
  // Unified consent is disabled.
  kDisabled,
  // Unified consent is enabled, but the bump is not shown.
  kEnabledNoBump,
  // Unified consent is enabled and the bump is shown.
  kEnabledWithBump
};

// TODO(https://crbug.com/777774): Cleanup this enum and remove related
// functions once Dice is fully rolled out, and/or Mirror code is removed on
// desktop.
enum class AccountConsistencyMethod : int {
  // No account consistency.
  kDisabled,

  // Account management UI in the avatar bubble.
  kMirror,

  // No account consistency, but Dice fixes authentication errors.
  kDiceFixAuthErrors,

  // Chrome uses the Dice signin flow and silently collects tokens associated
  // with Gaia cookies to prepare for the migration. Uses the Chrome sync Gaia
  // endpoint to enable sync.
  kDicePrepareMigration,

  // Account management UI on Gaia webpages is enabled once the accounts become
  // consistent.
  kDiceMigration,

  // Account management UI on Gaia webpages is enabled. If accounts are not
  // consistent when this is enabled, the account reconcilor enforces the
  // consistency.
  kDice
};

// Returns true if the |a| comes after |b| in the AccountConsistencyMethod enum.
// Should not be used for Mirror.
bool DiceMethodGreaterOrEqual(AccountConsistencyMethod a,
                              AccountConsistencyMethod b);

////////////////////////////////////////////////////////////////////////////////
// AccountConsistencyMethod related functions:

// WARNING: DEPRECATED. These methods are global, but account consistency is per
// profile.

// Returns the account consistency method.
AccountConsistencyMethod GetAccountConsistencyMethod();

// Checks whether Mirror account consistency is enabled. If enabled, the account
// management UI is available in the avatar bubble.
bool IsAccountConsistencyMirrorEnabled();

// Returns true if the account consistency method is kDiceFixAuthErrors or
// greater.
bool IsDiceFixAuthErrorsEnabled();

// Returns true if the account consistency method is
// kDicePrepareMigration or greater.
bool IsDicePrepareMigrationEnabled();

// Returns true if Dice account consistency is enabled or if the Dice migration
// process is in progress (account consistency method is kDice or
// kDiceMigration).
// To check wether Dice is enabled (i.e. the migration is complete), use
// IsDiceEnabledForProfile().
bool IsDiceMigrationEnabled();

////////////////////////////////////////////////////////////////////////////////
// Functions to test if Dice is enabled for a user profile:

// If true, then account management is done through Gaia webpages.
// Can only be used on the UI thread.
bool IsDiceEnabledForProfile(const PrefService* user_prefs);

// If true, then account management is done through Gaia webpages.
// Can be called on any thread, using a pref member obtained with
// CreateDicePrefMember().
// On the UI thread, consider using IsDiceEnabledForProfile() instead.
// Example usage:
//
// // On UI thread:
// std::unique_ptr<BooleanPrefMember> pref_member = GetDicePrefMember(prefs);
// pref_member->MoveToThread(io_thread);
//
// // Later, on IO thread:
// bool dice_enabled = GetDicePrefMember(pref_member.get());
bool IsDiceEnabled(const BooleanPrefMember* dice_pref_member);

// Gets a pref member suitable to use with IsDiceEnabled().
std::unique_ptr<BooleanPrefMember> CreateDicePrefMember(
    PrefService* user_prefs);

// Called to migrate a profile to Dice. After this call, it is enabled forever.
void MigrateProfileToDice(PrefService* user_prefs);

////////////////////////////////////////////////////////////////////////////////
// Other functions:

// Register account consistency user preferences.
void RegisterAccountConsistencyProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry);

// Whether the chrome.identity API should be multi-account.
bool IsExtensionsMultiAccount();

// |is_gaia_isolated| callback returns whether Gaia origin is isolated, which is
// a requirement for kDicePrepareMigration and later Dice steps.
void SetGaiaOriginIsolatedCallback(
    const base::RepeatingCallback<bool()>& is_gaia_isolated);

// Returns the state of the "Unified Consent" feature.
UnifiedConsentFeatureState GetUnifiedConsentFeatureState();

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_MANAGEMENT_SWITCHES_H_
