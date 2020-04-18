// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/user_policy_signin_service.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "components/account_id/account_id.h"
#include "components/policy/core/common/cloud/cloud_policy_client_registration_helper.h"
#include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/url_request/url_request_context_getter.h"

namespace policy {

UserPolicySigninService::UserPolicySigninService(
    Profile* profile,
    PrefService* local_state,
    DeviceManagementService* device_management_service,
    UserCloudPolicyManager* policy_manager,
    SigninManager* signin_manager,
    scoped_refptr<net::URLRequestContextGetter> system_request_context,
    ProfileOAuth2TokenService* token_service)
    : UserPolicySigninServiceBase(profile,
                                  local_state,
                                  device_management_service,
                                  policy_manager,
                                  signin_manager,
                                  system_request_context),
      profile_(profile),
      oauth2_token_service_(token_service) {
  // ProfileOAuth2TokenService should not yet have loaded its tokens since this
  // happens in the background after PKS initialization - so this service
  // should always be created before the oauth token is available.
  DCHECK(!oauth2_token_service_->RefreshTokenIsAvailable(
             signin_manager->GetAuthenticatedAccountId()));

  // Listen for an OAuth token to become available so we can register a client
  // if for some reason the client is not already registered (for example, if
  // the policy load failed during initial signin).
  oauth2_token_service_->AddObserver(this);
}

UserPolicySigninService::~UserPolicySigninService() {
}

void UserPolicySigninService::PrepareForUserCloudPolicyManagerShutdown() {
  // Stop any pending registration helper activity. We do this here instead of
  // in the destructor because we want to shutdown the registration helper
  // before UserCloudPolicyManager shuts down the CloudPolicyClient.
  registration_helper_.reset();

  UserPolicySigninServiceBase::PrepareForUserCloudPolicyManagerShutdown();
}

void UserPolicySigninService::Shutdown() {
  UserPolicySigninServiceBase::Shutdown();
  oauth2_token_service_->RemoveObserver(this);
}

void UserPolicySigninService::RegisterForPolicyWithLoginToken(
    const std::string& username,
    const std::string& oauth2_refresh_token,
    const PolicyRegistrationCallback& callback) {
  DCHECK(!oauth2_refresh_token.empty());

  // Create a new CloudPolicyClient for fetching the DMToken.
  std::unique_ptr<CloudPolicyClient> policy_client =
      CreateClientForRegistrationOnly(username);
  if (!policy_client) {
    callback.Run(std::string(), std::string());
    return;
  }

  // Fire off the registration process. Callback keeps the CloudPolicyClient
  // alive for the length of the registration process. Use the system
  // request context because the user is not signed in to this profile yet
  // (we are just doing a test registration to see if policy is supported for
  // this user).
  registration_helper_ = std::make_unique<CloudPolicyClientRegistrationHelper>(
      policy_client.get(),
      enterprise_management::DeviceRegisterRequest::BROWSER);
  registration_helper_->StartRegistrationWithLoginToken(
      oauth2_refresh_token,
      base::Bind(&UserPolicySigninService::CallPolicyRegistrationCallback,
                 base::Unretained(this),
                 base::Passed(&policy_client),
                 callback));
}

void UserPolicySigninService::RegisterForPolicyWithAccountId(
    const std::string& username,
    const std::string& account_id,
    const PolicyRegistrationCallback& callback) {
  DCHECK(!account_id.empty());

  // Create a new CloudPolicyClient for fetching the DMToken.
  std::unique_ptr<CloudPolicyClient> policy_client =
      CreateClientForRegistrationOnly(username);
  if (!policy_client) {
    callback.Run(std::string(), std::string());
    return;
  }

  // Fire off the registration process. Callback keeps the CloudPolicyClient
  // alive for the length of the registration process. Use the system
  // request context because the user is not signed in to this profile yet
  // (we are just doing a test registration to see if policy is supported for
  // this user).
  registration_helper_ = std::make_unique<CloudPolicyClientRegistrationHelper>(
      policy_client.get(),
      enterprise_management::DeviceRegisterRequest::BROWSER);
  registration_helper_->StartRegistration(
      oauth2_token_service_, account_id,
      base::Bind(&UserPolicySigninService::CallPolicyRegistrationCallback,
                 base::Unretained(this), base::Passed(&policy_client),
                 callback));
}

void UserPolicySigninService::CallPolicyRegistrationCallback(
    std::unique_ptr<CloudPolicyClient> client,
    PolicyRegistrationCallback callback) {
  registration_helper_.reset();
  callback.Run(client->dm_token(), client->client_id());
}

void UserPolicySigninService::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  if (!oauth2_token_service_->RefreshTokenIsAvailable(account_id))
    return;

