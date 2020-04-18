// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/account_fetcher_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_info_fetcher.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/avatar_icon_util.h"
#include "components/signin/core/browser/child_account_info_fetcher.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_switches.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

const base::TimeDelta kRefreshFromTokenServiceDelay =
    base::TimeDelta::FromHours(24);

constexpr int kAccountImageDownloadSize = 64;

bool AccountSupportsUserInfo(const std::string& account_id) {
  // Supervised users use a specially scoped token which when used for general
  // purposes causes the token service to raise spurious auth errors.
  // TODO(treib): this string is also used in supervised_user_constants.cc.
  // Should put in a common place.
  return account_id != "managed_user@localhost";
}

}  // namespace

// This pref used to be in the AccountTrackerService, hence its string value.
const char AccountFetcherService::kLastUpdatePref[] =
    "account_tracker_service_last_update";

// AccountFetcherService implementation
AccountFetcherService::AccountFetcherService()
    : account_tracker_service_(nullptr),
      token_service_(nullptr),
      signin_client_(nullptr),
      invalidation_service_(nullptr),
      network_fetches_enabled_(false),
      profile_loaded_(false),
      refresh_tokens_loaded_(false),
      shutdown_called_(false),
      scheduled_refresh_enabled_(true),
      child_info_request_(nullptr) {}

AccountFetcherService::~AccountFetcherService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(shutdown_called_);
}

// static
void AccountFetcherService::RegisterPrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {
  user_prefs->RegisterInt64Pref(kLastUpdatePref, 0);
}

void AccountFetcherService::Initialize(
    SigninClient* signin_client,
    OAuth2TokenService* token_service,
    AccountTrackerService* account_tracker_service,
    std::unique_ptr<image_fetcher::ImageDecoder> image_decoder) {
  DCHECK(signin_client);
  DCHECK(!signin_client_);
  signin_client_ = signin_client;
  DCHECK(account_tracker_service);
  DCHECK(!account_tracker_service_);
  account_tracker_service_ = account_tracker_service;
  DCHECK(token_service);
  DCHECK(!token_service_);
  token_service_ = token_service;
  token_service_->AddObserver(this);
  DCHECK(image_decoder);
  DCHECK(!image_decoder_);
  image_decoder_ = std::move(image_decoder);

  last_updated_ = base::Time::FromInternalValue(
      signin_client_->GetPrefs()->GetInt64(kLastUpdatePref));
}

void AccountFetcherService::Shutdown() {
  token_service_->RemoveObserver(this);
  // child_info_request_ is an invalidation handler and needs to be
  // unregistered during the lifetime of the invalidation service.
  child_info_request_.reset();
  invalidation_service_ = nullptr;
  shutdown_called_ = true;
}

bool AccountFetcherService::IsAllUserInfoFetched() const {
  return user_info_requests_.empty();
}

void AccountFetcherService::FetchUserInfoBeforeSignin(
    const std::string& account_id) {
  DCHECK(network_fetches_enabled_);
  RefreshAccountInfo(account_id, false);
}

void AccountFetcherService::SetupInvalidationsOnProfileLoad(
    invalidation::InvalidationService* invalidation_service) {
  DCHECK(!invalidation_service_);
  DCHECK(!profile_loaded_);
  DCHECK(!network_fetches_enabled_);
  DCHECK(!child_info_request_);
  invalidation_service_ = invalidation_service;
  profile_loaded_ = true;
  MaybeEnableNetworkFetches();
}

void AccountFetcherService::EnableNetworkFetchesForTest() {
  SetupInvalidationsOnProfileLoad(nullptr);
  OnRefreshTokensLoaded();
}

void AccountFetcherService::RefreshAllAccountInfo(bool only_fetch_if_invalid) {
  std::vector<std::string> accounts = token_service_->GetAccounts();
  for (std::vector<std::string>::const_iterator it = accounts.begin();
       it != accounts.end(); ++it) {
    RefreshAccountInfo(*it, only_fetch_if_invalid);
  }
}

