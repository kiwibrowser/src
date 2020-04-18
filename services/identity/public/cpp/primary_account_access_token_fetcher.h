// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_PRIMARY_ACCOUNT_ACCESS_TOKEN_FETCHER_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_PRIMARY_ACCOUNT_ACCESS_TOKEN_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace identity {

// Helper class to ease the task of obtaining an OAuth2 access token for the
// authenticated account. This handles various special cases, e.g. when the
// refresh token isn't loaded yet (during startup), or when there is some
// transient error.
// May only be used on the UI thread.
class PrimaryAccountAccessTokenFetcher : public SigninManagerBase::Observer,
                                         public OAuth2TokenService::Observer,
                                         public OAuth2TokenService::Consumer {
 public:
  // Callback for when a request completes (successful or not). On successful
  // requests, |error| is NONE and |access_token| contains the obtained OAuth2
  // access token. On failed requests, |error| contains the actual error and
  // |access_token| is empty.
  using TokenCallback =
      base::OnceCallback<void(const GoogleServiceAuthError& error,
                              const std::string& access_token)>;

  // Specifies how this instance should behave:
  // |kImmediate|: Makes one-shot immediate request.
  // |kWaitUntilAvailable|: Waits for the primary account to be available
  // before making the request.
  // Note that using |kWaitUntilAvailable| can result in waiting forever
  // if the user is not signed in and doesn't sign in.
  enum class Mode { kImmediate, kWaitUntilAvailable };

  // Instantiates a fetcher and immediately starts the process of obtaining an
  // OAuth2 access token for the given |scopes|. The |callback| is called once
  // the request completes (successful or not). If the
  // PrimaryAccountAccessTokenFetcher is destroyed before the process completes,
  // the callback is not called.
  PrimaryAccountAccessTokenFetcher(const std::string& oauth_consumer_name,
                                   SigninManagerBase* signin_manager,
                                   OAuth2TokenService* token_service,
                                   const OAuth2TokenService::ScopeSet& scopes,
                                   TokenCallback callback,
                                   Mode mode);

  ~PrimaryAccountAccessTokenFetcher() override;

 private:
  void Start();

  void WaitForRefreshToken();
  void ScheduleStartAccessTokenRequest();
  void StartAccessTokenRequest();

  // SigninManagerBase::Observer implementation.
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSigninFailed(const GoogleServiceAuthError& error) override;

  // OAuth2TokenService::Observer implementation.
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokensLoaded() override;

  // OAuth2TokenService::Consumer implementation.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  SigninManagerBase* signin_manager_;
  OAuth2TokenService* token_service_;
  OAuth2TokenService::ScopeSet scopes_;
  TokenCallback callback_;

  bool waiting_for_sign_in_;
  bool waiting_for_refresh_token_;

  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;

  // When a token request gets canceled, we want to retry once.
  bool access_token_retried_;

  Mode mode_;

  base::WeakPtrFactory<PrimaryAccountAccessTokenFetcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrimaryAccountAccessTokenFetcher);
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_PRIMARY_ACCOUNT_ACCESS_TOKEN_FETCHER_H_
