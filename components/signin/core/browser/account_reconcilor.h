// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace signin {
class AccountReconcilorDelegate;
}

class ProfileOAuth2TokenService;
class SigninClient;

class AccountReconcilor : public KeyedService,
                          public content_settings::Observer,
                          public GaiaCookieManagerService::Observer,
                          public OAuth2TokenService::Observer {
 public:
  // When an instance of this class exists, the account reconcilor is suspended.
  // It will automatically restart when all instances of Lock have been
  // destroyed.
  class Lock final {
   public:
    explicit Lock(AccountReconcilor* reconcilor);
    ~Lock();

   private:
    AccountReconcilor* reconcilor_;
    THREAD_CHECKER(thread_checker_);
    DISALLOW_COPY_AND_ASSIGN(Lock);
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // The typical order of events is:
    // - OnStartReconcile() called at the beginning of StartReconcile().
    // - When reconcile is blocked:
    //   1. current reconcile is aborted with AbortReconcile(),
    //   2. OnBlockReconcile() is called.
    // - When reconcile is unblocked:
    //   1. OnUnblockReconcile() is called,
    //   2. reconcile is restarted if needed with StartReconcile(), which
    //     triggers a call to OnStartReconcile().

    // Called whe reconcile starts.
    virtual void OnStartReconcile() {}
    // Called when the AccountReconcilor is blocked.
    virtual void OnBlockReconcile() {}
    // Called when the AccountReconcilor is unblocked.
    virtual void OnUnblockReconcile() {}
  };

  AccountReconcilor(
      ProfileOAuth2TokenService* token_service,
      SigninManagerBase* signin_manager,
      SigninClient* client,
      GaiaCookieManagerService* cookie_manager_service,
      std::unique_ptr<signin::AccountReconcilorDelegate> delegate);
  ~AccountReconcilor() override;

  // Initializes the account reconcilor. Should be called once after
  // construction.
  void Initialize(bool start_reconcile_if_tokens_available);

  // Enables and disables the reconciliation.
  void EnableReconcile();
  void DisableReconcile(bool logout_all_gaia_accounts);

  // Signal that an X-Chrome-Manage-Accounts was received from GAIA. Pass the
  // ServiceType specified by GAIA in the 204 response.
  // Virtual for testing.
  virtual void OnReceivedManageAccountsResponse(
      signin::GAIAServiceType service_type);

  // KeyedService implementation.
  void Shutdown() override;

  // Determine what the reconcilor is currently doing.
  signin_metrics::AccountReconcilorState GetState();

  // Adds ands removes observers.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  friend class Lock;
  friend class AccountReconcilorTest;
  friend class DiceBrowserTestBase;
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, SigninManagerRegistration);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, Reauth);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, ProfileAlreadyConnected);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTestDice, TableRowTest);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DiceTokenServiceRegistration);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DiceReconcileWhithoutSignin);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DiceReconcileNoop);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DiceLastKnownFirstAccount);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, UnverifiedAccountNoop);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, UnverifiedAccountMerge);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, HandleSigninDuringReconcile);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DiceMigrationAfterNoop);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           DiceNoMigrationWhenTokensNotReady);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           DiceNoMigrationAfterReconcile);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           DiceReconcileReuseGaiaFirstAccount);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           MigrationClearSecondaryTokens);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, MigrationClearAllTokens);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, TokensNotLoaded);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileCookiesDisabled);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileContentSettings);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileContentSettingsGaiaUrl);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileContentSettingsNonGaiaUrl);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileContentSettingsInvalidPattern);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, GetAccountsFromCookieSuccess);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, GetAccountsFromCookieFailure);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileNoop);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileNoopWithDots);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileNoopMultiple);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileAddToCookie);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, AuthErrorTriggersListAccount);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           SignoutAfterErrorDoesNotRecordUma);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileRemoveFromCookie);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileAddToCookieTwice);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileBadPrimary);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, StartReconcileOnlyOnce);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, Lock);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           StartReconcileWithSessionInfoExpiredDefault);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           AddAccountToCookieCompletedWithBogusAccount);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, NoLoopWithBadPrimary);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, WontMergeAccountsWithError);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DelegateTimeoutIsCalled);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest, DelegateTimeoutIsNotCalled);
  FRIEND_TEST_ALL_PREFIXES(AccountReconcilorTest,
                           DelegateTimeoutIsNotCalledIfTimeoutIsNotReached);

  void set_timer_for_testing(std::unique_ptr<base::Timer> timer);

  bool IsRegisteredWithTokenService() const {
    return registered_with_token_service_;
  }

  // Register and unregister with dependent services.
  void RegisterWithSigninManager();
  void UnregisterWithSigninManager();
  void RegisterWithTokenService();
  void UnregisterWithTokenService();
  void RegisterWithCookieManagerService();
  void UnregisterWithCookieManagerService();
  void RegisterWithContentSettings();
  void UnregisterWithContentSettings();

  // All actions with side effects, only doing meaningful work if account
  // consistency is enabled. Virtual so that they can be overridden in tests.
  virtual void PerformMergeAction(const std::string& account_id);
  virtual void PerformLogoutAllAccountsAction();

  // Used during periodic reconciliation.
  void StartReconcile();
  // |gaia_accounts| are the accounts in the Gaia cookie.
  void FinishReconcile(const std::string& primary_account,
                       const std::vector<std::string>& chrome_accounts,
                       std::vector<gaia::ListedAccount>&& gaia_accounts);
  void AbortReconcile();
  void CalculateIfReconcileIsDone();
  void ScheduleStartReconcileIfChromeAccountsChanged();
  // Revokes tokens for all accounts in chrome_accounts but the primary account.
  void RevokeAllSecondaryTokens(
      const std::string& primary_account,
      const std::vector<std::string>& chrome_accounts);

  // Returns the list of valid accounts from the TokenService.
  std::vector<std::string> LoadValidAccountsFromTokenService() const;

  // Note internally that this |account_id| is added to the cookie jar.
  bool MarkAccountAsAddedToCookie(const std::string& account_id);

  // The reconcilor only starts when the token service is ready.
  bool IsTokenServiceReady();

  // Overriden from content_settings::Observer.
  void OnContentSettingChanged(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      std::string resource_identifier) override;

  // Overriden from GaiaGookieManagerService::Observer.
  void OnAddAccountToCookieCompleted(
      const std::string& account_id,
      const GoogleServiceAuthError& error) override;
  void OnGaiaAccountsInCookieUpdated(
        const std::vector<gaia::ListedAccount>& accounts,
        const std::vector<gaia::ListedAccount>& signed_out_accounts,
        const GoogleServiceAuthError& error) override;

  // Overriden from OAuth2TokenService::Observer.
  void OnEndBatchChanges() override;
  void OnRefreshTokensLoaded() override;
  void OnAuthErrorChanged(const std::string& account_id,
                          const GoogleServiceAuthError& error) override;

  // Lock related methods.
  void IncrementLockCount();
  void DecrementLockCount();
  void BlockReconcile();
  void UnblockReconcile();
  bool IsReconcileBlocked() const;

  void HandleReconcileTimeout();

  std::unique_ptr<signin::AccountReconcilorDelegate> delegate_;

  // The ProfileOAuth2TokenService associated with this reconcilor.
  ProfileOAuth2TokenService* token_service_;

  // The SigninManager associated with this reconcilor.
  SigninManagerBase* signin_manager_;

  // The SigninClient associated with this reconcilor.
  SigninClient* client_;

  // The GaiaCookieManagerService associated with this reconcilor.
  GaiaCookieManagerService* cookie_manager_service_;

  bool registered_with_token_service_;
  bool registered_with_cookie_manager_service_;
  bool registered_with_content_settings_;

  // True while the reconcilor is busy checking or managing the accounts in
  // this profile.
  bool is_reconcile_started_;
  base::Time reconcile_start_time_;

  // True iff this is the first time the reconcilor is executing.
  bool first_execution_;

  // True iff an error occured during the last attempt to reconcile.
  bool error_during_last_reconcile_;

  // Used for Dice migration: migration can happen if the accounts are
  // consistent, which is indicated by reconcile being a no-op.
  bool reconcile_is_noop_;

  // Used during reconcile action.
  // These members are used to validate the tokens in OAuth2TokenService.
  std::vector<std::string> add_to_cookie_;
  bool chrome_accounts_changed_;

  // Used for the Lock.
  // StartReconcile() is blocked while this is > 0.
  int account_reconcilor_lock_count_;
  // StartReconcile() should be started when the reconcilor is unblocked.
  bool reconcile_on_unblock_;

  base::ObserverList<Observer, true> observer_list_;

  // A timer to set off reconciliation timeout handlers, if account
  // reconciliation does not happen in a given timeout duration.
  std::unique_ptr<base::Timer> timer_;
  base::TimeDelta timeout_;

  DISALLOW_COPY_AND_ASSIGN(AccountReconcilor);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_ACCOUNT_RECONCILOR_H_
