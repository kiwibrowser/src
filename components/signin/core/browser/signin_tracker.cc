// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_tracker.h"

#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "google_apis/gaia/gaia_constants.h"

SigninTracker::SigninTracker(ProfileOAuth2TokenService* token_service,
                             SigninManagerBase* signin_manager,
                             GaiaCookieManagerService* cookie_manager_service,
                             Observer* observer)
    : token_service_(token_service),
      signin_manager_(signin_manager),
      cookie_manager_service_(cookie_manager_service),
      observer_(observer) {
  Initialize();
}

SigninTracker::~SigninTracker() {
  signin_manager_->RemoveObserver(this);
  token_service_->RemoveObserver(this);
  cookie_manager_service_->RemoveObserver(this);
}

void SigninTracker::Initialize() {
  DCHECK(observer_);
  signin_manager_->AddObserver(this);
  token_service_->AddObserver(this);
  cookie_manager_service_->AddObserver(this);
}

void SigninTracker::GoogleSigninSucceeded(const std::string& account_id,
                                          const std::string& username) {
  if (token_service_->RefreshTokenIsAvailable(account_id))
    observer_->SigninSuccess();
}

void SigninTracker::GoogleSigninFailed(const GoogleServiceAuthError& error) {
  observer_->SigninFailed(error);
}

void SigninTracker::OnRefreshTokenAvailable(const std::string& account_id) {
  if (account_id != signin_manager_->GetAuthenticatedAccountId())
    return;

  observer_->SigninSuccess();
}

void SigninTracker::OnAddAccountToCookieCompleted(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  observer_->AccountAddedToCookie(error);
}
