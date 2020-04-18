// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/token_handle_fetcher.h"

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/chromeos/login/signin/token_handle_util.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_keyed_service_shutdown_notifier_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "google_apis/gaia/gaia_constants.h"

namespace {
const int kMaxRetries = 3;

class TokenHandleFetcherShutdownNotifierFactory
    : public BrowserContextKeyedServiceShutdownNotifierFactory {
 public:
  static TokenHandleFetcherShutdownNotifierFactory* GetInstance() {
    return base::Singleton<TokenHandleFetcherShutdownNotifierFactory>::get();
  }

 private:
  friend struct base::DefaultSingletonTraits<
      TokenHandleFetcherShutdownNotifierFactory>;

  TokenHandleFetcherShutdownNotifierFactory()
      : BrowserContextKeyedServiceShutdownNotifierFactory(
            "TokenHandleFetcher") {
    DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  }
  ~TokenHandleFetcherShutdownNotifierFactory() override {}

  DISALLOW_COPY_AND_ASSIGN(TokenHandleFetcherShutdownNotifierFactory);
};

}  // namespace

TokenHandleFetcher::TokenHandleFetcher(TokenHandleUtil* util,
                                       const AccountId& account_id)
    : OAuth2TokenService::Consumer("user_session_manager"),
      token_handle_util_(util),
      account_id_(account_id) {}

TokenHandleFetcher::~TokenHandleFetcher() {
  if (waiting_for_refresh_token_)
    token_service_->RemoveObserver(this);
}

void TokenHandleFetcher::BackfillToken(Profile* profile,
                                       const TokenFetchingCallback& callback) {
  profile_ = profile;
  callback_ = callback;

  token_service_ = ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile);
  const std::string user_email = signin_manager->GetAuthenticatedAccountId();
  if (!token_service_->RefreshTokenIsAvailable(user_email)) {
    account_without_token_ = user_email;
    profile_shutdown_notification_ =
        TokenHandleFetcherShutdownNotifierFactory::GetInstance()
            ->Get(profile)
            ->Subscribe(base::Bind(&TokenHandleFetcher::OnProfileDestroyed,
                                   base::Unretained(this)));

    token_service_->AddObserver(this);
    waiting_for_refresh_token_ = true;
    return;
  }
  RequestAccessToken(user_email);
}

void TokenHandleFetcher::OnRefreshTokenAvailable(
    const std::string& user_email) {
  if (account_without_token_ != user_email)
    return;
  waiting_for_refresh_token_ = false;
  token_service_->RemoveObserver(this);
  RequestAccessToken(user_email);
}

void TokenHandleFetcher::RequestAccessToken(const std::string& user_email) {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GaiaConstants::kOAuth1LoginScope);
  oauth2_access_token_request_ =
      token_service_->StartRequest(user_email, scopes, this);
}

void TokenHandleFetcher::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  oauth2_access_token_request_.reset();
  FillForAccessToken(access_token);
}

void TokenHandleFetcher::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  oauth2_access_token_request_.reset();
  LOG(ERROR) << "Could not get access token to backfill token handler"
             << error.ToString();
  callback_.Run(account_id_, false);
}

void TokenHandleFetcher::FillForNewUser(const std::string& access_token,
                                        const TokenFetchingCallback& callback) {
  profile_ = chromeos::ProfileHelper::Get()->GetSigninProfile();
  callback_ = callback;
  FillForAccessToken(access_token);
}

void TokenHandleFetcher::FillForAccessToken(const std::string& access_token) {
  if (!gaia_client_.get())
    gaia_client_.reset(
        new gaia::GaiaOAuthClient(profile_->GetRequestContext()));
  tokeninfo_response_start_time_ = base::TimeTicks::Now();
  gaia_client_->GetTokenInfo(access_token, kMaxRetries, this);
}

void TokenHandleFetcher::OnOAuthError() {
  callback_.Run(account_id_, false);
}

void TokenHandleFetcher::OnNetworkError(int response_code) {
  callback_.Run(account_id_, false);
}

void TokenHandleFetcher::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  bool success = false;
  if (!token_info->HasKey("error")) {
    std::string handle;
    if (token_info->GetString("token_handle", &handle)) {
      success = true;
      token_handle_util_->StoreTokenHandle(account_id_, handle);
    }
  }
  const base::TimeDelta duration =
      base::TimeTicks::Now() - tokeninfo_response_start_time_;
  UMA_HISTOGRAM_TIMES("Login.TokenObtainResponseTime", duration);
  callback_.Run(account_id_, success);
}

void TokenHandleFetcher::OnProfileDestroyed() {
  callback_.Run(account_id_, false);
}
