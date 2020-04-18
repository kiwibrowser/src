// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_TEST_MOCK_PROFILE_OAUTH2_TOKEN_SERVICE_PROVIDER_IOS_H_
#define IOS_TEST_MOCK_PROFILE_OAUTH2_TOKEN_SERVICE_PROVIDER_IOS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_provider.h"

// Mock class of ProfileOAuth2TokenServiceIOSProvider for testing.
class FakeProfileOAuth2TokenServiceIOSProvider
    : public ProfileOAuth2TokenServiceIOSProvider {
 public:
  FakeProfileOAuth2TokenServiceIOSProvider();
  ~FakeProfileOAuth2TokenServiceIOSProvider() override;

  // ProfileOAuth2TokenServiceIOSProvider
  void GetAccessToken(const std::string& account_id,
                      const std::string& client_id,
                      const std::set<std::string>& scopes,
                      const AccessTokenCallback& callback) override;
  std::vector<AccountInfo> GetAllAccounts() const override;
  AuthenticationErrorCategory GetAuthenticationErrorCategory(
      const std::string& gaia_id,
      NSError* error) const override;

  // Methods to configure this fake provider.
  AccountInfo AddAccount(const std::string& gaia, const std::string& email);
  void ClearAccounts();

  // Issues access token responses.
  void IssueAccessTokenForAllRequests();
  void IssueAccessTokenErrorForAllRequests();

 private:
  typedef std::pair<std::string, AccessTokenCallback> AccessTokenRequest;

  std::vector<AccountInfo> accounts_;
  std::vector<AccessTokenRequest> requests_;

  DISALLOW_COPY_AND_ASSIGN(FakeProfileOAuth2TokenServiceIOSProvider);
};

#endif  // IOS_TEST_PROVIDER_CHROME_BROWSER_SIGNIN_MOCK_PROFILE_OAUTH2_TOKEN_SERVICE_PROVIDER_IOS_H_
