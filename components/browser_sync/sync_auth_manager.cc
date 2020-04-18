// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/sync_auth_manager.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/stop_source.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/engine/sync_credentials.h"
#include "google_apis/gaia/gaia_constants.h"
#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"

namespace browser_sync {

namespace {

constexpr char kSyncOAuthConsumerName[] = "sync";

constexpr net::BackoffEntry::Policy kRequestAccessTokenBackoffPolicy = {
    // Number of initial errors (in sequence) to ignore before applying
    // exponential back-off rules.
    0,

    // Initial delay for exponential back-off in ms.
    2000,

    // Factor by which the waiting time will be multiplied.
    2,

    // Fuzzing percentage. ex: 10% will spread requests randomly
    // between 90%-100% of the calculated time.
    0.2,  // 20%

    // Maximum amount of time we are willing to delay our request in ms.
    // TODO(crbug.com/246686): We should retry RequestAccessToken on connection
    // state change after backoff.
    1000 * 3600 * 4,  // 4 hours.

    // Time to keep an entry from being discarded even when it
    // has no significant state, -1 to never discard.
    -1,

    // Don't use initial delay unless the last request was an error.
    false,
};

}  // namespace

SyncAuthManager::SyncAuthManager(ProfileSyncService* sync_service,
                                 syncer::SyncPrefs* sync_prefs,
                                 identity::IdentityManager* identity_manager,
                                 OAuth2TokenService* token_service)
    : sync_service_(sync_service),
      sync_prefs_(sync_prefs),
      identity_manager_(identity_manager),
      token_service_(token_service),
      registered_for_auth_notifications_(false),
      is_auth_in_progress_(false),
      request_access_token_backoff_(&kRequestAccessTokenBackoffPolicy),
      weak_ptr_factory_(this) {
  DCHECK(sync_service_);
  DCHECK(sync_prefs_);
  // |identity_manager_| and |token_service_| can be null if local Sync is
  // enabled.
}

SyncAuthManager::~SyncAuthManager() {
  if (registered_for_auth_notifications_) {
    token_service_->RemoveObserver(this);
    identity_manager_->RemoveObserver(this);
  }
}

void SyncAuthManager::RegisterForAuthNotifications() {
  DCHECK(!registered_for_auth_notifications_);
  identity_manager_->AddObserver(this);
  token_service_->AddObserver(this);
  registered_for_auth_notifications_ = true;
}

AccountInfo SyncAuthManager::GetAuthenticatedAccountInfo() const {
  return identity_manager_ ? identity_manager_->GetPrimaryAccountInfo()
                           : AccountInfo();
}

bool SyncAuthManager::RefreshTokenIsAvailable() const {
  std::string account_id = GetAuthenticatedAccountInfo().account_id;
  return !account_id.empty() &&
         token_service_->RefreshTokenIsAvailable(account_id);
}

const syncer::SyncTokenStatus& SyncAuthManager::GetSyncTokenStatus() const {
  return token_status_;
}

syncer::SyncCredentials SyncAuthManager::GetCredentials() const {
  syncer::SyncCredentials credentials;

  const AccountInfo account_info = GetAuthenticatedAccountInfo();
  credentials.account_id = account_info.account_id;
  credentials.email = account_info.email;
  credentials.sync_token = access_token_;

  credentials.scope_set.insert(GaiaConstants::kChromeSyncOAuth2Scope);

  return credentials;
}

void SyncAuthManager::ConnectionStatusChanged(syncer::ConnectionStatus status) {
  token_status_.connection_status_update_time = base::Time::Now();
  token_status_.connection_status = status;

  switch (status) {
    case syncer::CONNECTION_AUTH_ERROR:
      // Sync server returned error indicating that access token is invalid. It
      // could be either expired or access is revoked. Let's request another
      // access token and if access is revoked then request for token will fail
      // with corresponding error. If access token is repeatedly reported
      // invalid, there may be some issues with server, e.g. authentication
      // state is inconsistent on sync and token server. In that case, we
      // backoff token requests exponentially to avoid hammering token server
      // too much and to avoid getting same token due to token server's caching
      // policy. |request_access_token_retry_timer_| is used to backoff request
      // triggered by both auth error and failure talking to GAIA server.
      // Therefore, we're likely to reach the backoff ceiling more quickly than
      // you would expect from looking at the BackoffPolicy if both types of
      // errors happen. We shouldn't receive two errors back-to-back without
      // attempting a token/sync request in between, thus crank up request delay
      // unnecessary. This is because we won't make a sync request if we hit an
      // error until GAIA succeeds at sending a new token, and we won't request
      // a new token unless sync reports a token failure. But to be safe, don't
      // schedule request if this happens.
      if (request_access_token_retry_timer_.IsRunning()) {
        // The timer to perform a request later is already running; nothing
        // further needs to be done at this point.
      } else if (request_access_token_backoff_.failure_count() == 0) {
        // First time request without delay. Currently invalid token is used
        // to initialize sync engine and we'll always end up here. We don't
        // want to delay initialization.
        request_access_token_backoff_.InformOfRequest(false);
        RequestAccessToken();
      } else {
        request_access_token_backoff_.InformOfRequest(false);
        base::TimeDelta delay =
            request_access_token_backoff_.GetTimeUntilRelease();
        token_status_.next_token_request_time = base::Time::Now() + delay;
        request_access_token_retry_timer_.Start(
            FROM_HERE, delay,
            base::BindRepeating(&SyncAuthManager::RequestAccessToken,
                                weak_ptr_factory_.GetWeakPtr()));
      }
      break;
    case syncer::CONNECTION_OK:
      // Reset backoff time after successful connection.
      // Request shouldn't be scheduled at this time. But if it is, it's
      // possible that sync flips between OK and auth error states rapidly,
      // thus hammers token server. To be safe, only reset backoff delay when
      // no scheduled request.
      if (!request_access_token_retry_timer_.IsRunning()) {
        request_access_token_backoff_.Reset();
      }
      ClearAuthError();
      break;
    case syncer::CONNECTION_SERVER_ERROR:
      UpdateAuthErrorState(
          GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED));
      break;
    case syncer::CONNECTION_NOT_ATTEMPTED:
      // The connection status should never change to "not attempted".
      NOTREACHED();
      break;
  }
}