// Child account status is refreshed through invalidations which are only
// available for the primary account. Finding the primary account requires a
// dependency on signin_manager which we get around by only allowing a single
// account. This is possible since we only support a single account to be a
// child anyway.
#if defined(OS_ANDROID)
void AccountFetcherService::UpdateChildInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<std::string> accounts = token_service_->GetAccounts();
  if (accounts.size() == 1) {
    const std::string& candidate = accounts[0];
    if (candidate == child_request_account_id_)
      return;
    if (!child_request_account_id_.empty())
      ResetChildInfo();
    if (!AccountSupportsUserInfo(candidate))
      return;
    child_request_account_id_ = candidate;
    StartFetchingChildInfo(candidate);
  } else {
    ResetChildInfo();
  }
}
#endif

void AccountFetcherService::MaybeEnableNetworkFetches() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!profile_loaded_ || !refresh_tokens_loaded_)
    return;
  if (!network_fetches_enabled_) {
    network_fetches_enabled_ = true;
    ScheduleNextRefresh();
  }
  RefreshAllAccountInfo(true);
#if defined(OS_ANDROID)
  UpdateChildInfo();
#endif
}

void AccountFetcherService::RefreshAllAccountsAndScheduleNext() {
  DCHECK(network_fetches_enabled_);
  RefreshAllAccountInfo(false);
  last_updated_ = base::Time::Now();
  signin_client_->GetPrefs()->SetInt64(kLastUpdatePref,
                                       last_updated_.ToInternalValue());
  ScheduleNextRefresh();
}

void AccountFetcherService::ScheduleNextRefresh() {
  if (!scheduled_refresh_enabled_)
    return;
  DCHECK(!timer_.IsRunning());
  DCHECK(network_fetches_enabled_);

  const base::TimeDelta time_since_update = base::Time::Now() - last_updated_;
  if (time_since_update > kRefreshFromTokenServiceDelay) {
    RefreshAllAccountsAndScheduleNext();
  } else {
    timer_.Start(FROM_HERE, kRefreshFromTokenServiceDelay - time_since_update,
                 this,
                 &AccountFetcherService::RefreshAllAccountsAndScheduleNext);
  }
}

// Starts fetching user information. This is called periodically to refresh.
void AccountFetcherService::StartFetchingUserInfo(
    const std::string& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(network_fetches_enabled_);

  std::unique_ptr<AccountInfoFetcher>& request =
      user_info_requests_[account_id];
  if (!request) {
    DVLOG(1) << "StartFetching " << account_id;
    std::unique_ptr<AccountInfoFetcher> fetcher =
        std::make_unique<AccountInfoFetcher>(
            token_service_, signin_client_->GetURLRequestContext(), this,
            account_id);
    request = std::move(fetcher);
    request->Start();
  }
}

// Starts fetching whether this is a child account. Handles refresh internally.
void AccountFetcherService::StartFetchingChildInfo(
    const std::string& account_id) {
  child_info_request_ = ChildAccountInfoFetcher::CreateFrom(
      child_request_account_id_, this, token_service_,
      signin_client_->GetURLRequestContext(), invalidation_service_);
}

void AccountFetcherService::ResetChildInfo() {
  if (!child_request_account_id_.empty())
    SetIsChildAccount(child_request_account_id_, false);
  child_request_account_id_.clear();
  child_info_request_.reset();
}

void AccountFetcherService::RefreshAccountInfo(const std::string& account_id,
                                               bool only_fetch_if_invalid) {
  DCHECK(network_fetches_enabled_);
  account_tracker_service_->StartTrackingAccount(account_id);
  const AccountInfo& info =
      account_tracker_service_->GetAccountInfo(account_id);
  if (!AccountSupportsUserInfo(account_id))
    return;

// |only_fetch_if_invalid| is false when the service is due for a timed update.
#if defined(OS_ANDROID)
  // TODO(mlerman): Change this condition back to info.IsValid() and ensure the
  // Fetch doesn't occur until after ProfileImpl::OnPrefsLoaded().
  if (!only_fetch_if_invalid || info.gaia.empty())
#else
  if (!only_fetch_if_invalid || !info.IsValid())
#endif
    StartFetchingUserInfo(account_id);
}

