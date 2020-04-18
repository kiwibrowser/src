// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/oauth2_login_verifier.h"

#include <vector>

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

// String used for source parameter in GAIA cookie manager calls.
const char kCookieManagerSource[] = "ChromiumOAuth2LoginVerifier";

namespace chromeos {

OAuth2LoginVerifier::OAuth2LoginVerifier(
    OAuth2LoginVerifier::Delegate* delegate,
    GaiaCookieManagerService* cookie_manager_service,
    const std::string& primary_account_id,
    const std::string& oauthlogin_access_token)
    : delegate_(delegate),
      cookie_manager_service_(cookie_manager_service),
      primary_account_id_(primary_account_id),
      access_token_(oauthlogin_access_token) {
  DCHECK(delegate);
  cookie_manager_service_->AddObserver(this);
}

OAuth2LoginVerifier::~OAuth2LoginVerifier() {
  cookie_manager_service_->RemoveObserver(this);
}

void OAuth2LoginVerifier::VerifyUserCookies() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<gaia::ListedAccount> accounts;
  std::vector<gaia::ListedAccount> signed_out_accounts;
  if (cookie_manager_service_->ListAccounts(&accounts, &signed_out_accounts,
                                            kCookieManagerSource)) {
    OnGaiaAccountsInCookieUpdated(
        accounts, signed_out_accounts,
        GoogleServiceAuthError(GoogleServiceAuthError::NONE));
  }
}

void OAuth2LoginVerifier::VerifyProfileTokens() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (access_token_.empty()) {
    cookie_manager_service_->AddAccountToCookie(primary_account_id_,
                                                kCookieManagerSource);
  } else {
    cookie_manager_service_->AddAccountToCookieWithToken(
        primary_account_id_, access_token_, kCookieManagerSource);
  }
}

void OAuth2LoginVerifier::OnAddAccountToCookieCompleted(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  if (account_id != primary_account_id_)
    return;

  if (error.state() == GoogleServiceAuthError::State::NONE) {
    VLOG(1) << "MergeSession successful.";
    delegate_->OnSessionMergeSuccess();
    return;
  }

  LOG(WARNING) << "Failed MergeSession request,"
               << " error: " << error.state();
  delegate_->OnSessionMergeFailure(error.IsTransientError());
}

void OAuth2LoginVerifier::OnGaiaAccountsInCookieUpdated(
    const std::vector<gaia::ListedAccount>& accounts,
    const std::vector<gaia::ListedAccount>& signed_out_accounts,
    const GoogleServiceAuthError& error) {
  if (error.state() == GoogleServiceAuthError::State::NONE) {
    VLOG(1) << "ListAccounts successful.";
    delegate_->OnListAccountsSuccess(accounts);
    return;
  }

  LOG(WARNING) << "Failed to get list of session accounts, "
               << " error: " << error.state();
  delegate_->OnListAccountsFailure(error.IsTransientError());
}

}  // namespace chromeos
