// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/browser_context.h"
#include "google_apis/gaia/oauth2_access_token_fetcher_impl.h"
#include "google_apis/gaia/oauth2_token_service_delegate.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ShellOAuth2TokenServiceDelegate : public OAuth2TokenServiceDelegate {
 public:
  ShellOAuth2TokenServiceDelegate(content::BrowserContext* browser_context,
                                  std::string account_id,
                                  std::string refresh_token);
  ~ShellOAuth2TokenServiceDelegate() override;

  bool RefreshTokenIsAvailable(const std::string& account_id) const override;

  OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) override;
  net::URLRequestContextGetter* GetRequestContext() const override;

  std::vector<std::string> GetAccounts() override;

  void UpdateCredentials(const std::string& account_id,
                         const std::string& refresh_token) override;

 private:
  // Not owned.
  content::BrowserContext* browser_context_;

  // User account id, such as "foo@gmail.com".
  std::string account_id_;

  // Cached copy of an OAuth2 refresh token. Not stored on disk.
  std::string refresh_token_;

  DISALLOW_COPY_AND_ASSIGN(ShellOAuth2TokenServiceDelegate);
};

}  // namespace extensions
#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
