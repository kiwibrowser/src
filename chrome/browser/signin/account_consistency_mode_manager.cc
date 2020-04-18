// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_mode_manager.h"

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/google_api_keys.h"

namespace {

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Preference indicating that the Dice migration should happen at the next
// Chrome startup.
const char kDiceMigrationOnStartupPref[] =
    "signin.AccountReconcilor.kDiceMigrationOnStartup2";

const char kDiceMigrationStatusHistogram[] = "Signin.DiceMigrationStatus";

// Used for UMA histogram kDiceMigrationStatusHistogram.
// Do not remove or re-order values.
enum class DiceMigrationStatus {
  kEnabled,
  kDisabledReadyForMigration,
  kDisabledNotReadyForMigration,

  // This is the last value. New values should be inserted above.
  kDiceMigrationStatusCount
};
#endif

class AccountConsistencyModeManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns an instance of the factory singleton.
  static AccountConsistencyModeManagerFactory* GetInstance() {
    return base::Singleton<AccountConsistencyModeManagerFactory>::get();
  }

  static AccountConsistencyModeManager* GetForProfile(Profile* profile) {
    DCHECK(profile);
    return static_cast<AccountConsistencyModeManager*>(
        GetInstance()->GetServiceForBrowserContext(profile, true));
  }

 private:
  friend struct base::DefaultSingletonTraits<
      AccountConsistencyModeManagerFactory>;

  AccountConsistencyModeManagerFactory()
      : BrowserContextKeyedServiceFactory(
            "AccountConsistencyModeManager",
            BrowserContextDependencyManager::GetInstance()) {}

  ~AccountConsistencyModeManagerFactory() override = default;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override {
    DCHECK(!context->IsOffTheRecord());
    Profile* profile = static_cast<Profile*>(context);
    return new AccountConsistencyModeManager(profile);
  }
};

// Returns the default account consistency for guest profiles.
signin::AccountConsistencyMethod GetMethodForNonRegularProfile() {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  return signin::AccountConsistencyMethod::kDiceFixAuthErrors;
#else
  return signin::AccountConsistencyMethod::kDisabled;
#endif
}

}  // namespace

bool AccountConsistencyModeManager::ignore_missing_key_for_testing_ = false;

// static
AccountConsistencyModeManager* AccountConsistencyModeManager::GetForProfile(
    Profile* profile) {
  return AccountConsistencyModeManagerFactory::GetForProfile(profile);
}

AccountConsistencyModeManager::AccountConsistencyModeManager(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  DCHECK(!profile_->IsOffTheRecord());
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  bool is_ready_for_dice = IsReadyForDiceMigration();
  PrefService* user_prefs = profile->GetPrefs();
  if (is_ready_for_dice && signin::IsDiceMigrationEnabled()) {
    if (!signin::IsDiceEnabledForProfile(user_prefs))
      VLOG(1) << "Profile is migrating to Dice";
    signin::MigrateProfileToDice(user_prefs);
    DCHECK(signin::IsDiceEnabledForProfile(user_prefs));
  }
  UMA_HISTOGRAM_ENUMERATION(
      kDiceMigrationStatusHistogram,
      signin::IsDiceEnabledForProfile(user_prefs)
          ? DiceMigrationStatus::kEnabled
          : (is_ready_for_dice
                 ? DiceMigrationStatus::kDisabledReadyForMigration
                 : DiceMigrationStatus::kDisabledNotReadyForMigration),
      DiceMigrationStatus::kDiceMigrationStatusCount);

#endif
}

AccountConsistencyModeManager::~AccountConsistencyModeManager() {}

// static
void AccountConsistencyModeManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  registry->RegisterBooleanPref(kDiceMigrationOnStartupPref, false);
#endif
}

// static
signin::AccountConsistencyMethod
AccountConsistencyModeManager::GetMethodForProfile(Profile* profile) {
  if (profile->IsOffTheRecord())
    return GetMethodForNonRegularProfile();

  return AccountConsistencyModeManager::GetForProfile(profile)
      ->GetAccountConsistencyMethod();
}

