// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/account_tracker.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/trace_event/trace_event.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace gcm {

AccountTracker::AccountTracker(
    SigninManagerBase* signin_manager,
    ProfileOAuth2TokenService* token_service,
    net::URLRequestContextGetter* request_context_getter)
    : signin_manager_(signin_manager),
      token_service_(token_service),
      request_context_getter_(request_context_getter),
      shutdown_called_(false) {
  signin_manager_->AddObserver(this);
  token_service_->AddObserver(this);
}

AccountTracker::~AccountTracker() {
  DCHECK(shutdown_called_);
}

void AccountTracker::Shutdown() {
  shutdown_called_ = true;
  user_info_requests_.clear();
  token_service_->RemoveObserver(this);
  signin_manager_->RemoveObserver(this);
}

bool AccountTracker::IsAllUserInfoFetched() const {
  return user_info_requests_.empty();
}

void AccountTracker::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void AccountTracker::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

std::vector<AccountIds> AccountTracker::GetAccounts() const {
  const std::string active_account_id =
      signin_manager_->GetAuthenticatedAccountId();
  std::vector<AccountIds> accounts;

  for (std::map<std::string, AccountState>::const_iterator it =
           accounts_.begin();
       it != accounts_.end(); ++it) {
    const AccountState& state = it->second;
    bool is_visible = state.is_signed_in && !state.ids.gaia.empty();

    if (it->first == active_account_id) {
      if (is_visible)
        accounts.insert(accounts.begin(), state.ids);
      else
        return std::vector<AccountIds>();

    } else if (is_visible) {
      accounts.push_back(state.ids);
    }
  }
  return accounts;
}

void AccountTracker::OnRefreshTokenAvailable(const std::string& account_id) {
  TRACE_EVENT1("identity", "AccountTracker::OnRefreshTokenAvailable",
               "account_key", account_id);

  // Ignore refresh tokens if there is no active account ID at all.
  if (signin_manager_->GetAuthenticatedAccountId().empty())
    return;

  DVLOG(1) << "AVAILABLE " << account_id;
  UpdateSignInState(account_id, true);
}

void AccountTracker::OnRefreshTokenRevoked(const std::string& account_id) {
  TRACE_EVENT1("identity", "AccountTracker::OnRefreshTokenRevoked",
               "account_key", account_id);

  DVLOG(1) << "REVOKED " << account_id;
  UpdateSignInState(account_id, false);
}

void AccountTracker::GoogleSigninSucceeded(const std::string& account_id,
                                           const std::string& username) {
  TRACE_EVENT0("identity", "AccountTracker::GoogleSigninSucceeded");

  std::vector<std::string> accounts = token_service_->GetAccounts();

  DVLOG(1) << "LOGIN " << accounts.size() << " accounts available.";

  for (std::vector<std::string>::const_iterator it = accounts.begin();
       it != accounts.end(); ++it) {
    OnRefreshTokenAvailable(*it);
  }
}

void AccountTracker::GoogleSignedOut(const std::string& account_id,
                                     const std::string& username) {
  TRACE_EVENT0("identity", "AccountTracker::GoogleSignedOut");
  DVLOG(1) << "LOGOUT";
  StopTrackingAllAccounts();
}

void AccountTracker::NotifySignInChanged(const AccountState& account) {
  DCHECK(!account.ids.gaia.empty());
  for (auto& observer : observer_list_)
    observer.OnAccountSignInChanged(account.ids, account.is_signed_in);
}

void AccountTracker::UpdateSignInState(const std::string& account_key,
                                       bool is_signed_in) {
  StartTrackingAccount(account_key);
  AccountState& account = accounts_[account_key];
  bool needs_gaia_id = account.ids.gaia.empty();
  bool was_signed_in = account.is_signed_in;
  account.is_signed_in = is_signed_in;

  if (needs_gaia_id && is_signed_in)
    StartFetchingUserInfo(account_key);

  if (!needs_gaia_id && (was_signed_in != is_signed_in))
    NotifySignInChanged(account);
}

void AccountTracker::StartTrackingAccount(const std::string& account_key) {
  if (!base::ContainsKey(accounts_, account_key)) {
    DVLOG(1) << "StartTracking " << account_key;
    AccountState account_state;
    account_state.ids.account_key = account_key;
    account_state.ids.email = account_key;
    account_state.is_signed_in = false;
    accounts_.insert(make_pair(account_key, account_state));
  }
}

void AccountTracker::StopTrackingAccount(const std::string account_key) {
  DVLOG(1) << "StopTracking " << account_key;
  if (base::ContainsKey(accounts_, account_key)) {
    AccountState& account = accounts_[account_key];
    if (!account.ids.gaia.empty()) {
      UpdateSignInState(account_key, false);
    }
    accounts_.erase(account_key);
  }

  if (base::ContainsKey(user_info_requests_, account_key))
    DeleteFetcher(user_info_requests_[account_key].get());
}

