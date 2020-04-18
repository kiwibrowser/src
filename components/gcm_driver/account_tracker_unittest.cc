// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/account_tracker.h"

#include <algorithm>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

const char kPrimaryAccountKey[] = "primary_account@example.com";

enum TrackingEventType { SIGN_IN, SIGN_OUT };

std::string AccountKeyToObfuscatedId(const std::string& email) {
  return "obfid-" + email;
}

class TrackingEvent {
 public:
  TrackingEvent(TrackingEventType type,
                const std::string& account_key,
                const std::string& gaia_id)
      : type_(type), account_key_(account_key), gaia_id_(gaia_id) {}

  TrackingEvent(TrackingEventType type, const std::string& account_key)
      : type_(type),
        account_key_(account_key),
        gaia_id_(AccountKeyToObfuscatedId(account_key)) {}

  bool operator==(const TrackingEvent& event) const {
    return type_ == event.type_ && account_key_ == event.account_key_ &&
           gaia_id_ == event.gaia_id_;
  }

  std::string ToString() const {
    const char* typestr = "INVALID";
    switch (type_) {
      case SIGN_IN:
        typestr = " IN";
        break;
      case SIGN_OUT:
        typestr = "OUT";
        break;
    }
    return base::StringPrintf("{ type: %s, email: %s, gaia: %s }", typestr,
                              account_key_.c_str(), gaia_id_.c_str());
  }

 private:
  friend bool CompareByUser(TrackingEvent a, TrackingEvent b);

  TrackingEventType type_;
  std::string account_key_;
  std::string gaia_id_;
};

bool CompareByUser(TrackingEvent a, TrackingEvent b) {
  return a.account_key_ < b.account_key_;
}

std::string Str(const std::vector<TrackingEvent>& events) {
  std::string str = "[";
  bool needs_comma = false;
  for (std::vector<TrackingEvent>::const_iterator it = events.begin();
       it != events.end(); ++it) {
    if (needs_comma)
      str += ",\n ";
    needs_comma = true;
    str += it->ToString();
  }
  str += "]";
  return str;
}

}  // namespace

namespace gcm {

class AccountTrackerObserver : public AccountTracker::Observer {
 public:
  AccountTrackerObserver() {}
  virtual ~AccountTrackerObserver() {}

  testing::AssertionResult CheckEvents();
  testing::AssertionResult CheckEvents(const TrackingEvent& e1);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2,
                                       const TrackingEvent& e3);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2,
                                       const TrackingEvent& e3,
                                       const TrackingEvent& e4);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2,
                                       const TrackingEvent& e3,
                                       const TrackingEvent& e4,
                                       const TrackingEvent& e5);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2,
                                       const TrackingEvent& e3,
                                       const TrackingEvent& e4,
                                       const TrackingEvent& e5,
                                       const TrackingEvent& e6);
  void Clear();
  void SortEventsByUser();

  // AccountTracker::Observer implementation
  void OnAccountSignInChanged(const AccountIds& ids,
                              bool is_signed_in) override;

 private:
  testing::AssertionResult CheckEvents(
      const std::vector<TrackingEvent>& events);

  std::vector<TrackingEvent> events_;
};

void AccountTrackerObserver::OnAccountSignInChanged(const AccountIds& ids,
                                                    bool is_signed_in) {
  events_.push_back(
      TrackingEvent(is_signed_in ? SIGN_IN : SIGN_OUT, ids.email, ids.gaia));
}

void AccountTrackerObserver::Clear() {
  events_.clear();
}

void AccountTrackerObserver::SortEventsByUser() {
  std::stable_sort(events_.begin(), events_.end(), CompareByUser);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents() {
  std::vector<TrackingEvent> events;
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1,
    const TrackingEvent& e2) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  events.push_back(e2);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1,
    const TrackingEvent& e2,
    const TrackingEvent& e3) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  events.push_back(e2);
  events.push_back(e3);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1,
    const TrackingEvent& e2,
    const TrackingEvent& e3,
    const TrackingEvent& e4) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  events.push_back(e2);
  events.push_back(e3);
  events.push_back(e4);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1,
    const TrackingEvent& e2,
    const TrackingEvent& e3,
    const TrackingEvent& e4,
    const TrackingEvent& e5) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  events.push_back(e2);
  events.push_back(e3);
  events.push_back(e4);
  events.push_back(e5);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const TrackingEvent& e1,
    const TrackingEvent& e2,
    const TrackingEvent& e3,
    const TrackingEvent& e4,
    const TrackingEvent& e5,
    const TrackingEvent& e6) {
  std::vector<TrackingEvent> events;
  events.push_back(e1);
  events.push_back(e2);
  events.push_back(e3);
  events.push_back(e4);
  events.push_back(e5);
  events.push_back(e6);
  return CheckEvents(events);
}

