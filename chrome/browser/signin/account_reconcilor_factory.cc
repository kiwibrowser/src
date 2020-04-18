// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_reconcilor_factory.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/account_reconcilor.h"
#include "components/signin/core/browser/account_reconcilor_delegate.h"
#include "components/signin/core/browser/mirror_account_reconcilor_delegate.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_buildflags.h"

#if defined(OS_CHROMEOS)
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "components/user_manager/user_manager.h"
#include "google_apis/gaia/google_service_auth_error.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "components/signin/core/browser/dice_account_reconcilor_delegate.h"
#endif

namespace {

#if defined(OS_CHROMEOS)
class ChromeOSChildAccountReconcilorDelegate
    : public signin::MirrorAccountReconcilorDelegate {
 public:
  explicit ChromeOSChildAccountReconcilorDelegate(
      SigninManagerBase* signin_manager)
      : signin::MirrorAccountReconcilorDelegate(signin_manager) {}

  base::TimeDelta GetReconcileTimeout() const override {
    return base::TimeDelta::FromSeconds(10);
  }

  void OnReconcileError(const GoogleServiceAuthError& error) override {
    if (error.state() == GoogleServiceAuthError::CONNECTION_FAILED) {
      // Mark the account to require an online sign in.
      const user_manager::User* primary_user =
          user_manager::UserManager::Get()->GetPrimaryUser();
      user_manager::UserManager::Get()->SaveForceOnlineSignin(
          primary_user->GetAccountId(), true /* force_online_signin */);

      // Force a logout.
      UMA_HISTOGRAM_BOOLEAN(
          "ChildAccountReconcilor.ForcedUserExitOnReconcileError", true);
      chrome::AttemptUserExit();
    }
  }
};
#endif

}  // namespace

AccountReconcilorFactory::AccountReconcilorFactory()
    : BrowserContextKeyedServiceFactory(
          "AccountReconcilor",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ChromeSigninClientFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

AccountReconcilorFactory::~AccountReconcilorFactory() {}

// static
AccountReconcilor* AccountReconcilorFactory::GetForProfile(Profile* profile) {
  return static_cast<AccountReconcilor*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AccountReconcilorFactory* AccountReconcilorFactory::GetInstance() {
  return base::Singleton<AccountReconcilorFactory>::get();
}

KeyedService* AccountReconcilorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  AccountReconcilor* reconcilor = new AccountReconcilor(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      SigninManagerFactory::GetForProfile(profile),
      ChromeSigninClientFactory::GetForProfile(profile),
      GaiaCookieManagerServiceFactory::GetForProfile(profile),
      CreateAccountReconcilorDelegate(profile));
  reconcilor->Initialize(true /* start_reconcile_if_tokens_available */);
  return reconcilor;
}

// static
std::unique_ptr<signin::AccountReconcilorDelegate>
AccountReconcilorFactory::CreateAccountReconcilorDelegate(Profile* profile) {
  if (AccountConsistencyModeManager::IsMirrorEnabledForProfile(profile)) {
#if defined(OS_CHROMEOS)
    // Only for child accounts on Chrome OS, use the specialized Mirror
    // delegate.
    if (profile->IsChild()) {
      return std::make_unique<ChromeOSChildAccountReconcilorDelegate>(
          SigninManagerFactory::GetForProfile(profile));
    }
#endif
    return std::make_unique<signin::MirrorAccountReconcilorDelegate>(
        SigninManagerFactory::GetForProfile(profile));
  }
  // TODO(droger): Remove this switch case. |AccountConsistencyModeManager| is
  // the source of truth.
  switch (signin::GetAccountConsistencyMethod()) {
    case signin::AccountConsistencyMethod::kMirror:
      // It is not possible for |IsMirrorEnabledForProfile| to return false,
      // and this case being true.
      NOTREACHED();
      return nullptr;
    case signin::AccountConsistencyMethod::kDisabled:
    case signin::AccountConsistencyMethod::kDiceFixAuthErrors:
      return std::make_unique<signin::AccountReconcilorDelegate>();
    case signin::AccountConsistencyMethod::kDicePrepareMigration:
    case signin::AccountConsistencyMethod::kDiceMigration:
    case signin::AccountConsistencyMethod::kDice:
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
      return std::make_unique<signin::DiceAccountReconcilorDelegate>(
          ChromeSigninClientFactory::GetForProfile(profile),
          AccountConsistencyModeManager::GetMethodForProfile(profile));
#else
      NOTREACHED();
      return nullptr;
#endif
  }

  NOTREACHED();
  return nullptr;
}
