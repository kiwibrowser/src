// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/oauth2_token_fetcher.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using content::BrowserThread;

namespace {

// OAuth token request max retry count.
const int kMaxRequestAttemptCount = 5;
// OAuth token request retry delay in milliseconds.
const int kRequestRestartDelay = 3000;

}  // namespace

namespace chromeos {

OAuth2TokenFetcher::OAuth2TokenFetcher(
    OAuth2TokenFetcher::Delegate* delegate,
    net::URLRequestContextGetter* context_getter)
    : delegate_(delegate),
      auth_fetcher_(this, GaiaConstants::kChromeSource, context_getter),
      retry_count_(0) {
  DCHECK(delegate);
}

OAuth2TokenFetcher::~OAuth2TokenFetcher() {}

void OAuth2TokenFetcher::StartExchangeFromCookies(
    const std::string& session_index,
    const std::string& signin_scoped_device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  session_index_ = session_index;
  signin_scoped_device_id_ = signin_scoped_device_id;
  // Delay the verification if the network is not connected or on a captive
  // portal.
  const NetworkState* default_network =
      NetworkHandler::Get()->network_state_handler()->DefaultNetwork();
  if (!default_network || default_network->is_captive_portal()) {
    // If network is offline, defer the token fetching until online.
    VLOG(1) << "Network is offline.  Deferring OAuth2 token fetch.";
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&OAuth2TokenFetcher::StartExchangeFromCookies,
                       AsWeakPtr(), session_index, signin_scoped_device_id),
        base::TimeDelta::FromMilliseconds(kRequestRestartDelay));
    return;
  }
  auth_fetcher_.DeprecatedStartCookieForOAuthLoginTokenExchangeWithDeviceId(
      session_index, signin_scoped_device_id);
}

void OAuth2TokenFetcher::StartExchangeFromAuthCode(
    const std::string& auth_code,
    const std::string& signin_scoped_device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auth_code_ = auth_code;
  signin_scoped_device_id_ = signin_scoped_device_id;
  auth_fetcher_.StartAuthCodeForOAuth2TokenExchangeWithDeviceId(
      auth_code, signin_scoped_device_id);
}

void OAuth2TokenFetcher::OnClientOAuthSuccess(
    const GaiaAuthConsumer::ClientOAuthResult& oauth_tokens) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(1) << "Got OAuth2 tokens!";
  retry_count_ = 0;
  delegate_->OnOAuth2TokensAvailable(oauth_tokens);
}

void OAuth2TokenFetcher::OnClientOAuthFailure(
    const GoogleServiceAuthError& error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RetryOnError(
      error,
      auth_code_.empty()
          ? base::Bind(&OAuth2TokenFetcher::StartExchangeFromCookies,
                       AsWeakPtr(), session_index_, signin_scoped_device_id_)
          : base::Bind(&OAuth2TokenFetcher::StartExchangeFromAuthCode,
                       AsWeakPtr(), auth_code_, signin_scoped_device_id_),
      base::Bind(&Delegate::OnOAuth2TokensFetchFailed,
                 base::Unretained(delegate_)));
}

void OAuth2TokenFetcher::RetryOnError(const GoogleServiceAuthError& error,
                                      const base::Closure& task,
                                      const base::Closure& error_handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (error.IsTransientError() && retry_count_ < kMaxRequestAttemptCount) {
    retry_count_++;
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE, task,
        base::TimeDelta::FromMilliseconds(kRequestRestartDelay));
    return;
  }
  LOG(ERROR) << "Unrecoverable error or retry count max reached. State: "
             << error.state() << ", network error: " << error.network_error()
             << ", message: " << error.error_message();
  error_handler.Run();
}

}  // namespace chromeos
