// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_MANAGER_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"

#if !defined(OS_CHROMEOS)
#include "components/signin/core/browser/signin_manager.h"
#endif

// Necessary to declare this class as a friend.
namespace arc {
class ArcTermsOfServiceDefaultNegotiatorTest;
}

// Necessary to declare this class as a friend.
namespace browser_sync {
class ProfileSyncServiceStartupCrosTest;
}

// Necessary to declare these classes as friends.
namespace chromeos {
class ChromeSessionManager;
class UserSessionManager;
}

// Necessary to declare this class as a friend.
namespace file_manager {
class MultiProfileFileManagerBrowserTest;
}

// Necessary to declare these classes as friends.
class ArcSupportHostTest;
class MultiProfileDownloadNotificationTest;
class ProfileSyncServiceHarness;

namespace identity {

// Gives access to information about the user's Google identities. See
// ./README.md for detailed documentation.
class IdentityManager : public SigninManagerBase::Observer,
#if !defined(OS_CHROMEOS)
                        public SigninManager::DiagnosticsClient,
#endif
                        public OAuth2TokenService::DiagnosticsObserver {
 public:
  class Observer {
   public:
    Observer() = default;
    virtual ~Observer() = default;

    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    // Called when an account becomes the user's primary account.
    // This method is not called during a reauth.
    virtual void OnPrimaryAccountSet(const AccountInfo& primary_account_info) {}

    // Called when when the user moves from having a primary account to no
    // longer having a primary account.
    virtual void OnPrimaryAccountCleared(
        const AccountInfo& previous_primary_account_info) {}

    // TODO(blundell): Eventually we might need a callback for failure to log in
    // to the primary account.
  };

  // Observer interface for classes that want to monitor status of various
  // requests. Mostly useful in tests and debugging contexts (e.g., WebUI).
  class DiagnosticsObserver {
   public:
    DiagnosticsObserver() = default;
    virtual ~DiagnosticsObserver() = default;

    DiagnosticsObserver(const DiagnosticsObserver&) = delete;
    DiagnosticsObserver& operator=(const DiagnosticsObserver&) = delete;

    // Called when receiving request for access token.
    virtual void OnAccessTokenRequested(
        const std::string& account_id,
        const std::string& consumer_id,
        const OAuth2TokenService::ScopeSet& scopes) {}
  };

  IdentityManager(SigninManagerBase* signin_manager,
                  ProfileOAuth2TokenService* token_service);
  ~IdentityManager() override;

  // Provides access to the latest cached information of the user's primary
  // account.
  AccountInfo GetPrimaryAccountInfo();

  // Returns whether the primary account is available, according to the latest
  // cached information. Simple convenience wrapper over checking whether the
  // primary account info has a valid account ID.
  bool HasPrimaryAccount();

  // Creates a PrimaryAccountAccessTokenFetcher given the passed-in information.
  std::unique_ptr<PrimaryAccountAccessTokenFetcher>
  CreateAccessTokenFetcherForPrimaryAccount(
      const std::string& oauth_consumer_name,
      const OAuth2TokenService::ScopeSet& scopes,
      PrimaryAccountAccessTokenFetcher::TokenCallback callback,
      PrimaryAccountAccessTokenFetcher::Mode mode);

  // If an entry exists in the Identity Service's cache corresponding to the
  // given information, removes that entry; in this case, the next access token
  // request for |account_id| and |scopes| will fetch a new token from the
  // network. Otherwise, is a no-op.
  void RemoveAccessTokenFromCache(const AccountInfo& account_info,
                                  const OAuth2TokenService::ScopeSet& scopes,
                                  const std::string& access_token);

  // Methods to register or remove observers.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void AddDiagnosticsObserver(DiagnosticsObserver* observer);
  void RemoveDiagnosticsObserver(DiagnosticsObserver* observer);

 private:
  // These clients need to call SetPrimaryAccountSynchronouslyForTests().
  friend void MakePrimaryAccountAvailable(
      SigninManagerBase* signin_manager,
      ProfileOAuth2TokenService* token_service,
      IdentityManager* identity_manager,
      const std::string& email);
  friend MultiProfileDownloadNotificationTest;
  friend ProfileSyncServiceHarness;
  friend file_manager::MultiProfileFileManagerBrowserTest;

  // These clients needs to call SetPrimaryAccountSynchronously().
  friend ArcSupportHostTest;
  friend arc::ArcTermsOfServiceDefaultNegotiatorTest;
  friend chromeos::ChromeSessionManager;
  friend chromeos::UserSessionManager;
  friend browser_sync::ProfileSyncServiceStartupCrosTest;

  // Sets the primary account info synchronously with both the IdentityManager
  // and its backing SigninManager/ProfileOAuth2TokenService instances.
  // Prefer using the methods in identity_test_{environment, utils}.h to using
  // this method directly.
  void SetPrimaryAccountSynchronouslyForTests(const std::string& gaia_id,
                                              const std::string& email_address,
                                              const std::string& refresh_token);

  // Sets the primary account info synchronously with both the IdentityManager
  // and its backing SigninManager instance. If |refresh_token| is not empty,
  // sets the refresh token with the backing ProfileOAuth2TokenService
  // instance. This method should not be used directly; it exists only to serve
  // one legacy use case at this point.
  // TODO(https://crbug.com/814787): Eliminate the need for this method.
  void SetPrimaryAccountSynchronously(const std::string& gaia_id,
                                      const std::string& email_address,
                                      const std::string& refresh_token);

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const AccountInfo& account_info) override;
  void GoogleSignedOut(const AccountInfo& account_info) override;

#if !defined(OS_CHROMEOS)
  // SigninManager::DiagnosticsClient:
  // Override these to update |primary_account_info_| before any observers of
  // SigninManager are notified of the signin state change, ensuring that any
  // such observer flows that eventually interact with IdentityManager observe
  // its state as being consistent with that of SigninManager.
  void WillFireGoogleSigninSucceeded(const AccountInfo& account_info) override;
  void WillFireGoogleSignedOut(const AccountInfo& account_info) override;
#endif

  // OAuth2TokenService::DiagnosticsObserver:
  void OnAccessTokenRequested(
      const std::string& account_id,
      const std::string& consumer_id,
      const OAuth2TokenService::ScopeSet& scopes) override;

  // Removes synchronously token from token_service
  void HandleRemoveAccessTokenFromCache(
      const std::string& account_id,
      const OAuth2TokenService::ScopeSet& scopes,
      const std::string& access_token);

  // Notifies diagnostics observers. Invoked asynchronously from
  // OnAccessTokenRequested() to mimic the effect of receiving this call
  // asynchronously from the Identity Service.
  void HandleOnAccessTokenRequested(const std::string& account_id,
                                    const std::string& consumer_id,
                                    const OAuth2TokenService::ScopeSet& scopes);

  // Backing signin classes. NOTE: We strive to limit synchronous access to
  // these classes in the IdentityManager implementation, as all such
  // synchronous access will become impossible when IdentityManager is backed by
  // the Identity Service.
  SigninManagerBase* signin_manager_;
  ProfileOAuth2TokenService* token_service_;

  // The latest (cached) value of the primary account.
  AccountInfo primary_account_info_;

  // Lists of observers.
  // Makes sure lists are empty on destruction.
  base::ObserverList<Observer, true> observer_list_;
  base::ObserverList<DiagnosticsObserver, true> diagnostics_observer_list_;

  base::WeakPtrFactory<IdentityManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(IdentityManager);
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_MANAGER_H_
