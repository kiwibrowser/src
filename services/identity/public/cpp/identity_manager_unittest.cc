// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/identity_manager.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace identity {
namespace {

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

const char kTestGaiaId[] = "dummyId";
const char kTestEmail[] = "me@gmail.com";

#if defined(OS_CHROMEOS)
const char kTestEmailWithPeriod[] = "m.e@gmail.com";
#endif

// Subclass of FakeProfileOAuth2TokenService with bespoke behavior.
class CustomFakeProfileOAuth2TokenService
    : public FakeProfileOAuth2TokenService {
 public:
  void set_on_access_token_invalidated_info(
      std::string expected_account_id_to_invalidate,
      std::set<std::string> expected_scopes_to_invalidate,
      std::string expected_access_token_to_invalidate,
      base::OnceClosure callback) {
    expected_account_id_to_invalidate_ = expected_account_id_to_invalidate;
    expected_scopes_to_invalidate_ = expected_scopes_to_invalidate;
    expected_access_token_to_invalidate_ = expected_access_token_to_invalidate;
    on_access_token_invalidated_callback_ = std::move(callback);
  }

 private:
  // OAuth2TokenService:
  void InvalidateAccessTokenImpl(const std::string& account_id,
                                 const std::string& client_id,
                                 const ScopeSet& scopes,
                                 const std::string& access_token) override {
    if (on_access_token_invalidated_callback_) {
      EXPECT_EQ(expected_account_id_to_invalidate_, account_id);
      EXPECT_EQ(expected_scopes_to_invalidate_, scopes);
      EXPECT_EQ(expected_access_token_to_invalidate_, access_token);
      std::move(on_access_token_invalidated_callback_).Run();
    }
  }

  std::string expected_account_id_to_invalidate_;
  std::set<std::string> expected_scopes_to_invalidate_;
  std::string expected_access_token_to_invalidate_;
  base::OnceClosure on_access_token_invalidated_callback_;
};

class AccountTrackerServiceForTest : public AccountTrackerService {
 public:
  void SetAccountStateFromUserInfo(const std::string& account_id,
                                   const base::DictionaryValue* user_info) {
    AccountTrackerService::SetAccountStateFromUserInfo(account_id, user_info);
  }
};

class TestSigninManagerObserver : public SigninManagerBase::Observer {
 public:
  explicit TestSigninManagerObserver(SigninManagerBase* signin_manager)
      : signin_manager_(signin_manager) {
    signin_manager_->AddObserver(this);
  }
  ~TestSigninManagerObserver() override {
    signin_manager_->RemoveObserver(this);
  }

  void set_identity_manager(IdentityManager* identity_manager) {
    identity_manager_ = identity_manager;
  }

  void set_on_google_signin_succeeded_callback(base::OnceClosure callback) {
    on_google_signin_succeeded_callback_ = std::move(callback);
  }
  void set_on_google_signed_out_callback(base::OnceClosure callback) {
    on_google_signed_out_callback_ = std::move(callback);
  }

  const AccountInfo& primary_account_from_signin_callback() {
    return primary_account_from_signin_callback_;
  }
  const AccountInfo& primary_account_from_signout_callback() {
    return primary_account_from_signout_callback_;
  }

 private:
  // SigninManager::Observer:
  void GoogleSigninSucceeded(const AccountInfo& account_info) override {
    ASSERT_TRUE(identity_manager_);
    primary_account_from_signin_callback_ =
        identity_manager_->GetPrimaryAccountInfo();
    if (on_google_signin_succeeded_callback_)
      std::move(on_google_signin_succeeded_callback_).Run();
  }
  void GoogleSignedOut(const AccountInfo& account_info) override {
    ASSERT_TRUE(identity_manager_);
    primary_account_from_signout_callback_ =
        identity_manager_->GetPrimaryAccountInfo();
    if (on_google_signed_out_callback_)
      std::move(on_google_signed_out_callback_).Run();
  }