testing::AssertionResult AccountTrackerObserver::CheckEvents(
    const std::vector<TrackingEvent>& events) {
  std::string maybe_newline = (events.size() + events_.size()) > 2 ? "\n" : "";
  testing::AssertionResult result(
      (events_ == events)
          ? testing::AssertionSuccess()
          : (testing::AssertionFailure()
             << "Expected " << maybe_newline << Str(events) << ", "
             << maybe_newline << "Got " << maybe_newline << Str(events_)));
  events_.clear();
  return result;
}

class AccountTrackerTest : public testing::Test {
 public:
  AccountTrackerTest() {}

  ~AccountTrackerTest() override {}

  void SetUp() override {
    fake_oauth2_token_service_.reset(new FakeProfileOAuth2TokenService());

    test_signin_client_.reset(new TestSigninClient(&pref_service_));
#if defined(OS_CHROMEOS)
    fake_signin_manager_.reset(new SigninManagerForTest(
        test_signin_client_.get(), &account_tracker_service_));
#else
    fake_signin_manager_.reset(new SigninManagerForTest(
        test_signin_client_.get(), fake_oauth2_token_service_.get(),
        &account_tracker_service_, nullptr));
#endif

    AccountTrackerService::RegisterPrefs(pref_service_.registry());
    SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
    SigninManagerBase::RegisterPrefs(pref_service_.registry());
    account_tracker_service_.Initialize(test_signin_client_.get());

    account_tracker_.reset(new AccountTracker(
        fake_signin_manager_.get(), fake_oauth2_token_service_.get(),
        new net::TestURLRequestContextGetter(message_loop_.task_runner())));
    account_tracker_->AddObserver(&observer_);
  }

  void TearDown() override {
    account_tracker_->RemoveObserver(&observer_);
    account_tracker_->Shutdown();
  }

  AccountTrackerObserver* observer() { return &observer_; }

  AccountTracker* account_tracker() { return account_tracker_.get(); }

  // Helpers to pass fake events to the tracker.

  // Sets the primary account info but carries no guarantee of firing the
  // callback that signin occurred (see NotifyLogin() below if exercising a
  // true signin flow in a non-ChromeOS context).
  void SetActiveAccount(const std::string& account_key) {
#if defined(OS_CHROMEOS)
    fake_signin_manager_->SignIn(account_key);
#else
    fake_signin_manager_->SignIn(account_key, account_key, "" /* password */);
#endif
  }

// NOTE: On ChromeOS, the login callback is never fired in production (since the
// underlying GoogleSigninSucceeded callback is never sent). Tests that
// exercise functionality dependent on that callback firing are not relevant
// on ChromeOS and should simply not run on that platform.
#if !defined(OS_CHROMEOS)
  void NotifyLogin(const std::string& account_key) {
    fake_signin_manager_->SignIn(account_key, account_key, "" /* password */);
  }

  void NotifyLogoutOfPrimaryAccountOnly() {
    fake_signin_manager_->SignOutAndKeepAllAccounts(
        signin_metrics::SIGNOUT_TEST,
        signin_metrics::SignoutDelete::IGNORE_METRIC);
  }

  void NotifyLogoutOfAllAccounts() {
    fake_signin_manager_->SignOutAndRemoveAllAccounts(
        signin_metrics::SIGNOUT_TEST,
        signin_metrics::SignoutDelete::IGNORE_METRIC);
  }
#endif

  void NotifyTokenAvailable(const std::string& username) {
    fake_oauth2_token_service_->UpdateCredentials(username,
                                                  "fake_refresh_token");
  }

  void NotifyTokenRevoked(const std::string& username) {
    fake_oauth2_token_service_->RevokeCredentials(username);
  }

