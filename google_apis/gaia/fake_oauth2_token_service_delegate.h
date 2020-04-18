// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_FAKE_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
#define CHROME_BROWSER_SIGNIN_FAKE_OAUTH2_TOKEN_SERVICE_DELEGATE_H_

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service_delegate.h"
#include "net/url_request/url_request_context_getter.h"

class FakeOAuth2TokenServiceDelegate : public OAuth2TokenServiceDelegate {
 public:
  FakeOAuth2TokenServiceDelegate(net::URLRequestContextGetter* request_context);
  ~FakeOAuth2TokenServiceDelegate() override;

  OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) override;

  // Overriden to make sure it works on Android.
  bool RefreshTokenIsAvailable(const std::string& account_id) const override;
  GoogleServiceAuthError GetAuthError(
      const std::string& account_id) const override;
  void UpdateAuthError(const std::string& account_id,
                       const GoogleServiceAuthError& error) override;

  std::vector<std::string> GetAccounts() override;
  void RevokeAllCredentials() override;

  void LoadCredentials(const std::string& primary_account_id) override;

  void UpdateCredentials(const std::string& account_id,
                         const std::string& refresh_token) override;
  void RevokeCredentials(const std::string& account_id) override;

  net::URLRequestContextGetter* GetRequestContext() const override;

  void set_request_context(net::URLRequestContextGetter* request_context) {
    request_context_ = request_context;
  }

  std::string GetRefreshToken(const std::string& account_id) const;

 private:
  struct AccountInfo {
    AccountInfo(const std::string& refresh_token);

    const std::string refresh_token;
    GoogleServiceAuthError error;
  };

  void IssueRefreshTokenForUser(const std::string& account_id,
                                const std::string& token);

  // Maps account ids to info.
  typedef std::map<std::string, linked_ptr<AccountInfo>> AccountInfoMap;
  AccountInfoMap refresh_tokens_;

  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DISALLOW_COPY_AND_ASSIGN(FakeOAuth2TokenServiceDelegate);
};
#endif