  SigninManagerBase* signin_manager_;
  IdentityManager* identity_manager_;
  base::OnceClosure on_google_signin_succeeded_callback_;
  base::OnceClosure on_google_signed_out_callback_;
  AccountInfo primary_account_from_signin_callback_;
  AccountInfo primary_account_from_signout_callback_;
};

class TestIdentityManagerObserver : IdentityManager::Observer {
 public:
  explicit TestIdentityManagerObserver(IdentityManager* identity_manager)
      : identity_manager_(identity_manager) {
    identity_manager_->AddObserver(this);
  }
  ~TestIdentityManagerObserver() override {
    identity_manager_->RemoveObserver(this);
  }

  void set_on_primary_account_set_callback(base::OnceClosure callback) {
    on_primary_account_set_callback_ = std::move(callback);
  }
  void set_on_primary_account_cleared_callback(base::OnceClosure callback) {
    on_primary_account_cleared_callback_ = std::move(callback);
  }

  const AccountInfo& primary_account_from_set_callback() {
    return primary_account_from_set_callback_;
  }
  const AccountInfo& primary_account_from_cleared_callback() {
    return primary_account_from_cleared_callback_;
  }

 private:
  // IdentityManager::Observer:
  void OnPrimaryAccountSet(const AccountInfo& primary_account_info) override {
    primary_account_from_set_callback_ = primary_account_info;
    if (on_primary_account_set_callback_)
      std::move(on_primary_account_set_callback_).Run();
  }
  void OnPrimaryAccountCleared(
      const AccountInfo& previous_primary_account_info) override {
    primary_account_from_cleared_callback_ = previous_primary_account_info;
    if (on_primary_account_cleared_callback_)
      std::move(on_primary_account_cleared_callback_).Run();
  }

  IdentityManager* identity_manager_;
  base::OnceClosure on_primary_account_set_callback_;
  base::OnceClosure on_primary_account_cleared_callback_;
  AccountInfo primary_account_from_set_callback_;
  AccountInfo primary_account_from_cleared_callback_;
};

class TestIdentityManagerDiagnosticsObserver
    : IdentityManager::DiagnosticsObserver {
 public:
  explicit TestIdentityManagerDiagnosticsObserver(
      IdentityManager* identity_manager)
      : identity_manager_(identity_manager) {
    identity_manager_->AddDiagnosticsObserver(this);
  }
  ~TestIdentityManagerDiagnosticsObserver() override {
    identity_manager_->RemoveDiagnosticsObserver(this);
  }

  void set_on_access_token_requested_callback(base::OnceClosure callback) {
    on_access_token_requested_callback_ = std::move(callback);
  }

  const std::string& token_requestor_account_id() {
    return token_requestor_account_id_;
  }
  const std::string& token_requestor_consumer_id() {
    return token_requestor_consumer_id_;
  }
  const OAuth2TokenService::ScopeSet& token_requestor_scopes() {
    return token_requestor_scopes_;
  }

 private:
  // IdentityManager::DiagnosticsObserver:
  void OnAccessTokenRequested(
      const std::string& account_id,
      const std::string& consumer_id,
      const OAuth2TokenService::ScopeSet& scopes) override {
    token_requestor_account_id_ = account_id;
    token_requestor_consumer_id_ = consumer_id;
    token_requestor_scopes_ = scopes;
    std::move(on_access_token_requested_callback_).Run();
  }

  IdentityManager* identity_manager_;
  base::OnceClosure on_access_token_requested_callback_;
  std::string token_requestor_account_id_;
  std::string token_requestor_consumer_id_;
  OAuth2TokenService::ScopeSet token_requestor_scopes_;
};

}  // namespace

