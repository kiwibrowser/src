// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_
#define IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_

#include <string>
#include <vector>

#import "base/ios/block_types.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"

namespace browser_sync {
class ProfileSyncService;
}

class AccountTrackerService;
class AuthenticationServiceDelegate;
@class ChromeIdentity;
class PrefService;
class ProfileOAuth2TokenService;
class SigninManager;
class SyncSetupService;

// AuthenticationService is the Chrome interface to the iOS shared
// authentication library.
class AuthenticationService : public KeyedService,
                              public OAuth2TokenService::Observer,
                              public ios::ChromeIdentityService::Observer {
 public:
  AuthenticationService(PrefService* pref_service,
                        ProfileOAuth2TokenService* token_service,
                        SyncSetupService* sync_setup_service,
                        AccountTrackerService* account_tracker,
                        SigninManager* signin_manager,
                        browser_sync::ProfileSyncService* sync_service);
  ~AuthenticationService() override;

  // Registers the preferences used by AuthenticationService;
  static void RegisterPrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns whether the AuthenticationService has been initialized. It is
  // a fatal error to invoke any method on this object except Initialize()
  // if this method returns false.
  bool initialized() const { return initialized_; }

  // Initializes the AuthenticationService.
  void Initialize(std::unique_ptr<AuthenticationServiceDelegate> delegate);

  // KeyedService
  void Shutdown() override;

  // Reminds user to Sign in to Chrome when a new tab is opened if
  // |should_prompt| is true, otherwise stop prompting.
  void SetPromptForSignIn(bool should_prompt);

  // Returns whether user should be prompted to Sign in to Chrome.
  bool ShouldPromptForSignIn();

  // Returns whether the token service accounts have changed since the last time
  // they were stored in the browser state prefs. This storing happens every
  // time the accounts change in foreground.
  // This reloads the cached accounts if the information might be stale.
  virtual bool HaveAccountsChanged();

  // ChromeIdentity management

  // Returns true if the user is signed in.
  // While the AuthenticationService is in background, this will reload the
  // credentials to ensure the value is up to date.
  virtual bool IsAuthenticated();

  // Returns true if the user is signed in and the identity is considered
  // managed.
  virtual bool IsAuthenticatedIdentityManaged();

  // Returns the email of the authenticated user, or |nil| if the user is not
  // authenticated.
  virtual NSString* GetAuthenticatedUserEmail();

  // Retrieves the identity of the currently authenticated user or |nil| if
  // either the user is not authenticated, or is authenticated through
  // ClientLogin.
  // Virtual for testing.
  virtual ChromeIdentity* GetAuthenticatedIdentity();

  // Signs |identity| in to Chrome with |hosted_domain| as its hosted domain,
  // pauses sync and logs |identity| in to http://google.com. If |identity| has
  // no hosted domain, |hosted_domain| should be empty.
  // Virtual for testing.
  virtual void SignIn(ChromeIdentity* identity,
                      const std::string& hosted_domain);

  // Signs the authenticated user out of Chrome.
  // Virtual for testing.
  virtual void SignOut(signin_metrics::ProfileSignout signout_source,
                       ProceduralBlock completion);

  // Returns whether there is a cached associated MDM error for |identity|.
  bool HasCachedMDMErrorForIdentity(ChromeIdentity* identity);

  // Shows the MDM Error dialog for |identity| if it has an associated MDM
  // error. Returns true if |identity| had an associated error, false otherwise.
  bool ShowMDMErrorDialogForIdentity(ChromeIdentity* identity);

  // Resets the ChromeIdentityService observer to the one available in the
  // ChromeBrowserProvider. Used for testing when changing the
  // ChromeIdentityService to or from a fake one.
  void ResetChromeIdentityServiceObserverForTesting();

  // Returns a weak pointer of this.
  base::WeakPtr<AuthenticationService> GetWeakPtr();

 private:
  friend class AuthenticationServiceTest;

  // Method called each time the application enters foreground.
  void OnApplicationEnterForeground();

  // Method called each time the application enters background.
  void OnApplicationEnterBackground();

  // Migrates the token service accounts stored in prefs from emails to account
  // ids.
  void MigrateAccountsStoredInPrefsIfNeeded();

  // Stores the token service accounts in the browser state prefs.
  void StoreAccountsInPrefs();

  // Gets the accounts previously stored in the  browser state prefs.
  std::vector<std::string> GetAccountsInPrefs();

  // Returns the cached MDM infos associated with |identity|. If the cache is
  // stale for |identity|, the entry might be removed.
  NSDictionary* GetCachedMDMInfo(ChromeIdentity* identity);

  // Handles an MDM notification |user_info| associated with |identity|.
  // Returns whether the notification associated with |user_info| was fully
  // handled.
  bool HandleMDMNotification(ChromeIdentity* identity, NSDictionary* user_info);

  // Reloads the accounts to reflect the change in the SSO identities. If
  // |should_store_accounts_| is true, it will also store the available accounts
  // in the  browser state prefs.
  //
  // |should_prompt| indicates whether the user should be prompted with the
  // resign-in infobar if the method signs out.
  void HandleIdentityListChanged(bool should_prompt);

  // Verifies that the authenticated user is still associated with a valid
  // ChromeIdentity. This method must only be called when the user is
  // authenticated with the shared authentication library. If there is no valid
  // ChromeIdentity associated with the currently authenticated user, or the
  // identity is |invalid_identity|, this method will sign the user out.
  //
  // |invalid_identity| is an additional identity to consider invalid. It can be
  // nil if there is no such additional identity to ignore.
  //
  // |should_prompt| indicates whether the user should be prompted with the
  // resign-in infobar if the method signs out.
  void HandleForgottenIdentity(ChromeIdentity* invalid_identity,
                               bool should_prompt);

  // Checks if the authenticated identity was removed by calling
  // |HandleForgottenIdentity|. Reloads the OAuth2 token service accounts if the
  // authenticated identity is still present.
  // |should_prompt| indicates whether the user should be prompted if the
  // authenticated identity was removed.
  void ReloadCredentialsFromIdentities(bool should_prompt);

  // Computes whether the available accounts have changed since the last time
  // they were stored in the  browser state prefs.
  void ComputeHaveAccountsChanged();

  // OAuth2TokenService::Observer implementation.
  void OnEndBatchChanges() override;

  // ChromeIdentityServiceObserver implementation.
  void OnIdentityListChanged() override;
  void OnAccessTokenRefreshFailed(ChromeIdentity* identity,
                                  NSDictionary* user_info) override;
  void OnChromeIdentityServiceWillBeDestroyed() override;

  // The delegate for this AuthenticationService. It is invalid to call any
  // method on this object except Initialize() or Shutdown() if this pointer
  // is null.
  std::unique_ptr<AuthenticationServiceDelegate> delegate_;

  // Pointer to the KeyedServices used by AuthenticationService.
  PrefService* pref_service_ = nullptr;
  ProfileOAuth2TokenService* token_service_ = nullptr;
  SyncSetupService* sync_setup_service_ = nullptr;
  AccountTrackerService* account_tracker_ = nullptr;
  SigninManager* signin_manager_ = nullptr;
  browser_sync::ProfileSyncService* sync_service_ = nullptr;

  // Whether Initialized has been called.
  bool initialized_ = false;

  // Whether the accounts have changed while the AuthenticationService was in
  // background. When the AuthenticationService is in background, this value
  // cannot be trusted.
  bool have_accounts_changed_ = false;

  // Whether the AuthenticationService behaves as being in foreground. In
  // background, identities changes aren't always notified and can't be
  // initiated by the user.
  bool is_in_foreground_ = false;

  // Whether the AuthenticationService is currently reloading credentials, used
  // to avoid an infinite reloading loop.
  bool is_reloading_credentials_ = false;

  // Map between account IDs and their associated MDM error.
  std::map<std::string, NSDictionary*> cached_mdm_infos_;

  id foreground_observer_ = nil;
  id background_observer_ = nil;

  ScopedObserver<ios::ChromeIdentityService,
                 ios::ChromeIdentityService::Observer>
      identity_service_observer_;

  base::WeakPtrFactory<AuthenticationService> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticationService);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_