void AccountFetcherService::OnUserInfoFetchSuccess(
    const std::string& account_id,
    std::unique_ptr<base::DictionaryValue> user_info) {
  account_tracker_service_->SetAccountStateFromUserInfo(account_id,
                                                        user_info.get());
  FetchAccountImage(account_id);
  user_info_requests_.erase(account_id);
}

image_fetcher::ImageFetcherImpl*
AccountFetcherService::GetOrCreateImageFetcher() {
  // Lazy initialization of |image_fetcher_| because the request context might
  // not be available yet when |Initialize| is called.
  if (!image_fetcher_) {
    image_fetcher_ = std::make_unique<image_fetcher::ImageFetcherImpl>(
        std::move(image_decoder_), signin_client_->GetURLRequestContext());
  }
  return image_fetcher_.get();
}

void AccountFetcherService::FetchAccountImage(const std::string& account_id) {
  DCHECK(signin_client_);
  std::string picture_url_string =
      account_tracker_service_->GetAccountInfo(account_id).picture_url;
  GURL picture_url(picture_url_string);
  if (!picture_url.is_valid()) {
    DVLOG(1) << "Invalid avatar picture URL: \"" + picture_url_string + "\"";
    return;
  }
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("accounts_image_fetcher", R"(
        semantics {
          sender: "Image fetcher for GAIA accounts"
          description:
            "To use a GAIA web account to log into Chrome in the user menu, the"
            "account images of the signed-in GAIA accounts are displayed."
          trigger: "At startup."
          data: "Account picture URL of signed-in GAIA accounts."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled by settings, "
                   "however, it will only be requested if the user "
                   "has signed into the web."
          policy_exception_justification:
            "Not implemented, considered not useful as no content is being "
            "uploaded or saved; this request merely downloads the web account"
            "profile image."
        })");
  GURL image_url_with_size(signin::GetAvatarImageURLWithOptions(
      picture_url, kAccountImageDownloadSize, true /* no_silhouette */));
  auto callback = base::BindRepeating(&AccountFetcherService::OnImageFetched,
                                      base::Unretained(this));
  GetOrCreateImageFetcher()->FetchImage(account_id, image_url_with_size,
                                        callback, traffic_annotation);
}

void AccountFetcherService::SetIsChildAccount(const std::string& account_id,
                                              bool is_child_account) {
  if (child_request_account_id_ == account_id)
    account_tracker_service_->SetIsChildAccount(account_id, is_child_account);
}

void AccountFetcherService::OnUserInfoFetchFailure(
    const std::string& account_id) {
  LOG(WARNING) << "Failed to get UserInfo for " << account_id;
  account_tracker_service_->NotifyAccountUpdateFailed(account_id);
  user_info_requests_.erase(account_id);
}

void AccountFetcherService::OnRefreshTokenAvailable(
    const std::string& account_id) {
  TRACE_EVENT1("AccountFetcherService",
               "AccountFetcherService::OnRefreshTokenAvailable", "account_id",
               account_id);
  DVLOG(1) << "AVAILABLE " << account_id;

  // The SigninClient needs a "final init" in order to perform some actions
  // (such as fetching the signin token "handle" in order to look for password
  // changes) once everything is initialized and the refresh token is present.
  signin_client_->DoFinalInit();

  if (!network_fetches_enabled_)
    return;
  RefreshAccountInfo(account_id, true);
#if defined(OS_ANDROID)
  UpdateChildInfo();
#endif
}

void AccountFetcherService::OnRefreshTokenRevoked(
    const std::string& account_id) {
  TRACE_EVENT1("AccountFetcherService",
               "AccountFetcherService::OnRefreshTokenRevoked", "account_id",
               account_id);
  DVLOG(1) << "REVOKED " << account_id;

  if (!network_fetches_enabled_)
    return;
  user_info_requests_.erase(account_id);
#if defined(OS_ANDROID)
  UpdateChildInfo();
#endif
  account_tracker_service_->StopTrackingAccount(account_id);
}

void AccountFetcherService::OnRefreshTokensLoaded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  refresh_tokens_loaded_ = true;
  MaybeEnableNetworkFetches();
}

void AccountFetcherService::OnImageFetched(
    const std::string& id,
    const gfx::Image& image,
    const image_fetcher::RequestMetadata&) {
  account_tracker_service_->SetAccountImage(id, image);
}
