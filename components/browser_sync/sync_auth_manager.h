// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_SYNC_SYNC_AUTH_MANAGER_H_
#define COMPONENTS_BROWSER_SYNC_SYNC_AUTH_MANAGER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/signin/core/browser/account_info.h"
#include "components/sync/driver/sync_token_status.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/base/backoff_entry.h"
#include "services/identity/public/cpp/identity_manager.h"

namespace identity {
class PrimaryAccountAccessTokenFetcher;
}

namespace syncer {
struct SyncCredentials;
class SyncPrefs;
}  // namespace syncer

namespace browser_sync {

class ProfileSyncService;

// SyncAuthManager tracks the primary (i.e. blessed-for-sync) account and its
// authentication state.
class SyncAuthManager : public identity::IdentityManager::Observer,
                        public OAuth2TokenService::Observer {
 public:
  // |sync_service| and |sync_prefs| must not be null and must outlive this.
  // |identity_manager| and |token_service| may be null (this is the case if
  // local Sync is enabled), but if non-null, must outlive this object.
  // TODO(crbug.com/842697): Don't pass the ProfileSyncService in here. Instead,
  // pass a callback ("AccountStateChanged(new_state)").
  SyncAuthManager(ProfileSyncService* sync_service,
                  syncer::SyncPrefs* sync_prefs,
                  identity::IdentityManager* identity_manager,
                  OAuth2TokenService* token_service);
  ~SyncAuthManager() override;

  // Tells the tracker to start listening for changes to the account/sign-in
  // status. This gets called during SyncService initialization, except in the
  // case of local Sync.
  void RegisterForAuthNotifications();

  // Returns the AccountInfo for the primary (i.e. blessed-for-sync) account, or
  // an empty AccountInfo if there isn't one.
  AccountInfo GetAuthenticatedAccountInfo() const;

  // Returns whether a refresh token is available for the primary account.
  // TODO(crbug.com/842697, crbug.com/825190): ProfileSyncService shouldn't have
  // to care about the refresh token state.
  bool RefreshTokenIsAvailable() const;

  const GoogleServiceAuthError& GetLastAuthError() const {
    return last_auth_error_;
  }
  bool IsAuthInProgress() const { return is_auth_in_progress_; }

  // Returns the credentials to be passed to the SyncEngine.
  syncer::SyncCredentials GetCredentials() const;

  const std::string& access_token() const { return access_token_; }

  // Returns the state of the access token and token request, for display in
  // internals UI.
  const syncer::SyncTokenStatus& GetSyncTokenStatus() const;

  // Called by ProfileSyncService when the status of the connection to the Sync
  // server changed. Updates auth error state accordingly.
  void ConnectionStatusChanged(syncer::ConnectionStatus status);

  // TODO(crbug.com/842697, crbug.com/825190): Make this private once
  // ProfileSyncService doesn't care about the refresh token state anymore.
  void RequestAccessToken();

  // Clears all auth-related state (error, cached access token etc). Called
  // when Sync is turned off.
  void Clear();

  // identity::IdentityManager::Observer implementation.
  void OnPrimaryAccountSet(const AccountInfo& primary_account_info) override;
  void OnPrimaryAccountCleared(
      const AccountInfo& previous_primary_account_info) override;

  // OAuth2TokenService::Observer implementation.
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokenRevoked(const std::string& account_id) override;
  void OnRefreshTokensLoaded() override;

  // Test-only method for inspecting internal state.
  bool IsRetryingAccessTokenFetchForTest() const;

 private:
  void UpdateAuthErrorState(const GoogleServiceAuthError& error);
  void ClearAuthError();

  void ClearAccessTokenAndRequest();

  void AccessTokenFetched(const GoogleServiceAuthError& error,
                          const std::string& access_token);

  ProfileSyncService* const sync_service_;
  syncer::SyncPrefs* const sync_prefs_;
  identity::IdentityManager* const identity_manager_;
  OAuth2TokenService* const token_service_;

  bool registered_for_auth_notifications_;

  // This is a cache of the last authentication response we received either
  // from the sync server or from Chrome's identity/token management system.
  GoogleServiceAuthError last_auth_error_;

  // Set to true if a signin has completed but we're still waiting for the
  // engine to refresh its credentials.
  bool is_auth_in_progress_;

  std::string access_token_;

  // Pending request for an access token. Non-null iff there is a request
  // ongoing.
  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher>
      ongoing_access_token_fetch_;

  // If RequestAccessToken fails with transient error then retry requesting
  // access token with exponential backoff.
  base::OneShotTimer request_access_token_retry_timer_;
  net::BackoffEntry request_access_token_backoff_;

  // Info about the state of our access token, for display in the internals UI.
  syncer::SyncTokenStatus token_status_;

  base::WeakPtrFactory<SyncAuthManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncAuthManager);
};

}  // namespace browser_sync

#endif  // COMPONENTS_BROWSER_SYNC_SYNC_AUTH_MANAGER_H_
