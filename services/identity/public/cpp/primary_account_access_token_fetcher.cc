// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"

#include <utility>

#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace identity {

PrimaryAccountAccessTokenFetcher::PrimaryAccountAccessTokenFetcher(
    const std::string& oauth_consumer_name,
    SigninManagerBase* signin_manager,
    OAuth2TokenService* token_service,
    const OAuth2TokenService::ScopeSet& scopes,
    TokenCallback callback,
    Mode mode)
    : OAuth2TokenService::Consumer(oauth_consumer_name),
      signin_manager_(signin_manager),
      token_service_(token_service),
      scopes_(scopes),
      callback_(std::move(callback)),
      waiting_for_sign_in_(false),
      waiting_for_refresh_token_(false),
      access_token_retried_(false),
      mode_(mode),
      weak_factory_(this) {
  Start();
}

PrimaryAccountAccessTokenFetcher::~PrimaryAccountAccessTokenFetcher() {
  if (waiting_for_sign_in_) {
    signin_manager_->RemoveObserver(this);
  }
  if (waiting_for_refresh_token_) {
    token_service_->RemoveObserver(this);
  }
}

void PrimaryAccountAccessTokenFetcher::Start() {
  if (mode_ == Mode::kImmediate) {
    ScheduleStartAccessTokenRequest();
    return;
  }

  if (signin_manager_->IsAuthenticated()) {
    // Already signed in: Make sure we have a refresh token, then get the access
    // token.
    WaitForRefreshToken();
    return;
  }

  // Not signed in: Wait for a sign-in to complete (to get the refresh token),
  // then get the access token.
  DCHECK(!waiting_for_sign_in_);
  waiting_for_sign_in_ = true;
  signin_manager_->AddObserver(this);
}

void PrimaryAccountAccessTokenFetcher::WaitForRefreshToken() {
  DCHECK_EQ(Mode::kWaitUntilAvailable, mode_);
  DCHECK(signin_manager_->IsAuthenticated());
  DCHECK(!waiting_for_refresh_token_);

  if (token_service_->RefreshTokenIsAvailable(
          signin_manager_->GetAuthenticatedAccountId())) {
    // Already have refresh token: Get the access token directly.
    ScheduleStartAccessTokenRequest();
    return;
  }

  // Signed in, but refresh token isn't there yet: Wait for the refresh
  // token to be loaded, then get the access token.
  waiting_for_refresh_token_ = true;
  token_service_->AddObserver(this);
}

void PrimaryAccountAccessTokenFetcher::ScheduleStartAccessTokenRequest() {
  // Fire off the request asynchronously to mimic the asynchronous flow that
  // will occur when this request is going through the Identity Service.
  // NOTE: Posting the task using a WeakPtr is necessary as this instance
  // might die before the posted task runs (https://crbug.com/800263).
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PrimaryAccountAccessTokenFetcher::StartAccessTokenRequest,
                     weak_factory_.GetWeakPtr()));
}

void PrimaryAccountAccessTokenFetcher::StartAccessTokenRequest() {
  // Note: We might get here even in cases where we know that there's no refresh
  // token. We're requesting an access token anyway, so that the token service
  // will generate an appropriate error code that we can return to the client.
  DCHECK(!access_token_request_);
  access_token_request_ = token_service_->StartRequest(
      signin_manager_->GetAuthenticatedAccountId(), scopes_, this);
}

void PrimaryAccountAccessTokenFetcher::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  DCHECK_EQ(Mode::kWaitUntilAvailable, mode_);
  DCHECK(waiting_for_sign_in_);
  DCHECK(!waiting_for_refresh_token_);
  DCHECK(signin_manager_->IsAuthenticated());
  waiting_for_sign_in_ = false;
  signin_manager_->RemoveObserver(this);

  WaitForRefreshToken();
}

void PrimaryAccountAccessTokenFetcher::GoogleSigninFailed(
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(Mode::kWaitUntilAvailable, mode_);
  DCHECK(waiting_for_sign_in_);
  DCHECK(!waiting_for_refresh_token_);
  waiting_for_sign_in_ = false;
  signin_manager_->RemoveObserver(this);

  std::move(callback_).Run(error, std::string());
}

void PrimaryAccountAccessTokenFetcher::OnRefreshTokenAvailable(
    const std::string& account_id) {
  DCHECK_EQ(Mode::kWaitUntilAvailable, mode_);
  DCHECK(waiting_for_refresh_token_);
  DCHECK(!waiting_for_sign_in_);

  // Only react on tokens for the account the user has signed in with.
  if (account_id != signin_manager_->GetAuthenticatedAccountId()) {
    return;
  }

  waiting_for_refresh_token_ = false;
  token_service_->RemoveObserver(this);
  ScheduleStartAccessTokenRequest();
}

void PrimaryAccountAccessTokenFetcher::OnRefreshTokensLoaded() {
  DCHECK_EQ(Mode::kWaitUntilAvailable, mode_);
  DCHECK(waiting_for_refresh_token_);
  DCHECK(!waiting_for_sign_in_);
  DCHECK(!access_token_request_);

  // All refresh tokens were loaded, but we didn't get one for the account we
  // care about. We probably won't get one any time soon.
  // Attempt to fetch an access token anyway, so that the token service will
  // provide us with an appropriate error code.
  waiting_for_refresh_token_ = false;
  token_service_->RemoveObserver(this);
  ScheduleStartAccessTokenRequest();
}

void PrimaryAccountAccessTokenFetcher::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(request, access_token_request_.get());
  std::unique_ptr<OAuth2TokenService::Request> request_deleter(
      std::move(access_token_request_));

  std::move(callback_).Run(GoogleServiceAuthError::AuthErrorNone(),
                           access_token);
}

void PrimaryAccountAccessTokenFetcher::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(request, access_token_request_.get());
  std::unique_ptr<OAuth2TokenService::Request> request_deleter(
      std::move(access_token_request_));

  // There is a special case for Android that RefreshTokenIsAvailable and
  // StartRequest are called to pre-fetch the account image and name before
  // sign-in. In that case, our ongoing access token request gets cancelled.
  // Moreover, OnRefreshTokenAvailable might happen after startup when the
  // credentials are changed/updated.
  // To handle these cases, we retry a canceled request once.
  // However, a request may also get cancelled for legitimate reasons, e.g.
  // because the user signed out. In those cases, there's no point in retrying,
  // so only retry if there (still) is a valid refresh token.
  // NOTE: Maybe we should retry for all transient errors here, so that clients
  // don't have to.
  if (mode_ == Mode::kWaitUntilAvailable && !access_token_retried_ &&
      error.state() == GoogleServiceAuthError::State::REQUEST_CANCELED &&
      token_service_->RefreshTokenIsAvailable(
          signin_manager_->GetAuthenticatedAccountId())) {
    access_token_retried_ = true;
    ScheduleStartAccessTokenRequest();
    return;
  }

  std::move(callback_).Run(error, std::string());
}

}  // namespace identity
