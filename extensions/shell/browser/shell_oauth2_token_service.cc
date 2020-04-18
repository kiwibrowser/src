// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_oauth2_token_service.h"

#include "base/logging.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/shell/browser/shell_oauth2_token_service_delegate.h"

namespace extensions {
namespace {

ShellOAuth2TokenService* g_instance = nullptr;

}  // namespace

ShellOAuth2TokenService::ShellOAuth2TokenService(
    content::BrowserContext* browser_context,
    std::string account_id,
    std::string refresh_token)
    : OAuth2TokenService(
          std::make_unique<ShellOAuth2TokenServiceDelegate>(browser_context,
                                                            account_id,
                                                            refresh_token)) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!g_instance);
  g_instance = this;
}

ShellOAuth2TokenService::~ShellOAuth2TokenService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(g_instance);
  g_instance = nullptr;
}

// static
ShellOAuth2TokenService* ShellOAuth2TokenService::GetInstance() {
  DCHECK(g_instance);
  return g_instance;
}

void ShellOAuth2TokenService::SetRefreshToken(
    const std::string& account_id,
    const std::string& refresh_token) {
  GetDelegate()->UpdateCredentials(account_id, refresh_token);
}

std::string ShellOAuth2TokenService::AccountId() const {
  return GetAccounts()[0];
}

}  // namespace extensions