void SyncAuthManager::UpdateAuthErrorState(
    const GoogleServiceAuthError& error) {
  is_auth_in_progress_ = false;
  last_auth_error_ = error;
}

void SyncAuthManager::ClearAuthError() {
  UpdateAuthErrorState(GoogleServiceAuthError::AuthErrorNone());
}

void SyncAuthManager::ClearAccessTokenAndRequest() {
  access_token_.clear();
  request_access_token_retry_timer_.Stop();
  token_status_.next_token_request_time = base::Time();
  ongoing_access_token_fetch_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void SyncAuthManager::Clear() {
  ClearAuthError();
  ClearAccessTokenAndRequest();
}

void SyncAuthManager::OnPrimaryAccountSet(
    const AccountInfo& primary_account_info) {
  // Track the fact that we're still waiting for auth to complete.
  DCHECK(!is_auth_in_progress_);
  is_auth_in_progress_ = true;

  sync_service_->OnPrimaryAccountSet();

  if (token_service_->RefreshTokenIsAvailable(
          primary_account_info.account_id)) {
    OnRefreshTokenAvailable(primary_account_info.account_id);
  }
}

void SyncAuthManager::OnPrimaryAccountCleared(
    const AccountInfo& previous_primary_account_info) {
  UMA_HISTOGRAM_ENUMERATION("Sync.StopSource", syncer::SIGN_OUT,
                            syncer::STOP_SOURCE_LIMIT);
  is_auth_in_progress_ = false;
  sync_service_->OnPrimaryAccountCleared();
}

void SyncAuthManager::OnRefreshTokenAvailable(const std::string& account_id) {
  if (account_id != GetAuthenticatedAccountInfo().account_id) {
    return;
  }

  GoogleServiceAuthError token_error =
      token_service_->GetAuthError(GetAuthenticatedAccountInfo().account_id);
  if (token_error == GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                         GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                             CREDENTIALS_REJECTED_BY_CLIENT)) {
    is_auth_in_progress_ = false;
    // When the refresh token is replaced by a new token with a
    // CREDENTIALS_REJECTED_BY_CLIENT error, Sync must be stopped immediately,
    // even if the current access token is still valid. This happens e.g. when
    // the user signs out of the web with Dice enabled.
    // It is not necessary to do this when the refresh token is
    // CREDENTIALS_REJECTED_BY_SERVER, because in that case the access token
    // will be rejected by the server too.
    // We only do this in OnRefreshTokensLoaded(), as opposed to
    // OAuth2TokenService::Observer::OnAuthErrorChanged(), because
    // CREDENTIALS_REJECTED_BY_CLIENT is only set by the signin component when
    // the refresh token is created.
    ClearAccessTokenAndRequest();
    // TODO(treib): Should we also set our auth error state?

    sync_service_->OnRefreshTokenRevoked();
    // TODO(treib): We can probably early-out here - no point in also calling
    // OnRefreshTokenAvailable on the ProfileSyncService.
  }

  sync_service_->OnRefreshTokenAvailable();
}