class IdentityManagerTest : public testing::Test {
 public:
  IdentityManagerTest()
      : signin_client_(&pref_service_),
#if defined(OS_CHROMEOS)
        signin_manager_(&signin_client_, &account_tracker_)
#else
        signin_manager_(&signin_client_,
                        &token_service_,
                        &account_tracker_,
                        nullptr)
#endif
  {
    AccountTrackerService::RegisterPrefs(pref_service_.registry());
    SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
    SigninManagerBase::RegisterPrefs(pref_service_.registry());
    signin::RegisterAccountConsistencyProfilePrefs(pref_service_.registry());
    signin::SetGaiaOriginIsolatedCallback(
        base::BindRepeating([] { return true; }));

    account_tracker_.Initialize(&signin_client_);

    signin_manager()->SetAuthenticatedAccountInfo(kTestGaiaId, kTestEmail);

    RecreateIdentityManager();
  }

  IdentityManager* identity_manager() { return identity_manager_.get(); }
  TestIdentityManagerObserver* identity_manager_observer() {
    return identity_manager_observer_.get();
  }
  TestIdentityManagerDiagnosticsObserver*
  identity_manager_diagnostics_observer() {
    return identity_manager_diagnostics_observer_.get();
  }
  AccountTrackerServiceForTest* account_tracker() { return &account_tracker_; }
  SigninManagerForTest* signin_manager() { return &signin_manager_; }
  CustomFakeProfileOAuth2TokenService* token_service() {
    return &token_service_;
  }

  // Used by some tests that need to re-instantiate IdentityManager after
  // performing some other setup.
  void RecreateIdentityManager() {
    // Reset them all to null first to ensure that they're destroyed, as
    // otherwise SigninManager ends up getting a new DiagnosticsObserver added
    // before the old one is removed.
    identity_manager_observer_.reset();
    identity_manager_diagnostics_observer_.reset();
    identity_manager_.reset();

    identity_manager_.reset(
        new IdentityManager(&signin_manager_, &token_service_));
    identity_manager_observer_.reset(
        new TestIdentityManagerObserver(identity_manager_.get()));
    identity_manager_diagnostics_observer_.reset(
        new TestIdentityManagerDiagnosticsObserver(identity_manager_.get()));
  }

 private:
  base::MessageLoop message_loop_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  AccountTrackerServiceForTest account_tracker_;
  TestSigninClient signin_client_;
  SigninManagerForTest signin_manager_;
  CustomFakeProfileOAuth2TokenService token_service_;
  std::unique_ptr<IdentityManager> identity_manager_;
  std::unique_ptr<TestIdentityManagerObserver> identity_manager_observer_;
  std::unique_ptr<TestIdentityManagerDiagnosticsObserver>
      identity_manager_diagnostics_observer_;

  DISALLOW_COPY_AND_ASSIGN(IdentityManagerTest);
};

// Test that IdentityManager starts off with the information in SigninManager.
TEST_F(IdentityManagerTest, PrimaryAccountInfoAtStartup) {
  AccountInfo primary_account_info =
      identity_manager()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmail, primary_account_info.email);
}

// Signin/signout tests aren't relevant and cannot build on ChromeOS, which
// doesn't support signin/signout.
#if !defined(OS_CHROMEOS)
// Test that the user signing in results in firing of the IdentityManager
// observer callback and the IdentityManager's state being updated.
TEST_F(IdentityManagerTest, PrimaryAccountInfoAfterSignin) {
  base::RunLoop run_loop;
  identity_manager_observer()->set_on_primary_account_set_callback(
      run_loop.QuitClosure());

  signin_manager()->SignIn(kTestGaiaId, kTestEmail, "password");
  run_loop.Run();

  AccountInfo primary_account_from_set_callback =
      identity_manager_observer()->primary_account_from_set_callback();
  EXPECT_EQ(kTestGaiaId, primary_account_from_set_callback.gaia);
  EXPECT_EQ(kTestEmail, primary_account_from_set_callback.email);

  AccountInfo primary_account_info =
      identity_manager()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmail, primary_account_info.email);
}

