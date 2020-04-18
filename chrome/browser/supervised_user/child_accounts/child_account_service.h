// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_
#define CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/callback_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/supervised_user/child_accounts/family_info_fetcher.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "net/base/backoff_entry.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class Profile;

// This class handles detection of child accounts (on sign-in as well as on
// browser restart), and triggers the appropriate behavior (e.g. enable the
// supervised user experience, fetch information about the parent(s)).
class ChildAccountService : public KeyedService,
                            public FamilyInfoFetcher::Consumer,
                            public AccountTrackerService::Observer,
                            public SupervisedUserService::Delegate,
                            public GaiaCookieManagerService::Observer {
 public:
  enum class AuthState { AUTHENTICATED, NOT_AUTHENTICATED, PENDING };

  ~ChildAccountService() override;

  static bool IsChildAccountDetectionEnabled();

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void Init();

  // Responds whether at least one request for child status was successful.
  // And we got answer whether the profile belongs to a child account or not.
  bool IsChildAccountStatusKnown();

  // KeyedService:
  void Shutdown() override;

  void AddChildStatusReceivedCallback(const base::Closure& callback);

  // Returns whether or not the user is authenticated on Google web properties
  // based on the state of the cookie jar. Returns AuthState::PENDING if
  // authentication state can't be determined at the moment.
  AuthState GetGoogleAuthState();

  // Subscribes to changes to the Google authentication state
  // (see IsGoogleAuthenticated()). Can send a notification even if the
  // authentication state has not changed.
  std::unique_ptr<base::CallbackList<void()>::Subscription>
  ObserveGoogleAuthState(const base::Callback<void()>& callback);

 private:
  friend class ChildAccountServiceFactory;
  // Use |ChildAccountServiceFactory::GetForProfile(...)| to get an instance of
  // this service.
  explicit ChildAccountService(Profile* profile);

  // SupervisedUserService::Delegate implementation.
  bool SetActive(bool active) override;

  // Sets whether the signed-in account is a child account.
  void SetIsChildAccount(bool is_child_account);

  // AccountTrackerService::Observer implementation.
  void OnAccountUpdated(const AccountInfo& info) override;
  void OnAccountRemoved(const AccountInfo& info) override;

  // FamilyInfoFetcher::Consumer implementation.
  void OnGetFamilyMembersSuccess(
      const std::vector<FamilyInfoFetcher::FamilyMember>& members) override;
  void OnFailure(FamilyInfoFetcher::ErrorCode error) override;

  // GaiaCookieManagerService::Observer implementation.
  void OnGaiaAccountsInCookieUpdated(
      const std::vector<gaia::ListedAccount>& accounts,
      const std::vector<gaia::ListedAccount>& signed_out_accounts,
      const GoogleServiceAuthError& error) override;

  void StartFetchingFamilyInfo();
  void CancelFetchingFamilyInfo();
  void ScheduleNextFamilyInfoUpdate(base::TimeDelta delay);

  void PropagateChildStatusToUser(bool is_child);

  void SetFirstCustodianPrefs(const FamilyInfoFetcher::FamilyMember& custodian);
  void SetSecondCustodianPrefs(
      const FamilyInfoFetcher::FamilyMember& custodian);
  void ClearFirstCustodianPrefs();
  void ClearSecondCustodianPrefs();

  // Owns us via the KeyedService mechanism.
  Profile* profile_;

  bool active_;

  std::unique_ptr<FamilyInfoFetcher> family_fetcher_;
  // If fetching the family info fails, retry with exponential backoff.
  base::OneShotTimer family_fetch_timer_;
  net::BackoffEntry family_fetch_backoff_;

  GaiaCookieManagerService* gaia_cookie_manager_;

  base::CallbackList<void()> google_auth_state_observers_;

  // Callbacks to run when the user status becomes known.
  std::vector<base::Closure> status_received_callback_list_;

  base::WeakPtrFactory<ChildAccountService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChildAccountService);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_
