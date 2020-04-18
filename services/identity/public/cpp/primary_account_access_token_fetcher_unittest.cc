// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gmock_mutant.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::MockCallback;
using sync_preferences::TestingPrefServiceSyncable;
using testing::CallbackToFunctor;
using testing::InvokeWithoutArgs;
using testing::StrictMock;

#if defined(OS_CHROMEOS)
// ChromeOS doesn't have SigninManager.
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

namespace identity {

namespace {
void OnAccessTokenFetchComplete(base::OnceClosure done_closure,
                                const GoogleServiceAuthError& expected_error,
                                const std::string& expected_access_token,
                                const GoogleServiceAuthError& error,
                                const std::string& access_token) {
  EXPECT_EQ(expected_error, error);
  if (expected_error == GoogleServiceAuthError::AuthErrorNone())
    EXPECT_EQ(expected_access_token, access_token);

  std::move(done_closure).Run();
}

}  // namespace

class PrimaryAccountAccessTokenFetcherTest
    : public testing::Test,
      public OAuth2TokenService::DiagnosticsObserver {
 public:
  using TestTokenCallback =
      StrictMock<MockCallback<PrimaryAccountAccessTokenFetcher::TokenCallback>>;

  PrimaryAccountAccessTokenFetcherTest() : signin_client_(&pref_service_) {
    AccountTrackerService::RegisterPrefs(pref_service_.registry());
#if defined(OS_CHROMEOS)
    SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
    SigninManagerBase::RegisterPrefs(pref_service_.registry());
#else
    SigninManager::RegisterProfilePrefs(pref_service_.registry());
    SigninManager::RegisterPrefs(pref_service_.registry());
#endif  // OS_CHROMEOS

    account_tracker_ = std::make_unique<AccountTrackerService>();
    account_tracker_->Initialize(&signin_client_);

#if defined(OS_CHROMEOS)
    signin_manager_ = std::make_unique<FakeSigninManagerBase>(
        &signin_client_, account_tracker_.get());
#else
    signin_manager_ = std::make_unique<FakeSigninManager>(
        &signin_client_, &token_service_, account_tracker_.get(),
        /*cookie_manager_service=*/nullptr);
#endif  // OS_CHROMEOS
    token_service_.AddDiagnosticsObserver(this);
  }

  ~PrimaryAccountAccessTokenFetcherTest() override {
    token_service_.RemoveDiagnosticsObserver(this);
  }

  std::unique_ptr<PrimaryAccountAccessTokenFetcher> CreateFetcher(
      PrimaryAccountAccessTokenFetcher::TokenCallback callback,
      PrimaryAccountAccessTokenFetcher::Mode mode) {
    std::set<std::string> scopes{"scope"};
    return std::make_unique<PrimaryAccountAccessTokenFetcher>(
        "test_consumer", signin_manager_.get(), &token_service_, scopes,
        std::move(callback), mode);
  }

  FakeProfileOAuth2TokenService* token_service() { return &token_service_; }
  SigninManagerForTest* signin_manager() { return signin_manager_.get(); }

  void SignIn(const std::string& account) {
#if defined(OS_CHROMEOS)
    signin_manager_->SignIn(account);
#else
    signin_manager_->SignIn(account, "user", "pass");
#endif  // OS_CHROMEOS
  }

  void set_on_access_token_request_callback(base::OnceClosure callback) {
    on_access_token_request_callback_ = std::move(callback);
  }

 private:
  // OAuth2TokenService::DiagnosticsObserver:
  void OnAccessTokenRequested(
      const std::string& account_id,
      const std::string& consumer_id,
      const OAuth2TokenService::ScopeSet& scopes) override {
    if (on_access_token_request_callback_)
      std::move(on_access_token_request_callback_).Run();
  }

  base::MessageLoop message_loop_;
  TestingPrefServiceSyncable pref_service_;
  TestSigninClient signin_client_;
  FakeProfileOAuth2TokenService token_service_;

  std::unique_ptr<AccountTrackerService> account_tracker_;
  std::unique_ptr<SigninManagerForTest> signin_manager_;
  base::OnceClosure on_access_token_request_callback_;
};

TEST_F(PrimaryAccountAccessTokenFetcherTest, OneShotShouldReturnAccessToken) {
  TestTokenCallback callback;

  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(), PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  run_loop.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       WaitAndRetryShouldReturnAccessToken) {
  TestTokenCallback callback;

  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldNotReplyIfDestroyed) {
  TestTokenCallback callback;

  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(), PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  run_loop.Run();

  // Destroy the fetcher before the access token request is fulfilled.
  fetcher.reset();

  // Now fulfilling the access token request should have no effect.
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldNotRequestIfDestroyedEarly) {
  TestTokenCallback callback;

  base::RunLoop run_loop;
  set_on_access_token_request_callback(
      base::BindOnce([]() { EXPECT_TRUE(false); }));

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in
  // posting a task to make a request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(), PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  // Destroy the fetcher immediately.
  fetcher.reset();

  // No access token request should occur (i.e., the posted task should not
  // actually execute).
  run_loop.RunUntilIdle();

  // Now fulfilling the access token request should have no effect.
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest, OneShotCallsBackWhenSignedOut) {
  base::RunLoop run_loop;

  // Signed out -> we should get called back.
  auto fetcher = CreateFetcher(
      base::BindOnce(&OnAccessTokenFetchComplete, run_loop.QuitClosure(),
                     GoogleServiceAuthError(
                         GoogleServiceAuthError::State::USER_NOT_SIGNED_UP),
                     ""),
      PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  run_loop.Run();
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       OneShotCallsBackWhenNoRefreshToken) {
  base::RunLoop run_loop;

  SignIn("account");

  // Signed in, but there is no refresh token -> we should get called back.
  auto fetcher = CreateFetcher(
      base::BindOnce(&OnAccessTokenFetchComplete, run_loop.QuitClosure(),
                     GoogleServiceAuthError(
                         GoogleServiceAuthError::State::USER_NOT_SIGNED_UP),
                     ""),
      PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  run_loop.Run();
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       WaitAndRetryNoCallbackWhenSignedOut) {
  TestTokenCallback callback;

  // Signed out -> the fetcher should wait for a sign-in which never happens
  // in this test, so we shouldn't get called back.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);
}

// Tests related to waiting for sign-in don't apply on ChromeOS (it doesn't have
// that concept).
#if !defined(OS_CHROMEOS)

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldWaitForSignIn) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  // Not signed in, so this should wait for a sign-in to complete.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  SignIn("account");

  token_service()->UpdateCredentials("account", "refresh token");

  run_loop.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldWaitForSignInInProgress) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  signin_manager()->set_auth_in_progress("account");

