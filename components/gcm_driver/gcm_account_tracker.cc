// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_account_tracker.h"

#include <stdint.h>

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/ip_endpoint.h"

namespace gcm {

namespace {

// Scopes needed by the OAuth2 access tokens.
const char kGCMGroupServerScope[] = "https://www.googleapis.com/auth/gcm";
const char kGCMCheckinServerScope[] =
    "https://www.googleapis.com/auth/android_checkin";
// Name of the GCM account tracker for the OAuth2TokenService.
const char kGCMAccountTrackerName[] = "gcm_account_tracker";
// Minimum token validity when sending to GCM groups server.
const int64_t kMinimumTokenValidityMs = 500;
// Token reporting interval, when no account changes are detected.
const int64_t kTokenReportingIntervalMs =
    12 * 60 * 60 * 1000;  // 12 hours in ms.

}  // namespace

GCMAccountTracker::AccountInfo::AccountInfo(const std::string& email,
                                            AccountState state)
    : email(email), state(state) {
}

GCMAccountTracker::AccountInfo::~AccountInfo() {
}

GCMAccountTracker::GCMAccountTracker(
    std::unique_ptr<AccountTracker> account_tracker,
    ProfileOAuth2TokenService* token_service,
    GCMDriver* driver)
    : OAuth2TokenService::Consumer(kGCMAccountTrackerName),
      account_tracker_(account_tracker.release()),
      token_service_(token_service),
      driver_(driver),
      shutdown_called_(false),
      reporting_weak_ptr_factory_(this) {}

GCMAccountTracker::~GCMAccountTracker() {
  DCHECK(shutdown_called_);
}

void GCMAccountTracker::Shutdown() {
  shutdown_called_ = true;
  driver_->RemoveConnectionObserver(this);
  account_tracker_->RemoveObserver(this);
  account_tracker_->Shutdown();
}

void GCMAccountTracker::Start() {
  DCHECK(!shutdown_called_);
  account_tracker_->AddObserver(this);
  driver_->AddConnectionObserver(this);

  std::vector<AccountIds> accounts = account_tracker_->GetAccounts();
  for (std::vector<AccountIds>::const_iterator iter = accounts.begin();
       iter != accounts.end(); ++iter) {
    if (!iter->email.empty()) {
      account_infos_.insert(std::make_pair(
          iter->account_key, AccountInfo(iter->email, TOKEN_NEEDED)));
    }
  }

  if (IsTokenReportingRequired())
    ReportTokens();
  else
    ScheduleReportTokens();
}

void GCMAccountTracker::ScheduleReportTokens() {
  // Shortcutting here, in case GCM Driver is not yet connected. In that case
  // reporting will be scheduled/started when the connection is made.
  if (!driver_->IsConnected())
    return;

  DVLOG(1) << "Deferring the token reporting for: "
           << GetTimeToNextTokenReporting().InSeconds() << " seconds.";

  reporting_weak_ptr_factory_.InvalidateWeakPtrs();
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&GCMAccountTracker::ReportTokens,
                            reporting_weak_ptr_factory_.GetWeakPtr()),
      GetTimeToNextTokenReporting());
}

void GCMAccountTracker::OnAccountSignInChanged(const AccountIds& ids,
                                               bool is_signed_in) {
  if (is_signed_in)
    OnAccountSignedIn(ids);
  else
    OnAccountSignedOut(ids);
}

void GCMAccountTracker::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK(request);
  DCHECK(!request->GetAccountId().empty());
  DVLOG(1) << "Get token success: " << request->GetAccountId();

  AccountInfos::iterator iter = account_infos_.find(request->GetAccountId());
  DCHECK(iter != account_infos_.end());
  if (iter != account_infos_.end()) {
    DCHECK(iter->second.state == GETTING_TOKEN ||
           iter->second.state == ACCOUNT_REMOVED);
    // If OnAccountSignedOut(..) was called most recently, account is kept in
    // ACCOUNT_REMOVED state.
    if (iter->second.state == GETTING_TOKEN) {
      iter->second.state = TOKEN_PRESENT;
      iter->second.access_token = access_token;
      iter->second.expiration_time = expiration_time;
    }
  }

  DeleteTokenRequest(request);
  ReportTokens();
}