// Test that the user signing out results in firing of the IdentityManager
// observer callback and the IdentityManager's state being updated.
TEST_F(IdentityManagerTest, PrimaryAccountInfoAfterSigninAndSignout) {
  // First ensure that the user is signed in from the POV of the
  // IdentityManager.
  base::RunLoop run_loop;
  identity_manager_observer()->set_on_primary_account_set_callback(
      run_loop.QuitClosure());
  signin_manager()->SignIn(kTestGaiaId, kTestEmail, "password");
  run_loop.Run();

  // Sign the user out and check that the IdentityManager responds
  // appropriately.
  base::RunLoop run_loop2;
  identity_manager_observer()->set_on_primary_account_cleared_callback(
      run_loop2.QuitClosure());

  signin_manager()->ForceSignOut();
  run_loop2.Run();

  AccountInfo primary_account_from_cleared_callback =
      identity_manager_observer()->primary_account_from_cleared_callback();
  EXPECT_EQ(kTestGaiaId, primary_account_from_cleared_callback.gaia);
  EXPECT_EQ(kTestEmail, primary_account_from_cleared_callback.email);

  AccountInfo primary_account_info =
      identity_manager()->GetPrimaryAccountInfo();
  EXPECT_EQ("", primary_account_info.gaia);
  EXPECT_EQ("", primary_account_info.email);
}
#endif  // !defined(OS_CHROMEOS)

TEST_F(IdentityManagerTest, HasPrimaryAccount) {
  EXPECT_TRUE(identity_manager()->HasPrimaryAccount());

#if !defined(OS_CHROMEOS)
  base::RunLoop run_loop;
  identity_manager_observer()->set_on_primary_account_cleared_callback(
      run_loop.QuitClosure());

  signin_manager()->ForceSignOut();
  run_loop.Run();
  EXPECT_FALSE(identity_manager()->HasPrimaryAccount());
#endif
}

TEST_F(IdentityManagerTest, RemoveAccessTokenFromCache) {
  std::set<std::string> scopes{"scope"};
  std::string access_token = "access_token";

  signin_manager()->SetAuthenticatedAccountInfo(kTestGaiaId, kTestEmail);
  std::string account_id = signin_manager()->GetAuthenticatedAccountId();
  token_service()->UpdateCredentials(account_id, "refresh_token");

  base::RunLoop run_loop;
  token_service()->set_on_access_token_invalidated_info(
      account_id, scopes, access_token, run_loop.QuitClosure());

  AccountInfo account_info;
  account_info.account_id = account_id;
  account_info.gaia = kTestGaiaId;
  account_info.email = kTestEmail;
  identity_manager()->RemoveAccessTokenFromCache(account_info, scopes,
                                                 access_token);

  run_loop.Run();
}

TEST_F(IdentityManagerTest, CreateAccessTokenFetcherForPrimaryAccount) {
  std::set<std::string> scopes{"scope"};
  PrimaryAccountAccessTokenFetcher::TokenCallback callback =
      base::BindOnce([](const GoogleServiceAuthError& error,
                        const std::string& access_token) {});
  std::unique_ptr<PrimaryAccountAccessTokenFetcher> token_fetcher =
      identity_manager()->CreateAccessTokenFetcherForPrimaryAccount(
          "dummy_consumer", scopes, std::move(callback),
          PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);
  EXPECT_TRUE(token_fetcher);
}