// static
signin::AccountConsistencyMethod
AccountConsistencyModeManager::GetMethodForPrefMember(
    BooleanPrefMember* dice_pref_member) {
  if (signin::IsDiceEnabled(dice_pref_member))
    return signin::AccountConsistencyMethod::kDice;

  return signin::GetAccountConsistencyMethod();
}

// static
bool AccountConsistencyModeManager::IsDiceEnabledForProfile(Profile* profile) {
  return GetMethodForProfile(profile) ==
         signin::AccountConsistencyMethod::kDice;
}

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
void AccountConsistencyModeManager::SetReadyForDiceMigration(bool is_ready) {
  DCHECK_EQ(Profile::ProfileType::REGULAR_PROFILE, profile_->GetProfileType());
  SetDiceMigrationOnStartup(profile_->GetPrefs(), is_ready);
}

// static
void AccountConsistencyModeManager::SetDiceMigrationOnStartup(
    PrefService* prefs,
    bool migrate) {
  VLOG(1) << "Dice migration on next startup: " << migrate;
  prefs->SetBoolean(kDiceMigrationOnStartupPref, migrate);
}

bool AccountConsistencyModeManager::IsReadyForDiceMigration() {
  return (profile_->GetProfileType() ==
          Profile::ProfileType::REGULAR_PROFILE) &&
         (profile_->IsNewProfile() ||
          profile_->GetPrefs()->GetBoolean(kDiceMigrationOnStartupPref));
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

// static
bool AccountConsistencyModeManager::IsMirrorEnabledForProfile(
    Profile* profile) {
  return GetMethodForProfile(profile) ==
         signin::AccountConsistencyMethod::kMirror;
}

// static
void AccountConsistencyModeManager::SetIgnoreMissingApiKeysForTesting() {
  ignore_missing_key_for_testing_ = true;
}

signin::AccountConsistencyMethod
AccountConsistencyModeManager::GetAccountConsistencyMethod() {
  if (profile_->GetProfileType() != Profile::ProfileType::REGULAR_PROFILE) {
    DCHECK_EQ(Profile::ProfileType::GUEST_PROFILE, profile_->GetProfileType());
    return GetMethodForNonRegularProfile();
  }

#if BUILDFLAG(ENABLE_MIRROR)
  return signin::AccountConsistencyMethod::kMirror;
#endif

#if defined(OS_CHROMEOS)
  if (profile_->GetPrefs()->GetBoolean(
          prefs::kAccountConsistencyMirrorRequired)) {
    return signin::AccountConsistencyMethod::kMirror;
  }
#endif

  signin::AccountConsistencyMethod method =
      signin::GetAccountConsistencyMethod();

  if (method == signin::AccountConsistencyMethod::kMirror ||
      signin::DiceMethodGreaterOrEqual(
          signin::AccountConsistencyMethod::kDiceFixAuthErrors, method)) {
    return method;
  }

  DCHECK(signin::DiceMethodGreaterOrEqual(
      method, signin::AccountConsistencyMethod::kDicePrepareMigration));

  // Legacy supervised users cannot get Dice.
  // TODO(droger): remove this once legacy supervised users are no longer
  // supported.
  if (profile_->IsLegacySupervised())
    return signin::AccountConsistencyMethod::kDiceFixAuthErrors;

  bool can_enable_dice_for_build =
      ignore_missing_key_for_testing_ || google_apis::HasKeysConfigured();
  if (!can_enable_dice_for_build) {
    LOG(WARNING) << "Desktop Identity Consistency cannot be enabled as no "
                    "API keys have been configured.";
    return signin::AccountConsistencyMethod::kDiceFixAuthErrors;
  }

  if (signin::IsDiceEnabledForProfile(profile_->GetPrefs()))
    return signin::AccountConsistencyMethod::kDice;

  return method;
}
