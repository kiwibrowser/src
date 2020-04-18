// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace content {
class BrowserContext;
}

namespace extensions {

// Requests OAuth2 access tokens for app_shell. Requires the OAuth2 refresh
// token to be set explicitly. Only stores a single refresh token for a single
// user account. Created and accessed on the UI thread. Only one instance is
// allowed.
class ShellOAuth2TokenService : public OAuth2TokenService {
 public:
  ShellOAuth2TokenService(content::BrowserContext* browser_context,
                          std::string account_id,
                          std::string refresh_token);
  ~ShellOAuth2TokenService() override;

  // Returns the single instance for app_shell.
  static ShellOAuth2TokenService* GetInstance();

  void SetRefreshToken(const std::string& account_id,
                       const std::string& refresh_token);

  std::string AccountId() const;

  DISALLOW_COPY_AND_ASSIGN(ShellOAuth2TokenService);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_OAUTH2_TOKEN_SERVICE_H_