  // Helpers to fake access token and user info fetching
  void IssueAccessToken(const std::string& username) {
    fake_oauth2_token_service_->IssueAllTokensForAccount(
        username, "access_token-" + username, base::Time::Max());
  }

  std::string GetValidTokenInfoResponse(const std::string& account_key) {
    return std::string("{ \"id\": \"") + AccountKeyToObfuscatedId(account_key) +
           "\" }";
  }

  void ReturnOAuthUrlFetchResults(int fetcher_id,
                                  net::HttpStatusCode response_code,
                                  const std::string& response_string);

  void ReturnOAuthUrlFetchSuccess(const std::string& account_key);
  void ReturnOAuthUrlFetchFailure(const std::string& account_key);

  void SetupPrimaryLogin() {
    // Initial setup for tests that start with a signed in profile.
    SetActiveAccount(kPrimaryAccountKey);
    NotifyTokenAvailable(kPrimaryAccountKey);
    ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
    observer()->Clear();
  }

 private:
  base::MessageLoopForIO message_loop_;  // net:: stuff needs IO message loop.
  net::TestURLFetcherFactory test_fetcher_factory_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  AccountTrackerService account_tracker_service_;
  std::unique_ptr<TestSigninClient> test_signin_client_;
  std::unique_ptr<SigninManagerForTest> fake_signin_manager_;
  std::unique_ptr<FakeProfileOAuth2TokenService> fake_oauth2_token_service_;

  std::unique_ptr<AccountTracker> account_tracker_;
  AccountTrackerObserver observer_;
};

void AccountTrackerTest::ReturnOAuthUrlFetchResults(
    int fetcher_id,
    net::HttpStatusCode response_code,
    const std::string& response_string) {
  net::TestURLFetcher* fetcher =
      test_fetcher_factory_.GetFetcherByID(fetcher_id);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(response_code);
  fetcher->SetResponseString(response_string);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

void AccountTrackerTest::ReturnOAuthUrlFetchSuccess(
    const std::string& account_key) {
  IssueAccessToken(account_key);
  ReturnOAuthUrlFetchResults(gaia::GaiaOAuthClient::kUrlFetcherId, net::HTTP_OK,
                             GetValidTokenInfoResponse(account_key));
}

void AccountTrackerTest::ReturnOAuthUrlFetchFailure(
    const std::string& account_key) {
  IssueAccessToken(account_key);
  ReturnOAuthUrlFetchResults(gaia::GaiaOAuthClient::kUrlFetcherId,
                             net::HTTP_BAD_REQUEST, "");
}

// Primary tests just involve the Active account

TEST_F(AccountTrackerTest, PrimaryNoEventsBeforeLogin) {
  NotifyTokenAvailable(kPrimaryAccountKey);
  NotifyTokenRevoked(kPrimaryAccountKey);

// Logout is not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
  NotifyLogoutOfAllAccounts();
#endif

  EXPECT_TRUE(observer()->CheckEvents());
}

TEST_F(AccountTrackerTest, PrimaryLoginThenTokenAvailable) {
  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  EXPECT_TRUE(observer()->CheckEvents());

  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}

// These tests exercise true login/logout, which are not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(AccountTrackerTest, PrimaryTokenAvailableThenLogin) {
  NotifyTokenAvailable(kPrimaryAccountKey);
  EXPECT_TRUE(observer()->CheckEvents());

  NotifyLogin(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}

TEST_F(AccountTrackerTest, PrimaryTokenAvailableAndRevokedThenLogin) {
  NotifyTokenAvailable(kPrimaryAccountKey);
  EXPECT_TRUE(observer()->CheckEvents());

  NotifyLogin(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}
#endif

TEST_F(AccountTrackerTest, PrimaryRevoke) {
  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  observer()->Clear();

  NotifyTokenRevoked(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, kPrimaryAccountKey)));
}

TEST_F(AccountTrackerTest, PrimaryRevokeThenLogin) {
  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  NotifyTokenRevoked(kPrimaryAccountKey);
  observer()->Clear();

  SetActiveAccount(kPrimaryAccountKey);
  EXPECT_TRUE(observer()->CheckEvents());
}