void AccountTracker::StopTrackingAllAccounts() {
  while (!accounts_.empty())
    StopTrackingAccount(accounts_.begin()->first);
}

void AccountTracker::StartFetchingUserInfo(const std::string& account_key) {
  if (base::ContainsKey(user_info_requests_, account_key)) {
    DeleteFetcher(user_info_requests_[account_key].get());
  }

  DVLOG(1) << "StartFetching " << account_key;
  AccountIdFetcher* fetcher = new AccountIdFetcher(
      token_service_, request_context_getter_.get(), this, account_key);
  user_info_requests_[account_key] = base::WrapUnique(fetcher);
  fetcher->Start();
}

void AccountTracker::OnUserInfoFetchSuccess(AccountIdFetcher* fetcher,
                                            const std::string& gaia_id) {
  const std::string& account_key = fetcher->account_key();
  DCHECK(base::ContainsKey(accounts_, account_key));
  AccountState& account = accounts_[account_key];

  account.ids.gaia = gaia_id;

  if (account.is_signed_in)
    NotifySignInChanged(account);

  DeleteFetcher(fetcher);
}

void AccountTracker::OnUserInfoFetchFailure(AccountIdFetcher* fetcher) {
  LOG(WARNING) << "Failed to get UserInfo for " << fetcher->account_key();
  std::string key = fetcher->account_key();
  DeleteFetcher(fetcher);
  StopTrackingAccount(key);
}

void AccountTracker::DeleteFetcher(AccountIdFetcher* fetcher) {
  DVLOG(1) << "DeleteFetcher " << fetcher->account_key();
  const std::string& account_key = fetcher->account_key();
  DCHECK(base::ContainsKey(user_info_requests_, account_key));
  DCHECK_EQ(fetcher, user_info_requests_[account_key].get());
  user_info_requests_.erase(account_key);
}

AccountIdFetcher::AccountIdFetcher(
    OAuth2TokenService* token_service,
    net::URLRequestContextGetter* request_context_getter,
    AccountTracker* tracker,
    const std::string& account_key)
    : OAuth2TokenService::Consumer("gaia_account_tracker"),
      token_service_(token_service),
      request_context_getter_(request_context_getter),
      tracker_(tracker),
      account_key_(account_key) {
  TRACE_EVENT_ASYNC_BEGIN1("identity", "AccountIdFetcher", this, "account_key",
                           account_key);
}

AccountIdFetcher::~AccountIdFetcher() {
  TRACE_EVENT_ASYNC_END0("identity", "AccountIdFetcher", this);
}

void AccountIdFetcher::Start() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert("https://www.googleapis.com/auth/userinfo.profile");
  login_token_request_ =
      token_service_->StartRequest(account_key_, scopes, this);
}

void AccountIdFetcher::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  TRACE_EVENT_ASYNC_STEP_PAST0("identity", "AccountIdFetcher", this,
                               "OnGetTokenSuccess");
  DCHECK_EQ(request, login_token_request_.get());

  gaia_oauth_client_.reset(new gaia::GaiaOAuthClient(request_context_getter_));

  const int kMaxGetUserIdRetries = 3;
  gaia_oauth_client_->GetUserId(access_token, kMaxGetUserIdRetries, this);
}

void AccountIdFetcher::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  TRACE_EVENT_ASYNC_STEP_PAST1("identity", "AccountIdFetcher", this,
                               "OnGetTokenFailure", "google_service_auth_error",
                               error.ToString());
  LOG(ERROR) << "OnGetTokenFailure: " << error.ToString();
  DCHECK_EQ(request, login_token_request_.get());
  tracker_->OnUserInfoFetchFailure(this);
}

void AccountIdFetcher::OnGetUserIdResponse(const std::string& gaia_id) {
  TRACE_EVENT_ASYNC_STEP_PAST1("identity", "AccountIdFetcher", this,
                               "OnGetUserIdResponse", "gaia_id", gaia_id);
  tracker_->OnUserInfoFetchSuccess(this, gaia_id);
}

void AccountIdFetcher::OnOAuthError() {
  TRACE_EVENT_ASYNC_STEP_PAST0("identity", "AccountIdFetcher", this,
                               "OnOAuthError");
  LOG(ERROR) << "OnOAuthError";
  tracker_->OnUserInfoFetchFailure(this);
}

void AccountIdFetcher::OnNetworkError(int response_code) {
  TRACE_EVENT_ASYNC_STEP_PAST1("identity", "AccountIdFetcher", this,
                               "OnNetworkError", "response_code",
                               response_code);
  LOG(ERROR) << "OnNetworkError " << response_code;
  tracker_->OnUserInfoFetchFailure(this);
}

}  // namespace gcm