  // A sign-in is currently in progress, so this should wait for the sign-in to
  // complete.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  run_loop.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldWaitForFailedSignIn) {
  TestTokenCallback callback;

  signin_manager()->set_auth_in_progress("account");

  // A sign-in is currently in progress, so this should wait for the sign-in to
  // complete.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  // The fetcher should detect the failed sign-in and call us with an empty
  // access token.
  EXPECT_CALL(
      callback,
      Run(GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED),
          std::string()));

  signin_manager()->FailSignin(
      GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED));
}

#endif  // !OS_CHROMEOS

TEST_F(PrimaryAccountAccessTokenFetcherTest, ShouldWaitForRefreshToken) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");

  // Signed in, but there is no refresh token -> we should not get called back
  // (yet).
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  // Getting a refresh token should result in a request for an access token.
  token_service()->UpdateCredentials("account", "refresh token");

  run_loop.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldIgnoreRefreshTokensForOtherAccounts) {
  TestTokenCallback callback;

  // Signed-in to "account", but there's only a refresh token for a different
  // account.
  SignIn("account");
  token_service()->UpdateCredentials("account 2", "refresh");

  // The fetcher should wait for the correct refresh token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  // A refresh token for yet another account shouldn't matter either.
  token_service()->UpdateCredentials("account 3", "refresh");
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldReturnWhenNoRefreshTokenAvailable) {
  TestTokenCallback callback;

  SignIn("account");

  // Signed in, but there is no refresh token -> we should not get called back
  // (yet).
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  // Getting a refresh token for some other account should have no effect.
  token_service()->UpdateCredentials("different account", "refresh token");

  // When all refresh tokens have been loaded by the token service, but the one
  // for our account wasn't among them, we should get called back with an empty
  // access token.
  EXPECT_CALL(callback, Run(testing::_, std::string()));
  token_service()->LoadCredentials("account doesn't matter");

  // Wait for the task posted by OAuth2TokenService to run.
  base::RunLoop().RunUntilIdle();
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       OneShotCanceledAccessTokenRequest) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  base::RunLoop run_loop2;

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      base::BindOnce(
          &OnAccessTokenFetchComplete, run_loop2.QuitClosure(),
          GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED), ""),
      PrimaryAccountAccessTokenFetcher::Mode::kImmediate);

  run_loop.Run();

  // A canceled access token request should result in a callback.
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));

  run_loop2.Run();
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       WaitAndRetryCanceledAccessTokenRequest) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // Before cancelling the first request, set up to wait for the second request.
  base::RunLoop run_loop2;
  set_on_access_token_request_callback(run_loop2.QuitClosure());

  // A canceled access token request should get retried once.
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));

  run_loop2.Run();

  // Once the access token request is fulfilled, we should get called back with
  // the access token.
  EXPECT_CALL(callback,
              Run(GoogleServiceAuthError::AuthErrorNone(), "access token"));
  token_service()->IssueAllTokensForAccount(
      "account", "access token",
      base::Time::Now() + base::TimeDelta::FromHours(1));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldRetryCanceledAccessTokenRequestOnlyOnce) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // Before cancelling the first request, set up to wait for the second request.
  base::RunLoop run_loop2;
  set_on_access_token_request_callback(run_loop2.QuitClosure());

  // A canceled access token request should get retried once.
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));

  run_loop2.Run();

  // On the second failure, we should get called back with an empty access
  // token.
  EXPECT_CALL(
      callback,
      Run(GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED),
          std::string()));
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
}

