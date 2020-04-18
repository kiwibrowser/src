// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_PROVIDER_IMPL_H_
#define IOS_CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_PROVIDER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_provider.h"

// Implementation of ProfileOAuth2TokenServiceIOSProvider.
class ProfileOAuth2TokenServiceIOSProviderImpl
    : public ProfileOAuth2TokenServiceIOSProvider {
 public:
  ProfileOAuth2TokenServiceIOSProviderImpl();
  ~ProfileOAuth2TokenServiceIOSProviderImpl() override;

  // ios::ProfileOAuth2TokenServiceIOSProvider
  void GetAccessToken(const std::string& gaia_id,
                      const std::string& client_id,
                      const std::set<std::string>& scopes,
                      const AccessTokenCallback& callback) override;
  std::vector<AccountInfo> GetAllAccounts() const override;
  AuthenticationErrorCategory GetAuthenticationErrorCategory(
      const std::string& gaia_id,
      NSError* error) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileOAuth2TokenServiceIOSProviderImpl);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_PROFILE_OAUTH2_TOKEN_SERVICE_IOS_PROVIDER_IMPL_H_
