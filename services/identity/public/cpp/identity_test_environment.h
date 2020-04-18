// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_

#include "services/identity/public/cpp/identity_manager.h"

namespace identity {

class IdentityTestEnvironmentInternal;

// Class that creates an IdentityManager for use in testing contexts and
// provides facilities for driving that IdentityManager. The IdentityManager
// instance is brought up in an environment where the primary account is
// not available; call MakePrimaryAccountAvailable() as needed.
class IdentityTestEnvironment : public IdentityManager::DiagnosticsObserver {
 public:
  IdentityTestEnvironment();
  ~IdentityTestEnvironment() override;

  // The IdentityManager instance created and owned by this instance.
  IdentityManager* identity_manager();

  // Makes the primary account available for the given email address, generating
  // a GAIA ID and refresh token that correspond uniquely to that email address.
  // On non-ChromeOS platforms, this will also result in the firing of the
  // IdentityManager and SigninManager callbacks for signin success. On all
  // platforms, this method blocks until the primary account is available.
  void MakePrimaryAccountAvailable(std::string email);

  // Clears the primary account. On non-ChromeOS, results in the firing of the
  // IdentityManager and SigninManager callbacks for signout. Blocks until the
  // primary account is cleared.
  void ClearPrimaryAccount();

  // When this is set, access token requests will be automatically granted with
  // an access token value of "access_token".
  void SetAutomaticIssueOfAccessTokens(bool grant);

  // Issues |token| in response to an access token request that either has (a)
  // just occurred in the current iteration of the run loop, or (b) will occur
  // in the future via a task that was posted in the current iteration of the
  // run loop. In the latter case, waits until the access token request occurs.
  // NOTE: This method behaves this way to allow IdentityTestEnvironment to be
  // agnostic with respect to whether access token requests are handled
  // synchronously or asynchronously in the production code.
  // NOTE: The implementation currently issues tokens in response to *all*
  // pending access token requests. If you need finer granularity, contact
  // blundell@chromium.org
  void WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
      const std::string& token,
      const base::Time& expiration);

  // Issues |error| in response to an access token request that either has (a)
  // just occurred in the current iteration of the run loop, or (b) will occur
  // in the future via a task that was posted in the current iteration of the
  // run loop. In the latter case, waits until the access token request occurs.
  // NOTE: This method behaves this way to allow IdentityTestEnvironment to be
  // agnostic with respect to whether access token requests are handled
  // synchronously or asynchronously in the production code.
  // NOTE: The implementation currently issues errors in response to *all*
  // pending access token requests. If you need finer granularity, contact
  // blundell@chromium.org
  void WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
      const GoogleServiceAuthError& error);

 private:
  // IdentityManager::DiagnosticsObserver:
  void OnAccessTokenRequested(
      const std::string& account_id,
      const std::string& consumer_id,
      const OAuth2TokenService::ScopeSet& scopes) override;

  // Handles the notification that an access token request was received by
  // invoking |on_access_token_request_callback_| if the latter is non-null.
  void HandleOnAccessTokenRequested();

  // Runs a nested runloop until an access token request is observed.
  void WaitForAccessTokenRequestIfNecessary();

  std::unique_ptr<IdentityTestEnvironmentInternal> internals_;
  base::OnceClosure on_access_token_requested_callback_;

  DISALLOW_COPY_AND_ASSIGN(IdentityTestEnvironment);
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_