TEST_F(IdentityManagerTest, ObserveAccessTokenFetch) {
  base::RunLoop run_loop;
  identity_manager_diagnostics_observer()
      ->set_on_access_token_requested_callback(run_loop.QuitClosure());

  signin_manager()->SetAuthenticatedAccountInfo(kTestGaiaId, kTestEmail);
  std::string account_id = signin_manager()->GetAuthenticatedAccountId();
  token_service()->UpdateCredentials(account_id, "refresh_token");

  std::set<std::string> scopes{"scope"};
  PrimaryAccountAccessTokenFetcher::TokenCallback callback =
      base::BindOnce([](const GoogleServiceAuthError& error,
                        const std::string& access_token) {});
  std::unique_ptr<PrimaryAccountAccessTokenFetcher> token_fetcher =
      identity_manager()->CreateAccessTokenFetcherForPrimaryAccount(
          "dummy_consumer", scopes, std::move(callback),
          PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  EXPECT_EQ(
      account_id,
      identity_manager_diagnostics_observer()->token_requestor_account_id());
  EXPECT_EQ(
      "dummy_consumer",
      identity_manager_diagnostics_observer()->token_requestor_consumer_id());
  EXPECT_EQ(scopes,
            identity_manager_diagnostics_observer()->token_requestor_scopes());
}

#if !defined(OS_CHROMEOS)
TEST_F(IdentityManagerTest,
       IdentityManagerGetsSignInEventBeforeSigninManagerObserver) {
  signin_manager()->ForceSignOut();

  base::RunLoop run_loop;
  TestSigninManagerObserver signin_manager_observer(signin_manager());
  signin_manager_observer.set_on_google_signin_succeeded_callback(
      run_loop.QuitClosure());

  // NOTE: For this test to be meaningful, TestSigninManagerObserver
  // needs to be created before the IdentityManager instance that it's
  // interacting with. Otherwise, even an implementation where they're
  // both SigninManager::Observers would work as IdentityManager would
  // get notified first during the observer callbacks.
  RecreateIdentityManager();
  signin_manager_observer.set_identity_manager(identity_manager());

  signin_manager()->SignIn(kTestGaiaId, kTestEmail, "password");
  run_loop.Run();

  AccountInfo primary_account_from_signin_callback =
      signin_manager_observer.primary_account_from_signin_callback();
  EXPECT_EQ(kTestGaiaId, primary_account_from_signin_callback.gaia);
  EXPECT_EQ(kTestEmail, primary_account_from_signin_callback.email);
}

TEST_F(IdentityManagerTest,
       IdentityManagerGetsSignOutEventBeforeSigninManagerObserver) {
  base::RunLoop run_loop;
  TestSigninManagerObserver signin_manager_observer(signin_manager());
  signin_manager_observer.set_on_google_signed_out_callback(
      run_loop.QuitClosure());

  // NOTE: For this test to be meaningful, TestSigninManagerObserver
  // needs to be created before the IdentityManager instance that it's
  // interacting with. Otherwise, even an implementation where they're
  // both SigninManager::Observers would work as IdentityManager would
  // get notified first during the observer callbacks.
  RecreateIdentityManager();
  signin_manager_observer.set_identity_manager(identity_manager());

  signin_manager()->ForceSignOut();
  run_loop.Run();

  AccountInfo primary_account_from_signout_callback =
      signin_manager_observer.primary_account_from_signout_callback();
  EXPECT_EQ(std::string(), primary_account_from_signout_callback.gaia);
  EXPECT_EQ(std::string(), primary_account_from_signout_callback.email);
}
#endif

#if defined(OS_CHROMEOS)
// On ChromeOS, AccountTrackerService first receives the normalized email
// address from GAIA and then later has it updated with the user's
// originally-specified version of their email address (at the time of that
// address' creation). This latter will differ if the user's originally-
// specified address was not in normalized form (e.g., if it contained
// periods). This test simulates such a flow in order to verify that
// IdentityManager correctly reflects the updated version. See crbug.com/842041
// and crbug.com/842670 for further details.
TEST_F(IdentityManagerTest, IdentityManagerReflectsUpdatedEmailAddress) {
  AccountInfo primary_account_info =
      identity_manager()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmail, primary_account_info.email);

  // Simulate the flow wherein the user's email address was updated
  // to the originally-created non-normalized version.
  base::DictionaryValue user_info;
  user_info.SetString("id", kTestGaiaId);
  user_info.SetString("email", kTestEmailWithPeriod);
  account_tracker()->SetAccountStateFromUserInfo(
      primary_account_info.account_id, &user_info);

  // Verify that IdentityManager reflects the update.
  primary_account_info = identity_manager()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmailWithPeriod, primary_account_info.email);
}
#endif

}  // namespace identity
