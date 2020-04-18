// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/auth/arc_auth_context.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/arc/arc_support_host.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_ui_util.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/common/url_constants.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "google_apis/gaia/gaia_constants.h"

namespace arc {

namespace {

constexpr int kMaxRetryAttempts = 3;

constexpr base::TimeDelta kRefreshTokenTimeout =
    base::TimeDelta::FromSeconds(10);

constexpr net::BackoffEntry::Policy kRetryBackoffPolicy = {
    // Number of initial errors (in sequence) to ignore before applying
    // exponential back-off rules.
    0,

    // Initial delay for exponential back-off in ms.
    5000,

    // Factor by which the waiting time will be multiplied.
    2.0,

    // Fuzzing percentage. ex: 10% will spread requests randomly
    // between 90%-100% of the calculated time.
    0.0,  // 0%

    // Maximum amount of time we are willing to delay our request in ms.
    1000 * 15,  // 15 seconds.

    // Time to keep an entry from being discarded even when it
    // has no significant state, -1 to never discard.
    -1,

    // Don't use initial delay unless the last request was an error.
    false,
};

}  // namespace

ArcAuthContext::ArcAuthContext(Profile* profile)
    : profile_(profile), retry_backoff_(&kRetryBackoffPolicy) {
  // Get token service and account ID to fetch auth tokens.
  token_service_ = ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  const SigninManagerBase* const signin_manager =
      SigninManagerFactory::GetForProfile(profile);
  CHECK(token_service_ && signin_manager);
  account_id_ = signin_manager->GetAuthenticatedAccountId();

  full_account_id_ = base::UTF16ToUTF8(
      signin_ui_util::GetAuthenticatedUsername(signin_manager));
}

ArcAuthContext::~ArcAuthContext() {
  token_service_->RemoveObserver(this);
}

void ArcAuthContext::Prepare(const PrepareCallback& callback) {
  if (context_prepared_) {
    callback.Run(profile_->GetRequestContext());
    return;
  }

  callback_ = callback;
  token_service_->RemoveObserver(this);
  refresh_token_timeout_.Stop();
  ResetFetchers();
  retry_backoff_.Reset();

  if (!token_service_->RefreshTokenIsAvailable(account_id_)) {
    token_service_->AddObserver(this);
    refresh_token_timeout_.Start(FROM_HERE, kRefreshTokenTimeout, this,
                                 &ArcAuthContext::OnRefreshTokenTimeout);
    return;
  }

  StartFetchers();
}

void ArcAuthContext::OnRefreshTokenAvailable(const std::string& account_id) {
  if (account_id != account_id_)
    return;
  OnRefreshTokensLoaded();
}

void ArcAuthContext::OnRefreshTokensLoaded() {
  token_service_->RemoveObserver(this);
  refresh_token_timeout_.Stop();
  StartFetchers();
}

void ArcAuthContext::OnRefreshTokenTimeout() {
  LOG(WARNING) << "Failed to wait for refresh token.";
  token_service_->RemoveObserver(this);
  std::move(callback_).Run(nullptr);
}

void ArcAuthContext::StartFetchers() {
  DCHECK(!refresh_token_timeout_.IsRunning());
  ResetFetchers();

  if (skip_merge_session_for_testing_) {
    OnMergeSessionSuccess("");
    return;
  }

  ubertoken_fetcher_.reset(new UbertokenFetcher(token_service_, this,
                                                GaiaConstants::kChromeOSSource,
                                                profile_->GetRequestContext()));
  ubertoken_fetcher_->StartFetchingToken(account_id_);
}

void ArcAuthContext::ResetFetchers() {
  merger_fetcher_.reset();
  ubertoken_fetcher_.reset();
  retry_timeout_.Stop();
}

void ArcAuthContext::OnFetcherError(const GoogleServiceAuthError& error) {
  ResetFetchers();
  DCHECK(error.state() != GoogleServiceAuthError::NONE);
  if (error.IsTransientError()) {
    retry_backoff_.InformOfRequest(false);
    if (retry_backoff_.failure_count() <= kMaxRetryAttempts) {
      LOG(WARNING) << "Found transient error. Retry attempt "
                   << retry_backoff_.failure_count() << ".";
      refresh_token_timeout_.Start(FROM_HERE,
                                   retry_backoff_.GetTimeUntilRelease(), this,
                                   &ArcAuthContext::StartFetchers);
      return;
    }
    LOG(WARNING) << "Too many transient errors. Stop retrying.";
  }
  std::move(callback_).Run(nullptr);
}

void ArcAuthContext::OnUbertokenSuccess(const std::string& token) {
  ResetFetchers();
  merger_fetcher_.reset(new GaiaAuthFetcher(
      this, GaiaConstants::kChromeOSSource, profile_->GetRequestContext()));
  merger_fetcher_->StartMergeSession(token, std::string());
}

void ArcAuthContext::OnUbertokenFailure(const GoogleServiceAuthError& error) {
  LOG(WARNING) << "Failed to get ubertoken " << error.ToString() << ".";
  OnFetcherError(error);
}

void ArcAuthContext::OnMergeSessionSuccess(const std::string& data) {
  VLOG_IF(1, retry_backoff_.failure_count())
      << "Auth context was successfully prepared after retry.";
  context_prepared_ = true;
  ResetFetchers();
  std::move(callback_).Run(profile_->GetRequestContext());
}

void ArcAuthContext::OnMergeSessionFailure(
    const GoogleServiceAuthError& error) {
  LOG(WARNING) << "Failed to merge gaia session " << error.ToString() << ".";
  OnFetcherError(error);
}

}  // namespace arc
