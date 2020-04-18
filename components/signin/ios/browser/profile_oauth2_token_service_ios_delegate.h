// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_SIGNIN_IOS_BROWSER_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_DELEGATE_H_
#define COMPONENTS_SIGNIN_IOS_BROWSER_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_DELEGATE_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "google_apis/gaia/oauth2_token_service_delegate.h"

class AccountTrackerService;
class ProfileOAuth2TokenServiceIOSProvider;
class SigninClient;

class ProfileOAuth2TokenServiceIOSDelegate : public OAuth2TokenServiceDelegate {
 public:
  ProfileOAuth2TokenServiceIOSDelegate(
      SigninClient* client,
      std::unique_ptr<ProfileOAuth2TokenServiceIOSProvider> provider,
      AccountTrackerService* account_tracker_service,
      SigninErrorController* signin_error_controller);
  ~ProfileOAuth2TokenServiceIOSDelegate() override;

  OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) override;

  // KeyedService
  void Shutdown() override;

  bool RefreshTokenIsAvailable(const std::string& account_id) const override;
  GoogleServiceAuthError GetAuthError(
      const std::string& account_id) const override;
  void UpdateAuthError(const std::string& account_id,
                       const GoogleServiceAuthError& error) override;

  void LoadCredentials(const std::string& primary_account_id) override;
  std::vector<std::string> GetAccounts() override;

  // This method should not be called when using shared authentication.
  void UpdateCredentials(const std::string& account_id,
                         const std::string& refresh_token) override;

  // Removes all credentials from this instance of |ProfileOAuth2TokenService|,
  // however, it does not revoke the identities from the device.
  // Subsequent calls to |RefreshTokenIsAvailable| will return |false|.
  void RevokeAllCredentials() override;

  // Reloads accounts from the provider. Fires |OnRefreshTokenAvailable| for
  // each new account. Fires |OnRefreshTokenRevoked| for each account that was
  // removed.
  // It expects that there is already a primary account id.
  void ReloadCredentials();

  // Sets the primary account and then reloads the accounts from the provider.
  // Should be called when the user signs in to a new account.
  // |primary_account_id| must not be an empty string.
  void ReloadCredentials(const std::string& primary_account_id);

  // Adds |account_id| to |accounts_| if it does not exist or udpates
  // the auth error state of |account_id| if it exists. Fires
  // |OnRefreshTokenAvailable| if the account info is updated.
  virtual void AddOrUpdateAccount(const std::string& account_id);

 protected:
  // Removes |account_id| from |accounts_|. Fires |OnRefreshTokenRevoked|
  // if the account info is removed.
  virtual void RemoveAccount(const std::string& account_id);

 private:
  friend class ProfileOAuth2TokenServiceIOSDelegateTest;
  FRIEND_TEST_ALL_PREFIXES(ProfileOAuth2TokenServiceIOSDelegateTest,
                           LoadRevokeCredentialsClearsExcludedAccounts);

  class AccountStatus : public SigninErrorController::AuthStatusProvider {
   public:
    AccountStatus(SigninErrorController* signin_error_controller,
                  const std::string& account_id);
    ~AccountStatus() override;

    void SetLastAuthError(const GoogleServiceAuthError& error);

    // SigninErrorController::AuthStatusProvider implementation.
    std::string GetAccountId() const override;
    GoogleServiceAuthError GetAuthStatus() const override;

   private:
    SigninErrorController* signin_error_controller_;
    std::string account_id_;
    GoogleServiceAuthError last_auth_error_;

    DISALLOW_COPY_AND_ASSIGN(AccountStatus);
  };

  // Maps the |account_id| of accounts known to ProfileOAuth2TokenService
  // to information about the account.
  typedef std::map<std::string, linked_ptr<AccountStatus>> AccountStatusMap;

  // Clears exclude secondary accounts preferences.
  void ClearExcludedSecondaryAccounts();

  // The primary account id.
  std::string primary_account_id_;

  // Info about the existing accounts.
  AccountStatusMap accounts_;

  // Calls to this class are expected to be made from the browser UI thread.
  // The purpose of this checker is to detect access to
  // ProfileOAuth2TokenService from multiple threads in upstream code.
  base::ThreadChecker thread_checker_;

  // The client with which this instance was initialied, or NULL.
  SigninClient* client_;
  std::unique_ptr<ProfileOAuth2TokenServiceIOSProvider> provider_;
  AccountTrackerService* account_tracker_service_;

  // The error controller with which this instance was initialized, or NULL.
  SigninErrorController* signin_error_controller_;

  DISALLOW_COPY_AND_ASSIGN(ProfileOAuth2TokenServiceIOSDelegate);
};
#endif  // COMPONENTS_SIGNIN_IOS_BROWSER_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_DELEGATE_H_