TEST_F(AccountTrackerTest, PrimaryRevokeThenTokenAvailable) {
  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  NotifyTokenRevoked(kPrimaryAccountKey);
  observer()->Clear();

  NotifyTokenAvailable(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}

// These tests exercise true login/logout, which are not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(AccountTrackerTest, PrimaryLogoutThenRevoke) {
  NotifyLogin(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  observer()->Clear();

  NotifyLogoutOfAllAccounts();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, kPrimaryAccountKey)));

  NotifyTokenRevoked(kPrimaryAccountKey);
  EXPECT_TRUE(observer()->CheckEvents());
}

TEST_F(AccountTrackerTest, PrimaryLogoutFetchCancelAvailable) {
  NotifyLogin(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  // TokenAvailable kicks off a fetch. Logout without satisfying it.
  NotifyLogoutOfAllAccounts();
  EXPECT_TRUE(observer()->CheckEvents());

  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}
#endif

// Non-primary accounts

TEST_F(AccountTrackerTest, Available) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  EXPECT_TRUE(observer()->CheckEvents());

  ReturnOAuthUrlFetchSuccess("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com")));
}

TEST_F(AccountTrackerTest, AvailableRevokeAvailable) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  NotifyTokenRevoked("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com"),
                              TrackingEvent(SIGN_OUT, "user@example.com")));

  NotifyTokenAvailable("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com")));
}

TEST_F(AccountTrackerTest, AvailableRevokeAvailableWithPendingFetch) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  NotifyTokenRevoked("user@example.com");
  EXPECT_TRUE(observer()->CheckEvents());

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com")));
}

TEST_F(AccountTrackerTest, AvailableRevokeRevoke) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  NotifyTokenRevoked("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com"),
                              TrackingEvent(SIGN_OUT, "user@example.com")));

  NotifyTokenRevoked("user@example.com");
  EXPECT_TRUE(observer()->CheckEvents());
}

TEST_F(AccountTrackerTest, AvailableAvailable) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com")));

  NotifyTokenAvailable("user@example.com");
  EXPECT_TRUE(observer()->CheckEvents());
}

TEST_F(AccountTrackerTest, TwoAccounts) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("alpha@example.com");
  ReturnOAuthUrlFetchSuccess("alpha@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "alpha@example.com")));

  NotifyTokenAvailable("beta@example.com");
  ReturnOAuthUrlFetchSuccess("beta@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "beta@example.com")));

  NotifyTokenRevoked("alpha@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, "alpha@example.com")));

  NotifyTokenRevoked("beta@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, "beta@example.com")));
}

TEST_F(AccountTrackerTest, AvailableTokenFetchFailAvailable) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchFailure("user@example.com");
  EXPECT_TRUE(observer()->CheckEvents());

  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "user@example.com")));
}

// These tests exercise true login/logout, which are not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(AccountTrackerTest, MultiSignOutSignIn) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("alpha@example.com");
  ReturnOAuthUrlFetchSuccess("alpha@example.com");
  NotifyTokenAvailable("beta@example.com");
  ReturnOAuthUrlFetchSuccess("beta@example.com");

  observer()->SortEventsByUser();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "alpha@example.com"),
                              TrackingEvent(SIGN_IN, "beta@example.com")));

  // Log out of the primary account only (allows for testing that the account
  // tracker preserves knowledge of "beta@example.com").
  NotifyLogoutOfPrimaryAccountOnly();
  observer()->SortEventsByUser();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, "alpha@example.com"),
                              TrackingEvent(SIGN_OUT, "beta@example.com"),
                              TrackingEvent(SIGN_OUT, kPrimaryAccountKey)));

  // No events fire at all while profile is signed out.
  NotifyTokenRevoked("alpha@example.com");
  NotifyTokenAvailable("gamma@example.com");
  EXPECT_TRUE(observer()->CheckEvents());

  // Signing the profile in again will resume tracking all accounts.
  NotifyLogin(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess("beta@example.com");
  ReturnOAuthUrlFetchSuccess("gamma@example.com");
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  observer()->SortEventsByUser();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, "beta@example.com"),
                              TrackingEvent(SIGN_IN, "gamma@example.com"),
                              TrackingEvent(SIGN_IN, kPrimaryAccountKey)));

  // Revoking the primary token does not affect other accounts.
  NotifyTokenRevoked(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, kPrimaryAccountKey)));

  NotifyTokenAvailable(kPrimaryAccountKey);
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_IN, kPrimaryAccountKey)));
}
#endif

// Primary/non-primary interactions

