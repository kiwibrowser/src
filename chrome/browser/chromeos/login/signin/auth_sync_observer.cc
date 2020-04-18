// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/auth_sync_observer.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chrome/browser/chromeos/login/reauth_stats.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "components/user_manager/user_manager.h"
#include "components/user_manager/user_type.h"

namespace chromeos {

// static
bool AuthSyncObserver::ShouldObserve(Profile* profile) {
  const user_manager::User* const user =
      ProfileHelper::Get()->GetUserByProfile(profile);
  return user && (user->HasGaiaAccount() ||
                  user->GetType() == user_manager::USER_TYPE_SUPERVISED);
}

AuthSyncObserver::AuthSyncObserver(Profile* profile) : profile_(profile) {
  DCHECK(ShouldObserve(profile));
}

AuthSyncObserver::~AuthSyncObserver() {}

void AuthSyncObserver::StartObserving() {
  browser_sync::ProfileSyncService* const sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (sync_service)
    sync_service->AddObserver(this);

  SigninErrorController* const error_controller =
      SigninErrorControllerFactory::GetForProfile(profile_);
  if (error_controller) {
    error_controller->AddObserver(this);
    OnErrorChanged();
  }
}

void AuthSyncObserver::Shutdown() {
  browser_sync::ProfileSyncService* const sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile_);
  if (sync_service)
    sync_service->RemoveObserver(this);

  SigninErrorController* const error_controller =
      SigninErrorControllerFactory::GetForProfile(profile_);
  if (error_controller)
    error_controller->RemoveObserver(this);
}

void AuthSyncObserver::OnStateChanged(syncer::SyncService* sync) {
  HandleAuthError(sync->GetAuthError());
}

void AuthSyncObserver::OnErrorChanged() {
  SigninErrorController* const error_controller =
      SigninErrorControllerFactory::GetForProfile(profile_);
  const std::string error_account_id = error_controller->error_account_id();

  const std::string primary_account_id =
      SigninManagerFactory::GetForProfile(profile_)
          ->GetAuthenticatedAccountId();

  // Bail if there is an error account id and it is not the primary account id.
  if (!error_account_id.empty() && error_account_id != primary_account_id)
    return;

  HandleAuthError(error_controller->auth_error());
}

void AuthSyncObserver::HandleAuthError(
    const GoogleServiceAuthError& auth_error) {
  const user_manager::User* const user =
      ProfileHelper::Get()->GetUserByProfile(profile_);
  DCHECK(user->HasGaiaAccount() ||
         user->GetType() == user_manager::USER_TYPE_SUPERVISED);

  if (auth_error.IsPersistentError()) {
    // Invalidate OAuth2 refresh token to force Gaia sign-in flow. This is
    // needed because sign-out/sign-in solution is suggested to the user.
    LOG(WARNING) << "Invalidate OAuth token because of an auth error: "
                 << auth_error.ToString();
    const AccountId& account_id = user->GetAccountId();
    DCHECK(account_id.is_valid());

    user_manager::User::OAuthTokenStatus old_status =
        user->oauth_token_status();
    user_manager::UserManager::Get()->SaveUserOAuthStatus(
        account_id, user_manager::User::OAUTH2_TOKEN_STATUS_INVALID);
    RecordReauthReason(account_id, ReauthReason::SYNC_FAILED);

    if (user->GetType() == user_manager::USER_TYPE_SUPERVISED &&
        old_status != user_manager::User::OAUTH2_TOKEN_STATUS_INVALID) {
      // Attempt to restore token from file.
      ChromeUserManager::Get()
          ->GetSupervisedUserManager()
          ->LoadSupervisedUserToken(
              profile_, base::Bind(&AuthSyncObserver::OnSupervisedTokenLoaded,
                                   base::Unretained(this)));
      base::RecordAction(
          base::UserMetricsAction("ManagedUsers_Chromeos_Sync_Invalidated"));
    }
  } else if (auth_error.state() == GoogleServiceAuthError::NONE) {
    if (user->GetType() == user_manager::USER_TYPE_SUPERVISED &&
        user->oauth_token_status() ==
            user_manager::User::OAUTH2_TOKEN_STATUS_INVALID) {
      LOG(ERROR) << "Got an incorrectly invalidated token case, restoring "
                    "token status.";
      user_manager::UserManager::Get()->SaveUserOAuthStatus(
          user->GetAccountId(), user_manager::User::OAUTH2_TOKEN_STATUS_VALID);
      base::RecordAction(
          base::UserMetricsAction("ManagedUsers_Chromeos_Sync_Recovered"));
    }
  }
}

void AuthSyncObserver::OnSupervisedTokenLoaded(const std::string& token) {
  ChromeUserManager::Get()->GetSupervisedUserManager()->ConfigureSyncWithToken(
      profile_, token);
}

}  // namespace chromeos