  // ProfileOAuth2TokenService now has a refresh token for the primary account
  // so initialize the UserCloudPolicyManager.
  TryInitializeForSignedInUser();
}

void UserPolicySigninService::OnRefreshTokenAvailable(
    const std::string& account_id) {
  // Ignore OAuth tokens for any account but the primary one.
  if (account_id != signin_manager()->GetAuthenticatedAccountId())
    return;

  // ProfileOAuth2TokenService now has a refresh token for the primary account
  // so initialize the UserCloudPolicyManager.
  TryInitializeForSignedInUser();
}

void UserPolicySigninService::TryInitializeForSignedInUser() {
  DCHECK(signin_manager()->IsAuthenticated());
  DCHECK(oauth2_token_service_->RefreshTokenIsAvailable(
      signin_manager()->GetAuthenticatedAccountId()));

  // If using a TestingProfile with no UserCloudPolicyManager, skip
  // initialization.
  if (!policy_manager()) {
    DVLOG(1) << "Skipping initialization for tests due to missing components.";
    return;
  }

  InitializeForSignedInUser(
      signin_manager()->GetAuthenticatedAccountInfo().GetAccountId(),
      profile_->GetRequestContext());
}

void UserPolicySigninService::InitializeUserCloudPolicyManager(
    const AccountId& account_id,
    std::unique_ptr<CloudPolicyClient> client) {
  UserPolicySigninServiceBase::InitializeUserCloudPolicyManager(
      account_id, std::move(client));
  ProhibitSignoutIfNeeded();
}

void UserPolicySigninService::ShutdownUserCloudPolicyManager() {
  UserCloudPolicyManager* manager = policy_manager();
  // Allow the user to signout again.
  if (manager)
    signin_manager()->ProhibitSignout(false);
  UserPolicySigninServiceBase::ShutdownUserCloudPolicyManager();
}

void UserPolicySigninService::OnInitializationCompleted(
    CloudPolicyService* service) {
  UserCloudPolicyManager* manager = policy_manager();
  DCHECK_EQ(service, manager->core()->service());
  DCHECK(service->IsInitializationComplete());
  // The service is now initialized - if the client is not yet registered, then
  // it means that there is no cached policy and so we need to initiate a new
  // client registration.
  DVLOG_IF(1, manager->IsClientRegistered())
      << "Client already registered - not fetching DMToken";
  if (!manager->IsClientRegistered()) {
    if (!oauth2_token_service_->RefreshTokenIsAvailable(
             signin_manager()->GetAuthenticatedAccountId())) {
      // No token yet - this class listens for OnRefreshTokenAvailable()
      // and will re-attempt registration once the token is available.
      DLOG(WARNING) << "No OAuth Refresh Token - delaying policy download";
      return;
    }
    RegisterCloudPolicyService();
  }
  // If client is registered now, prohibit signout.
  ProhibitSignoutIfNeeded();
}

void UserPolicySigninService::RegisterCloudPolicyService() {
  DCHECK(!policy_manager()->IsClientRegistered());
  DVLOG(1) << "Fetching new DM Token";
  // Do nothing if already starting the registration process.
  if (registration_helper_)
    return;

  // Start the process of registering the CloudPolicyClient. Once it completes,
  // policy fetch will automatically happen.
  registration_helper_.reset(new CloudPolicyClientRegistrationHelper(
      policy_manager()->core()->client(),
      enterprise_management::DeviceRegisterRequest::BROWSER));
  registration_helper_->StartRegistration(
      oauth2_token_service_,
      signin_manager()->GetAuthenticatedAccountId(),
      base::Bind(&UserPolicySigninService::OnRegistrationComplete,
                 base::Unretained(this)));
}

void UserPolicySigninService::OnRegistrationComplete() {
  ProhibitSignoutIfNeeded();
  registration_helper_.reset();
}

void UserPolicySigninService::ProhibitSignoutIfNeeded() {
  if (policy_manager()->IsClientRegistered()) {
    DVLOG(1) << "User is registered for policy - prohibiting signout";
    signin_manager()->ProhibitSignout(true);
  }
}

}  // namespace policy