void GCMAccountTracker::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK(request);
  DCHECK(!request->GetAccountId().empty());
  DVLOG(1) << "Get token failure: " << request->GetAccountId();

  AccountInfos::iterator iter = account_infos_.find(request->GetAccountId());
  DCHECK(iter != account_infos_.end());
  if (iter != account_infos_.end()) {
    DCHECK(iter->second.state == GETTING_TOKEN ||
           iter->second.state == ACCOUNT_REMOVED);
    // If OnAccountSignedOut(..) was called most recently, account is kept in
    // ACCOUNT_REMOVED state.
    if (iter->second.state == GETTING_TOKEN) {
      // Given the fetcher has a built in retry logic, consider this situation
      // to be invalid refresh token, that is only fixed when user signs in.
      // Once the users signs in properly the minting will retry.
      iter->second.access_token.clear();
      iter->second.state = ACCOUNT_REMOVED;
    }
  }

  DeleteTokenRequest(request);
  ReportTokens();
}

void GCMAccountTracker::OnConnected(const net::IPEndPoint& ip_endpoint) {
  // We are sure here, that GCM is running and connected. We can start reporting
  // tokens if reporting is due now, or schedule reporting for later.
  if (IsTokenReportingRequired())
    ReportTokens();
  else
    ScheduleReportTokens();
}

void GCMAccountTracker::OnDisconnected() {
  // We are disconnected, so no point in trying to work with tokens.
}

void GCMAccountTracker::ReportTokens() {
  SanitizeTokens();
  // Make sure all tokens are valid.
  if (IsTokenFetchingRequired()) {
    GetAllNeededTokens();
    return;
  }

  // Wait for AccountTracker to be done with fetching the user info, as
  // well as all of the pending token requests from GCMAccountTracker to be done
  // before you report the results.
  if (!account_tracker_->IsAllUserInfoFetched() ||
      !pending_token_requests_.empty()) {
    return;
  }

  bool account_removed = false;
  // Stop tracking the accounts, that were removed, as it will be reported to
  // the driver.
  for (AccountInfos::iterator iter = account_infos_.begin();
       iter != account_infos_.end();) {
    if (iter->second.state == ACCOUNT_REMOVED) {
      account_removed = true;
      account_infos_.erase(iter++);
    } else {
      ++iter;
    }
  }

  std::vector<GCMClient::AccountTokenInfo> account_tokens;
  for (AccountInfos::iterator iter = account_infos_.begin();
       iter != account_infos_.end(); ++iter) {
    if (iter->second.state == TOKEN_PRESENT) {
      GCMClient::AccountTokenInfo token_info;
      token_info.account_id = iter->first;
      token_info.email = iter->second.email;
      token_info.access_token = iter->second.access_token;
      account_tokens.push_back(token_info);
    } else {
      // This should not happen, as we are making a check that there are no
      // pending requests above, stopping tracking of removed accounts, or start
      // fetching tokens.
      NOTREACHED();
    }
  }

  // Make sure that there is something to report, otherwise bail out.
  if (!account_tokens.empty() || account_removed) {
    DVLOG(1) << "Reporting the tokens to driver: " << account_tokens.size();
    driver_->SetAccountTokens(account_tokens);
    driver_->SetLastTokenFetchTime(base::Time::Now());
    ScheduleReportTokens();
  } else {
    DVLOG(1) << "No tokens and nothing removed. Skipping callback.";
  }
}

void GCMAccountTracker::SanitizeTokens() {
  for (AccountInfos::iterator iter = account_infos_.begin();
       iter != account_infos_.end();
       ++iter) {
    if (iter->second.state == TOKEN_PRESENT &&
        iter->second.expiration_time <
            base::Time::Now() +
                base::TimeDelta::FromMilliseconds(kMinimumTokenValidityMs)) {
      iter->second.access_token.clear();
      iter->second.state = TOKEN_NEEDED;
      iter->second.expiration_time = base::Time();
    }
  }
}