#if !defined(OS_CHROMEOS)

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldNotRetryCanceledAccessTokenRequestIfSignedOut) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // Simulate the user signing out while the access token request is pending.
  // In this case, the pending request gets canceled, and the fetcher should
  // *not* retry.
  signin_manager()->ForceSignOut();
  EXPECT_CALL(
      callback,
      Run(GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED),
          std::string()));
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
}

#endif  // !OS_CHROMEOS

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldNotRetryCanceledAccessTokenRequestIfRefreshTokenRevoked) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // Simulate the refresh token getting invalidated. In this case, pending
  // access token requests get canceled, and the fetcher should *not* retry.
  token_service()->RevokeCredentials("account");
  EXPECT_CALL(
      callback,
      Run(GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED),
          std::string()));
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
}

TEST_F(PrimaryAccountAccessTokenFetcherTest,
       ShouldNotRetryFailedAccessTokenRequest) {
  base::RunLoop run_loop;
  set_on_access_token_request_callback(run_loop.QuitClosure());

  TestTokenCallback callback;

  SignIn("account");
  token_service()->UpdateCredentials("account", "refresh token");

  // Signed in and refresh token already exists, so this should result in a
  // request for an access token.
  auto fetcher = CreateFetcher(
      callback.Get(),
      PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);

  run_loop.Run();

  // An access token failure other than "canceled" should not be retried; we
  // should immediately get called back with an empty access token.
  EXPECT_CALL(
      callback,
      Run(GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE),
          std::string()));
  token_service()->IssueErrorForAllPendingRequestsForAccount(
      "account",
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
}

}  // namespace identity
