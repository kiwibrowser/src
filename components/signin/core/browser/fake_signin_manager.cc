// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/fake_signin_manager.h"

#include "base/callback_helpers.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/signin_metrics.h"

FakeSigninManagerBase::FakeSigninManagerBase(
    SigninClient* client,
    AccountTrackerService* account_tracker_service,
    SigninErrorController* signin_error_controller)
    : SigninManagerBase(client,
                        account_tracker_service,
                        signin_error_controller) {}

FakeSigninManagerBase::~FakeSigninManagerBase() {}

void FakeSigninManagerBase::SignIn(const std::string& account_id) {
  SetAuthenticatedAccountId(account_id);
}

#if !defined(OS_CHROMEOS)

FakeSigninManager::FakeSigninManager(
    SigninClient* client,
    ProfileOAuth2TokenService* token_service,
    AccountTrackerService* account_tracker_service,
    GaiaCookieManagerService* cookie_manager_service,
    SigninErrorController* signin_error_controller)
    : SigninManager(client,
                    token_service,
                    account_tracker_service,
                    cookie_manager_service,
                    signin_error_controller,
                    signin::AccountConsistencyMethod::kDisabled),
      token_service_(token_service) {}

FakeSigninManager::~FakeSigninManager() {}

void FakeSigninManager::StartSignInWithRefreshToken(
    const std::string& refresh_token,
    const std::string& gaia_id,
    const std::string& username,
    const std::string& password,
    const OAuthTokenFetchedCallback& oauth_fetched_callback) {
  set_auth_in_progress(
      account_tracker_service()->SeedAccountInfo(gaia_id, username));
  set_password(password);
  username_ = username;

  possibly_invalid_gaia_id_.assign(gaia_id);
  possibly_invalid_email_.assign(username);

  if (!oauth_fetched_callback.is_null())
    oauth_fetched_callback.Run(refresh_token);
}

void FakeSigninManager::CompletePendingSignin() {
  SetAuthenticatedAccountId(GetAccountIdForAuthInProgress());
  set_auth_in_progress(std::string());
  FireGoogleSigninSucceeded();
}

void FakeSigninManager::SignIn(const std::string& gaia_id,
                               const std::string& username,
                               const std::string& password) {
  StartSignInWithRefreshToken(std::string(), gaia_id, username, password,
                              OAuthTokenFetchedCallback());
  CompletePendingSignin();
}

void FakeSigninManager::ForceSignOut() {
  prohibit_signout_ = false;
  SignOut(signin_metrics::SIGNOUT_TEST,
          signin_metrics::SignoutDelete::IGNORE_METRIC);
}

void FakeSigninManager::FailSignin(const GoogleServiceAuthError& error) {
  for (auto& observer : observer_list_)
    observer.GoogleSigninFailed(error);
}

void FakeSigninManager::DoSignOut(
    signin_metrics::ProfileSignout signout_source_metric,
    signin_metrics::SignoutDelete signout_delete_metric,
    RemoveAccountsOption remove_option) {
  if (IsSignoutProhibited())
    return;
  set_auth_in_progress(std::string());
  set_password(std::string());
  AccountInfo account_info = GetAuthenticatedAccountInfo();
  const std::string account_id = GetAuthenticatedAccountId();
  const std::string username = account_info.email;
  authenticated_account_id_.clear();
  switch (remove_option) {
    case RemoveAccountsOption::kRemoveAllAccounts:
      if (token_service_)
        token_service_->RevokeAllCredentials();
      break;
    case RemoveAccountsOption::kRemoveAuthenticatedAccountIfInError:
      if (token_service_ && token_service_->RefreshTokenHasError(account_id))
        token_service_->RevokeCredentials(account_id);
      break;
    case RemoveAccountsOption::kKeepAllAccounts:
      // Do nothing.
      break;
  }

  FireGoogleSignedOut(account_id, account_info);
}

#endif  // !defined (OS_CHROMEOS)