TEST_F(AccountTrackerTest, MultiNoEventsBeforeLogin) {
  NotifyTokenAvailable(kPrimaryAccountKey);
  NotifyTokenAvailable("user@example.com");
  NotifyTokenRevoked("user@example.com");
  NotifyTokenRevoked(kPrimaryAccountKey);

// Logout is not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
  NotifyLogoutOfAllAccounts();
#endif

  EXPECT_TRUE(observer()->CheckEvents());
}

// This test exercises true login/logout, which are not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(AccountTrackerTest, MultiLogoutRemovesAllAccounts) {
  NotifyLogin(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  observer()->Clear();

  NotifyLogoutOfAllAccounts();
  observer()->SortEventsByUser();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, kPrimaryAccountKey),
                              TrackingEvent(SIGN_OUT, "user@example.com")));
}
#endif

TEST_F(AccountTrackerTest, MultiRevokePrimaryDoesNotRemoveAllAccounts) {
  SetActiveAccount(kPrimaryAccountKey);
  NotifyTokenAvailable(kPrimaryAccountKey);
  ReturnOAuthUrlFetchSuccess(kPrimaryAccountKey);
  NotifyTokenAvailable("user@example.com");
  ReturnOAuthUrlFetchSuccess("user@example.com");
  observer()->Clear();

  NotifyTokenRevoked(kPrimaryAccountKey);
  observer()->SortEventsByUser();
  EXPECT_TRUE(
      observer()->CheckEvents(TrackingEvent(SIGN_OUT, kPrimaryAccountKey)));
}

TEST_F(AccountTrackerTest, GetAccountsPrimary) {
  SetupPrimaryLogin();

  std::vector<AccountIds> ids = account_tracker()->GetAccounts();
  EXPECT_EQ(1ul, ids.size());
  EXPECT_EQ(kPrimaryAccountKey, ids[0].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId(kPrimaryAccountKey), ids[0].gaia);
}

TEST_F(AccountTrackerTest, GetAccountsSignedOut) {
  std::vector<AccountIds> ids = account_tracker()->GetAccounts();
  EXPECT_EQ(0ul, ids.size());
}

TEST_F(AccountTrackerTest, GetAccountsOnlyReturnAccountsWithTokens) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("alpha@example.com");
  NotifyTokenAvailable("beta@example.com");
  ReturnOAuthUrlFetchSuccess("beta@example.com");

  std::vector<AccountIds> ids = account_tracker()->GetAccounts();
  EXPECT_EQ(2ul, ids.size());
  EXPECT_EQ(kPrimaryAccountKey, ids[0].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId(kPrimaryAccountKey), ids[0].gaia);
  EXPECT_EQ("beta@example.com", ids[1].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId("beta@example.com"), ids[1].gaia);
}

TEST_F(AccountTrackerTest, GetAccountsSortOrder) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("zeta@example.com");
  ReturnOAuthUrlFetchSuccess("zeta@example.com");
  NotifyTokenAvailable("alpha@example.com");
  ReturnOAuthUrlFetchSuccess("alpha@example.com");

  // The primary account will be first in the vector. Remaining accounts
  // will be sorted by gaia ID.
  std::vector<AccountIds> ids = account_tracker()->GetAccounts();
  EXPECT_EQ(3ul, ids.size());
  EXPECT_EQ(kPrimaryAccountKey, ids[0].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId(kPrimaryAccountKey), ids[0].gaia);
  EXPECT_EQ("alpha@example.com", ids[1].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId("alpha@example.com"), ids[1].gaia);
  EXPECT_EQ("zeta@example.com", ids[2].account_key);
  EXPECT_EQ(AccountKeyToObfuscatedId("zeta@example.com"), ids[2].gaia);
}

TEST_F(AccountTrackerTest, GetAccountsReturnNothingWhenPrimarySignedOut) {
  SetupPrimaryLogin();

  NotifyTokenAvailable("zeta@example.com");
  ReturnOAuthUrlFetchSuccess("zeta@example.com");
  NotifyTokenAvailable("alpha@example.com");
  ReturnOAuthUrlFetchSuccess("alpha@example.com");

  NotifyTokenRevoked(kPrimaryAccountKey);

  std::vector<AccountIds> ids = account_tracker()->GetAccounts();
  EXPECT_EQ(0ul, ids.size());
}

}  // namespace gcm
