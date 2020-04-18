// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_IDENTITY_MANAGER_IMPL_H_
#define SERVICES_IDENTITY_IDENTITY_MANAGER_IMPL_H_

#include "base/callback_list.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/identity/public/cpp/account_state.h"
#include "services/identity/public/cpp/scope_set.h"
#include "services/identity/public/mojom/identity_manager.mojom.h"

class AccountTrackerService;

namespace identity {

class IdentityManagerImpl : public mojom::IdentityManager,
                            public OAuth2TokenService::Observer,
                            public SigninManagerBase::Observer {
 public:
  static void Create(mojom::IdentityManagerRequest request,
                     AccountTrackerService* account_tracker,
                     SigninManagerBase* signin_manager,
                     ProfileOAuth2TokenService* token_service);

  IdentityManagerImpl(mojom::IdentityManagerRequest request,
                      AccountTrackerService* account_tracker,
                      SigninManagerBase* signin_manager,
                      ProfileOAuth2TokenService* token_service);
  ~IdentityManagerImpl() override;

 private:
  // Makes an access token request to the OAuth2TokenService on behalf of a
  // given consumer that has made the request to the Identity Service.
  class AccessTokenRequest : public OAuth2TokenService::Consumer {
   public:
    AccessTokenRequest(const std::string& account_id,
                       const ScopeSet& scopes,
                       const std::string& consumer_id,
                       GetAccessTokenCallback consumer_callback,
                       ProfileOAuth2TokenService* token_service,
                       IdentityManagerImpl* manager);
    ~AccessTokenRequest() override;

   private:
    // OAuth2TokenService::Consumer:
    void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                           const std::string& access_token,
                           const base::Time& expiration_time) override;
    void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                           const GoogleServiceAuthError& error) override;

    // Completes the pending access token request by calling back the consumer.
    void OnRequestCompleted(const OAuth2TokenService::Request* request,
                            const base::Optional<std::string>& access_token,
                            base::Time expiration_time,
                            const GoogleServiceAuthError& error);

    ProfileOAuth2TokenService* token_service_;
    std::unique_ptr<OAuth2TokenService::Request> token_service_request_;
    GetAccessTokenCallback consumer_callback_;
    IdentityManagerImpl* manager_;
  };
  using AccessTokenRequests =
      std::map<AccessTokenRequest*, std::unique_ptr<AccessTokenRequest>>;

  // mojom::IdentityManager:
  void GetPrimaryAccountInfo(GetPrimaryAccountInfoCallback callback) override;
  void GetPrimaryAccountWhenAvailable(
      GetPrimaryAccountWhenAvailableCallback callback) override;
  void GetAccountInfoFromGaiaId(
      const std::string& gaia_id,
      GetAccountInfoFromGaiaIdCallback callback) override;
  void GetAccounts(GetAccountsCallback callback) override;
  void GetAccessToken(const std::string& account_id,
                      const ScopeSet& scopes,
                      const std::string& consumer_id,
                      GetAccessTokenCallback callback) override;

  // OAuth2TokenService::Observer:
  void OnRefreshTokenAvailable(const std::string& account_id) override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;

  // Notified when there is a change in the state of the account
  // corresponding to |account_id|.
  void OnAccountStateChange(const std::string& account_id);

  // Deletes |request|.
  void AccessTokenRequestCompleted(AccessTokenRequest* request);

  // Gets the current state of the account represented by |account_info|.
  AccountState GetStateOfAccount(const AccountInfo& account_info);

  // Called when |signin_manager_| is shutting down. Destroys this instance,
  // since this instance can't outlive the signin classes that it is depending
  // on. Note that once IdentityManagerImpl manages the lifetime of its
  // dependencies internally, this will no longer be necessary.
  void OnSigninManagerShutdown();

  // Called when |binding_| hits a connection error. Destroys this instance,
  // since it's no longer needed.
  void OnConnectionError();

  mojo::Binding<mojom::IdentityManager> binding_;
  AccountTrackerService* account_tracker_;
  SigninManagerBase* signin_manager_;
  ProfileOAuth2TokenService* token_service_;

  std::unique_ptr<base::CallbackList<void()>::Subscription>
      signin_manager_shutdown_subscription_;

  // The set of pending requests for access tokens.
  AccessTokenRequests access_token_requests_;

  // List of callbacks that will be notified when the primary account is
  // available.
  std::vector<GetPrimaryAccountWhenAvailableCallback>
      primary_account_available_callbacks_;
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_IDENTITY_MANAGER_IMPL_H_