void SyncAuthManager::OnRefreshTokenRevoked(const std::string& account_id) {
  if (account_id != GetAuthenticatedAccountInfo().account_id) {
    return;
  }

  UpdateAuthErrorState(
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));

  ClearAccessTokenAndRequest();

  sync_service_->OnRefreshTokenRevoked();
}

void SyncAuthManager::OnRefreshTokensLoaded() {
  // This notification gets fired when OAuth2TokenService loads the tokens from
  // storage. Initialize the engine if sync is enabled. If the sync token was
  // not loaded, GetCredentials() will generate invalid credentials to cause the
  // engine to generate an auth error (https://crbug.com/121755).
  // TODO(treib): Is this necessary? Either we actually have a refresh token, in
  // which case this was already called from OnRefreshTokenAvailable above, or
  // there is no refresh token, in which case Sync can't start anyway.
  sync_service_->OnRefreshTokenAvailable();
}

bool SyncAuthManager::IsRetryingAccessTokenFetchForTest() const {
  return request_access_token_retry_timer_.IsRunning();
}

void SyncAuthManager::RequestAccessToken() {
  // Only one active request at a time.
  if (ongoing_access_token_fetch_) {
    return;
  }
  request_access_token_retry_timer_.Stop();
  token_status_.next_token_request_time = base::Time();

  OAuth2TokenService::ScopeSet oauth2_scopes;
  oauth2_scopes.insert(GaiaConstants::kChromeSyncOAuth2Scope);

  // Invalidate previous token, otherwise token service will return the same
  // token again.
  if (!access_token_.empty()) {
    identity_manager_->RemoveAccessTokenFromCache(GetAuthenticatedAccountInfo(),
                                                  oauth2_scopes, access_token_);
  }

  access_token_.clear();

  token_status_.token_request_time = base::Time::Now();
  token_status_.token_receive_time = base::Time();
  token_status_.next_token_request_time = base::Time();
  ongoing_access_token_fetch_ =
      identity_manager_->CreateAccessTokenFetcherForPrimaryAccount(
          kSyncOAuthConsumerName, oauth2_scopes,
          base::BindOnce(&SyncAuthManager::AccessTokenFetched,
                         base::Unretained(this)),
          identity::PrimaryAccountAccessTokenFetcher::Mode::kImmediate);
}

void SyncAuthManager::AccessTokenFetched(const GoogleServiceAuthError& error,
                                         const std::string& access_token) {
  DCHECK(ongoing_access_token_fetch_);

  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher> fetcher_deleter(
      std::move(ongoing_access_token_fetch_));

  access_token_ = access_token;
  token_status_.last_get_token_error = error;

  switch (error.state()) {
    case GoogleServiceAuthError::NONE:
      token_status_.token_receive_time = base::Time::Now();
      sync_prefs_->SetSyncAuthError(false);
      ClearAuthError();
      break;
    case GoogleServiceAuthError::CONNECTION_FAILED:
    case GoogleServiceAuthError::REQUEST_CANCELED:
    case GoogleServiceAuthError::SERVICE_ERROR:
    case GoogleServiceAuthError::SERVICE_UNAVAILABLE:
      // Transient error. Retry after some time.
      request_access_token_backoff_.InformOfRequest(false);
      token_status_.next_token_request_time =
          base::Time::Now() +
          request_access_token_backoff_.GetTimeUntilRelease();
      request_access_token_retry_timer_.Start(
          FROM_HERE, request_access_token_backoff_.GetTimeUntilRelease(),
          base::BindRepeating(&SyncAuthManager::RequestAccessToken,
                              weak_ptr_factory_.GetWeakPtr()));
      break;
    case GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS:
      sync_prefs_->SetSyncAuthError(true);
      UpdateAuthErrorState(error);
      break;
    default:
      LOG(ERROR) << "Unexpected persistent error: " << error.ToString();
      UpdateAuthErrorState(error);
  }

  sync_service_->AccessTokenFetched(error);
}

}  // namespace browser_sync
