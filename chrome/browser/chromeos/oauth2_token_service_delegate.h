// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_OAUTH2_TOKEN_SERVICE_DELEGATE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "chromeos/account_manager/account_manager.h"
#include "google_apis/gaia/oauth2_token_service_delegate.h"

class AccountTrackerService;

namespace chromeos {

class ChromeOSOAuth2TokenServiceDelegate : public OAuth2TokenServiceDelegate,
                                           public AccountManager::Observer {
 public:
  // Accepts non-owning pointers to |AccountTrackerService| and
  // |AccountManager|. |AccountTrackerService| is a |KeyedService| and
  // |AccountManager| transitively belongs to |g_browser_process| and they
  // outlive (as they must) |this| delegate.
  ChromeOSOAuth2TokenServiceDelegate(
      AccountTrackerService* account_tracker_service,
      AccountManager* account_manager);
  ~ChromeOSOAuth2TokenServiceDelegate() override;

  // OAuth2TokenServiceDelegate overrides
  OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) override;
  bool RefreshTokenIsAvailable(const std::string& account_id) const override;
  void UpdateAuthError(const std::string& account_id,
                       const GoogleServiceAuthError& error) override;
  GoogleServiceAuthError GetAuthError(
      const std::string& account_id) const override;
  std::vector<std::string> GetAccounts() override;
  void LoadCredentials(const std::string& primary_account_id) override;
  void UpdateCredentials(const std::string& account_id,
                         const std::string& refresh_token) override;
  net::URLRequestContextGetter* GetRequestContext() const override;
  LoadCredentialsState GetLoadCredentialsState() const override;

  // |AccountManager::Observer| overrides
  void OnTokenUpserted(const AccountManager::AccountKey& account_key) override;

  // TODO(sinhak): Implement server token revocation.

 private:
  // Callback handler for |AccountManager::GetAccounts|.
  void GetAccountsCallback(
      std::vector<AccountManager::AccountKey> account_keys);

  // A utility method to map an |account_key| to the account id used by the
  // OAuth2TokenService chain (see |AccountInfo|). Returns an empty string for
  // non-Gaia accounts.
  std::string MapAccountKeyToAccountId(
      const AccountManager::AccountKey& account_key) const;

  // A utility method to map the |account_id| used by the OAuth2TokenService
  // chain (see |AccountInfo|) to an |AccountManager::AccountKey|.
  AccountManager::AccountKey MapAccountIdToAccountKey(
      const std::string& account_id) const;

  LoadCredentialsState load_credentials_state_ =
      LoadCredentialsState::LOAD_CREDENTIALS_NOT_STARTED;

  // A non-owning pointer to |AccountTrackerService|, which itself is a
  // |KeyedService|.
  AccountTrackerService* account_tracker_service_;

  // A non-owning pointer to |AccountManager|. |AccountManager| is available
  // throughout the lifetime of a user session.
  AccountManager* account_manager_;

  // A cache of AccountKeys.
  std::set<AccountManager::AccountKey> account_keys_;

  // A map from account id to the last seen error for that account.
  std::map<std::string, GoogleServiceAuthError> errors_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ChromeOSOAuth2TokenServiceDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOSOAuth2TokenServiceDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
