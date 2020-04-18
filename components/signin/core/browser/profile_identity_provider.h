// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_IDENTITY_PROVIDER_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_IDENTITY_PROVIDER_H_

#include "base/macros.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "google_apis/gaia/identity_provider.h"

class ProfileOAuth2TokenService;

// An identity provider implementation that's backed by
// ProfileOAuth2TokenService and SigninManager.
class ProfileIdentityProvider : public IdentityProvider,
                                public SigninManagerBase::Observer {
 public:
  ProfileIdentityProvider(SigninManagerBase* signin_manager,
                          ProfileOAuth2TokenService* token_service);
  ~ProfileIdentityProvider() override;

  // IdentityProvider:
  std::string GetActiveUsername() override;
  std::string GetActiveAccountId() override;
  OAuth2TokenService* GetTokenService() override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

 private:
  SigninManagerBase* const signin_manager_;
  ProfileOAuth2TokenService* const token_service_;

  DISALLOW_COPY_AND_ASSIGN(ProfileIdentityProvider);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_PROFILE_IDENTITY_PROVIDER_H_