bool GCMAccountTracker::IsTokenReportingRequired() const {
  if (GetTimeToNextTokenReporting().is_zero())
    return true;

  bool reporting_required = false;
  for (AccountInfos::const_iterator iter = account_infos_.begin();
       iter != account_infos_.end();
       ++iter) {
    if (iter->second.state == ACCOUNT_REMOVED)
      reporting_required = true;
  }

  return reporting_required;
}

bool GCMAccountTracker::IsTokenFetchingRequired() const {
  bool token_needed = false;
  for (AccountInfos::const_iterator iter = account_infos_.begin();
       iter != account_infos_.end();
       ++iter) {
    if (iter->second.state == TOKEN_NEEDED)
      token_needed = true;
  }

  return token_needed;
}

base::TimeDelta GCMAccountTracker::GetTimeToNextTokenReporting() const {
  base::TimeDelta time_till_next_reporting =
      driver_->GetLastTokenFetchTime() +
      base::TimeDelta::FromMilliseconds(kTokenReportingIntervalMs) -
      base::Time::Now();

  // Case when token fetching is overdue.
  if (time_till_next_reporting < base::TimeDelta())
    return base::TimeDelta();

  // Case when calculated period is larger than expected, including the
  // situation when the method is called before GCM driver is completely
  // initialized.
  if (time_till_next_reporting >
          base::TimeDelta::FromMilliseconds(kTokenReportingIntervalMs)) {
    return base::TimeDelta::FromMilliseconds(kTokenReportingIntervalMs);
  }

  return time_till_next_reporting;
}

void GCMAccountTracker::DeleteTokenRequest(
    const OAuth2TokenService::Request* request) {
  auto iter = std::find_if(
      pending_token_requests_.begin(), pending_token_requests_.end(),
      [request](const std::unique_ptr<OAuth2TokenService::Request>& r) {
        return request == r.get();
      });
  if (iter != pending_token_requests_.end())
    pending_token_requests_.erase(iter);
}

void GCMAccountTracker::GetAllNeededTokens() {
  // Only start fetching tokens if driver is running, they have a limited
  // validity time and GCM connection is a good indication of network running.
  // If the GetAllNeededTokens was called as part of periodic schedule, it may
  // not have network. In that case the next network change will trigger token
  // fetching.
  if (!driver_->IsConnected())
    return;

  for (AccountInfos::iterator iter = account_infos_.begin();
       iter != account_infos_.end();
       ++iter) {
    if (iter->second.state == TOKEN_NEEDED)
      GetToken(iter);
  }
}

void GCMAccountTracker::GetToken(AccountInfos::iterator& account_iter) {
  DCHECK_EQ(account_iter->second.state, TOKEN_NEEDED);

  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kGCMGroupServerScope);
  scopes.insert(kGCMCheckinServerScope);
  std::unique_ptr<OAuth2TokenService::Request> request =
      token_service_->StartRequest(account_iter->first, scopes, this);

  pending_token_requests_.push_back(std::move(request));
  account_iter->second.state = GETTING_TOKEN;
}

void GCMAccountTracker::OnAccountSignedIn(const AccountIds& ids) {
  DVLOG(1) << "Account signed in: " << ids.email;
  AccountInfos::iterator iter = account_infos_.find(ids.account_key);
  if (iter == account_infos_.end()) {
    DCHECK(!ids.email.empty());
    account_infos_.insert(
        std::make_pair(ids.account_key, AccountInfo(ids.email, TOKEN_NEEDED)));
  } else if (iter->second.state == ACCOUNT_REMOVED) {
    iter->second.state = TOKEN_NEEDED;
  }

  GetAllNeededTokens();
}

void GCMAccountTracker::OnAccountSignedOut(const AccountIds& ids) {
  DVLOG(1) << "Account signed out: " << ids.email;
  AccountInfos::iterator iter = account_infos_.find(ids.account_key);
  if (iter == account_infos_.end())
    return;

  iter->second.access_token.clear();
  iter->second.state = ACCOUNT_REMOVED;
  ReportTokens();
}

}  // namespace gcm
