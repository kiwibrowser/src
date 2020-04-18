// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_cloud_policy_token_forwarder.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/notification_source.h"
#include "google_apis/gaia/gaia_constants.h"

namespace policy {

UserCloudPolicyTokenForwarder::UserCloudPolicyTokenForwarder(
    UserCloudPolicyManagerChromeOS* manager,
    ProfileOAuth2TokenService* token_service,
    SigninManagerBase* signin_manager)
    : OAuth2TokenService::Consumer("policy_token_forwarder"),
      manager_(manager),
      token_service_(token_service),
      signin_manager_(signin_manager) {
  // Start by waiting for the CloudPolicyService to be initialized, so that
  // we can check if it already has a DMToken or not.
  if (manager_->core()->service()->IsInitializationComplete()) {
    Initialize();
  } else {
    manager_->core()->service()->AddObserver(this);
  }
}

UserCloudPolicyTokenForwarder::~UserCloudPolicyTokenForwarder() {}

void UserCloudPolicyTokenForwarder::Shutdown() {
  request_.reset();
  token_service_->RemoveObserver(this);
  manager_->core()->service()->RemoveObserver(this);
}

void UserCloudPolicyTokenForwarder::OnRefreshTokenAvailable(
    const std::string& account_id) {
  RequestAccessToken();
}

void UserCloudPolicyTokenForwarder::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  manager_->OnAccessTokenAvailable(access_token);
  // All done here.
  Shutdown();
}

void UserCloudPolicyTokenForwarder::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // This should seldom happen: if the user is signing in for the first time
  // then this was an online signin and network errors are unlikely; if the
  // user had already signed in before then they should have policy cached, and
  // RequestAccessToken() wouldn't have been invoked.
  // Still, something just went wrong (server 500, or something). Currently
  // we don't recover in this case, and we'll just try to register for policy
  // again on the next signin.
  // TODO(joaodasilva, atwilson): consider blocking signin when this happens,
  // so that the user has to try again before getting into the session. That
  // would guarantee that a session always has fresh policy, or at least
  // enforces a cached policy.
  Shutdown();
}

void UserCloudPolicyTokenForwarder::OnInitializationCompleted(
    CloudPolicyService* service) {
  Initialize();
}

void UserCloudPolicyTokenForwarder::Initialize() {
  // TODO(mnissler): Once a better way to reconfirm whether a user is on the
  // login whitelist is available, there is no reason to fetch the OAuth2 token
  // here if the client is already registered, so check and bail out here.

  if (token_service_->RefreshTokenIsAvailable(
          signin_manager_->GetAuthenticatedAccountId()))
    RequestAccessToken();
  else
    token_service_->AddObserver(this);
}

void UserCloudPolicyTokenForwarder::RequestAccessToken() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GaiaConstants::kDeviceManagementServiceOAuth);
  scopes.insert(GaiaConstants::kOAuthWrapBridgeUserInfoScope);
  request_ = token_service_->StartRequest(
      signin_manager_->GetAuthenticatedAccountId(), scopes, this);
}

}  // namespace policy
