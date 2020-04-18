// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_account_tracker.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "components/gcm_driver/fake_gcm_driver.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gcm {

namespace {

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

const char kAccountId1[] = "account_1";
const char kAccountId2[] = "account_2";

std::string AccountKeyToObfuscatedId(const std::string& email) {
  return "obfid-" + email;
}

std::string GetValidTokenInfoResponse(const std::string& account_key) {
  return std::string("{ \"id\": \"") + AccountKeyToObfuscatedId(account_key) +
         "\" }";
}

std::string MakeAccessToken(const std::string& account_key) {
  return "access_token-" + account_key;
}

GCMClient::AccountTokenInfo MakeAccountToken(const std::string& account_key) {
  GCMClient::AccountTokenInfo token_info;
  token_info.account_id = account_key;
  token_info.email = account_key;
  token_info.access_token = MakeAccessToken(account_key);
  return token_info;
}

void VerifyAccountTokens(
    const std::vector<GCMClient::AccountTokenInfo>& expected_tokens,
    const std::vector<GCMClient::AccountTokenInfo>& actual_tokens) {
  EXPECT_EQ(expected_tokens.size(), actual_tokens.size());
  for (std::vector<GCMClient::AccountTokenInfo>::const_iterator
           expected_iter = expected_tokens.begin(),
           actual_iter = actual_tokens.begin();
       expected_iter != expected_tokens.end() &&
           actual_iter != actual_tokens.end();
       ++expected_iter, ++actual_iter) {
    EXPECT_EQ(expected_iter->account_id, actual_iter->account_id);
    EXPECT_EQ(expected_iter->email, actual_iter->email);
    EXPECT_EQ(expected_iter->access_token, actual_iter->access_token);
  }
}

// This version of FakeGCMDriver is customized around handling accounts and
// connection events for testing GCMAccountTracker.
class CustomFakeGCMDriver : public FakeGCMDriver {
 public:
  CustomFakeGCMDriver();
  ~CustomFakeGCMDriver() override;

  // GCMDriver overrides:
  void SetAccountTokens(
      const std::vector<GCMClient::AccountTokenInfo>& account_tokens) override;
  void AddConnectionObserver(GCMConnectionObserver* observer) override;
  void RemoveConnectionObserver(GCMConnectionObserver* observer) override;
  bool IsConnected() const override { return connected_; }
  base::Time GetLastTokenFetchTime() override;
  void SetLastTokenFetchTime(const base::Time& time) override;

  // Test results and helpers.
  void SetConnected(bool connected);
  void ResetResults();
  bool update_accounts_called() const { return update_accounts_called_; }
  const std::vector<GCMClient::AccountTokenInfo>& accounts() const {
    return accounts_;
  }
  const GCMConnectionObserver* last_connection_observer() const {
    return last_connection_observer_;
  }
  const GCMConnectionObserver* last_removed_connection_observer() const {
    return removed_connection_observer_;
  }

 private:
  bool connected_;
  std::vector<GCMClient::AccountTokenInfo> accounts_;
  bool update_accounts_called_;
  GCMConnectionObserver* last_connection_observer_;
  GCMConnectionObserver* removed_connection_observer_;
  net::IPEndPoint ip_endpoint_;
  base::Time last_token_fetch_time_;

  DISALLOW_COPY_AND_ASSIGN(CustomFakeGCMDriver);
};

CustomFakeGCMDriver::CustomFakeGCMDriver()
    : connected_(true),
      update_accounts_called_(false),
      last_connection_observer_(nullptr),
      removed_connection_observer_(nullptr) {}

CustomFakeGCMDriver::~CustomFakeGCMDriver() {
}

void CustomFakeGCMDriver::SetAccountTokens(
    const std::vector<GCMClient::AccountTokenInfo>& accounts) {
  update_accounts_called_ = true;
  accounts_ = accounts;
}

void CustomFakeGCMDriver::AddConnectionObserver(
    GCMConnectionObserver* observer) {
  last_connection_observer_ = observer;
}

void CustomFakeGCMDriver::RemoveConnectionObserver(
    GCMConnectionObserver* observer) {
  removed_connection_observer_ = observer;
}

void CustomFakeGCMDriver::SetConnected(bool connected) {
  connected_ = connected;
  if (connected && last_connection_observer_)
    last_connection_observer_->OnConnected(ip_endpoint_);
}

void CustomFakeGCMDriver::ResetResults() {
  accounts_.clear();
  update_accounts_called_ = false;
  last_connection_observer_ = nullptr;
  removed_connection_observer_ = nullptr;
}


base::Time CustomFakeGCMDriver::GetLastTokenFetchTime() {
  return last_token_fetch_time_;
}

void CustomFakeGCMDriver::SetLastTokenFetchTime(const base::Time& time) {
  last_token_fetch_time_ = time;
}

}  // namespace

class GCMAccountTrackerTest : public testing::Test {
 public:
  GCMAccountTrackerTest();
  ~GCMAccountTrackerTest() override;

  // Helpers to pass fake info to the tracker. Tests should have either a pair
  // of Start(Primary)/FinishAccountAddition or Add(Primary)Account per
  // account. Don't mix.
  // Call to RemoveAccount is not mandatory.
  void StartAccountAddition(const std::string& account_key);
  void StartPrimaryAccountAddition(const std::string& account_key);
  void FinishAccountAddition(const std::string& account_key);
  void AddAccount(const std::string& account_key);
  void AddPrimaryAccount(const std::string& account_key);
  void RemoveAccount(const std::string& account_key);

  // Helpers for dealing with OAuth2 access token requests.
  void IssueAccessToken(const std::string& account_key);
  void IssueExpiredAccessToken(const std::string& account_key);
  void IssueError(const std::string& account_key);

  // Accessors to account tracker and gcm driver.
  GCMAccountTracker* tracker() { return tracker_.get(); }
  CustomFakeGCMDriver* driver() { return &driver_; }

  // Accessors to private methods of account tracker.
  bool IsFetchingRequired() const;
  bool IsTokenReportingRequired() const;
  base::TimeDelta GetTimeToNextTokenReporting() const;

 private:
  CustomFakeGCMDriver driver_;

  base::MessageLoop message_loop_;
  net::TestURLFetcherFactory test_fetcher_factory_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  AccountTrackerService account_tracker_service_;
  std::unique_ptr<TestSigninClient> test_signin_client_;
  std::unique_ptr<SigninManagerForTest> fake_signin_manager_;
  std::unique_ptr<FakeProfileOAuth2TokenService> fake_token_service_;
  std::unique_ptr<GCMAccountTracker> tracker_;
};

GCMAccountTrackerTest::GCMAccountTrackerTest() {
  fake_token_service_.reset(new FakeProfileOAuth2TokenService());

  test_signin_client_.reset(new TestSigninClient(&pref_service_));
#if defined(OS_CHROMEOS)
  fake_signin_manager_.reset(new SigninManagerForTest(
      test_signin_client_.get(), &account_tracker_service_));
#else
  fake_signin_manager_.reset(new SigninManagerForTest(
      test_signin_client_.get(), fake_token_service_.get(),
      &account_tracker_service_, nullptr));
#endif

  AccountTrackerService::RegisterPrefs(pref_service_.registry());
  SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
  SigninManagerBase::RegisterPrefs(pref_service_.registry());
  account_tracker_service_.Initialize(test_signin_client_.get());

  std::unique_ptr<AccountTracker> gaia_account_tracker(new AccountTracker(
      fake_signin_manager_.get(), fake_token_service_.get(),
      new net::TestURLRequestContextGetter(message_loop_.task_runner())));

  tracker_.reset(new GCMAccountTracker(std::move(gaia_account_tracker),
                                       fake_token_service_.get(), &driver_));
}

GCMAccountTrackerTest::~GCMAccountTrackerTest() {
  if (tracker_)
    tracker_->Shutdown();
}

void GCMAccountTrackerTest::StartAccountAddition(
    const std::string& account_key) {
  fake_token_service_->UpdateCredentials(account_key, "fake_refresh_token");
}

void GCMAccountTrackerTest::StartPrimaryAccountAddition(
    const std::string& account_key) {
// NOTE: Setting of the primary account info must be done first on ChromeOS
// to ensure that AccountTracker and GCMAccountTracker respond as expected
// when the token is added to the token service.
// TODO(blundell): On non-ChromeOS, it would be good to add tests wherein
// setting of the primary account is done afterward to check that the flow
// that ensues from the GoogleSigninSucceeded callback firing works as
// expected.
#if defined(OS_CHROMEOS)
  fake_signin_manager_->SignIn(account_key);
#else
  fake_signin_manager_->SignIn(account_key, account_key, "" /* password */);
#endif
  StartAccountAddition(account_key);
}

void GCMAccountTrackerTest::FinishAccountAddition(
    const std::string& account_key) {
  IssueAccessToken(account_key);

  net::TestURLFetcher* fetcher = test_fetcher_factory_.GetFetcherByID(
      gaia::GaiaOAuthClient::kUrlFetcherId);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(GetValidTokenInfoResponse(account_key));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

void GCMAccountTrackerTest::AddPrimaryAccount(const std::string& account_key) {
  StartPrimaryAccountAddition(account_key);
  FinishAccountAddition(account_key);
}

void GCMAccountTrackerTest::AddAccount(const std::string& account_key) {
  StartAccountAddition(account_key);
  FinishAccountAddition(account_key);
}

void GCMAccountTrackerTest::RemoveAccount(const std::string& account_key) {
  fake_token_service_->RevokeCredentials(account_key);
}

void GCMAccountTrackerTest::IssueAccessToken(const std::string& account_key) {
  fake_token_service_->IssueAllTokensForAccount(
      account_key, MakeAccessToken(account_key), base::Time::Max());
}

void GCMAccountTrackerTest::IssueExpiredAccessToken(
    const std::string& account_key) {
  fake_token_service_->IssueAllTokensForAccount(
      account_key, MakeAccessToken(account_key), base::Time::Now());
}

void GCMAccountTrackerTest::IssueError(const std::string& account_key) {
  fake_token_service_->IssueErrorForAllPendingRequestsForAccount(
      account_key,
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
}

bool GCMAccountTrackerTest::IsFetchingRequired() const {
  return tracker_->IsTokenFetchingRequired();
}

bool GCMAccountTrackerTest::IsTokenReportingRequired() const {
  return tracker_->IsTokenReportingRequired();
}

base::TimeDelta GCMAccountTrackerTest::GetTimeToNextTokenReporting() const {
  return tracker_->GetTimeToNextTokenReporting();
}

TEST_F(GCMAccountTrackerTest, NoAccounts) {
  EXPECT_FALSE(driver()->update_accounts_called());
  tracker()->Start();
  // Callback should not be called if there where no accounts provided.
  EXPECT_FALSE(driver()->update_accounts_called());
  EXPECT_TRUE(driver()->accounts().empty());
}

// Verifies that callback is called after a token is issued for a single account
// with a specific scope. In this scenario, the underlying account tracker is
// still working when the CompleteCollectingTokens is called for the first time.
TEST_F(GCMAccountTrackerTest, SingleAccount) {
  StartPrimaryAccountAddition(kAccountId1);

  tracker()->Start();
  // We don't have any accounts to report, but given the inner account tracker
  // is still working we don't make a call with empty accounts list.
  EXPECT_FALSE(driver()->update_accounts_called());

  // This concludes the work of inner account tracker.
  FinishAccountAddition(kAccountId1);
  IssueAccessToken(kAccountId1);

  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, MultipleAccounts) {
  StartPrimaryAccountAddition(kAccountId1);

  StartAccountAddition(kAccountId2);

  tracker()->Start();
  EXPECT_FALSE(driver()->update_accounts_called());

  FinishAccountAddition(kAccountId1);
  IssueAccessToken(kAccountId1);
  EXPECT_FALSE(driver()->update_accounts_called());

  FinishAccountAddition(kAccountId2);
  IssueAccessToken(kAccountId2);
  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  expected_accounts.push_back(MakeAccountToken(kAccountId2));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, AccountAdded) {
  tracker()->Start();
  driver()->ResetResults();

  AddPrimaryAccount(kAccountId1);
  EXPECT_FALSE(driver()->update_accounts_called());

  IssueAccessToken(kAccountId1);
  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, AccountRemoved) {
  AddPrimaryAccount(kAccountId1);
  AddAccount(kAccountId2);

  tracker()->Start();
  IssueAccessToken(kAccountId1);
  IssueAccessToken(kAccountId2);
  EXPECT_TRUE(driver()->update_accounts_called());

  driver()->ResetResults();
  EXPECT_FALSE(driver()->update_accounts_called());

  RemoveAccount(kAccountId2);
  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, GetTokenFailed) {
  AddPrimaryAccount(kAccountId1);
  AddAccount(kAccountId2);

  tracker()->Start();
  IssueAccessToken(kAccountId1);
  EXPECT_FALSE(driver()->update_accounts_called());

  IssueError(kAccountId2);

  // Failed token is not retried any more. Account marked as removed.
  EXPECT_EQ(0UL, tracker()->get_pending_token_request_count());
  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, GetTokenFailedAccountRemoved) {
  AddPrimaryAccount(kAccountId1);
  AddAccount(kAccountId2);

  tracker()->Start();
  IssueAccessToken(kAccountId1);

  driver()->ResetResults();
  RemoveAccount(kAccountId2);
  IssueError(kAccountId2);

  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

TEST_F(GCMAccountTrackerTest, AccountRemovedWhileRequestsPending) {
  AddPrimaryAccount(kAccountId1);
  AddAccount(kAccountId2);

  tracker()->Start();
  IssueAccessToken(kAccountId1);
  EXPECT_FALSE(driver()->update_accounts_called());

  RemoveAccount(kAccountId2);
  IssueAccessToken(kAccountId2);
  EXPECT_TRUE(driver()->update_accounts_called());

  std::vector<GCMClient::AccountTokenInfo> expected_accounts;
  expected_accounts.push_back(MakeAccountToken(kAccountId1));
  VerifyAccountTokens(expected_accounts, driver()->accounts());
}

// Makes sure that tracker observes GCM connection when running.
TEST_F(GCMAccountTrackerTest, TrackerObservesConnection) {
  EXPECT_EQ(nullptr, driver()->last_connection_observer());
  tracker()->Start();
  EXPECT_EQ(tracker(), driver()->last_connection_observer());
  tracker()->Shutdown();
  EXPECT_EQ(tracker(), driver()->last_removed_connection_observer());
}

// Makes sure that token fetching happens only after connection is established.
TEST_F(GCMAccountTrackerTest, PostponeTokenFetchingUntilConnected) {
  driver()->SetConnected(false);
  StartPrimaryAccountAddition(kAccountId1);
  tracker()->Start();
  FinishAccountAddition(kAccountId1);

  EXPECT_EQ(0UL, tracker()->get_pending_token_request_count());
  driver()->SetConnected(true);

  EXPECT_EQ(1UL, tracker()->get_pending_token_request_count());
}

TEST_F(GCMAccountTrackerTest, InvalidateExpiredTokens) {
  StartPrimaryAccountAddition(kAccountId1);
  StartAccountAddition(kAccountId2);
  tracker()->Start();
  FinishAccountAddition(kAccountId1);
  FinishAccountAddition(kAccountId2);

  EXPECT_EQ(2UL, tracker()->get_pending_token_request_count());

  IssueExpiredAccessToken(kAccountId1);
  IssueAccessToken(kAccountId2);
  // Because the first token is expired, we expect the sanitize to kick in and
  // clean it up before the SetAccessToken is called. This also means a new
  // token request will be issued
  EXPECT_FALSE(driver()->update_accounts_called());
  EXPECT_EQ(1UL, tracker()->get_pending_token_request_count());
}

// Testing for whether there are still more tokens to be fetched. Typically the
// need for token fetching triggers immediate request, unless there is no
// connection, that is why connection is set on and off in this test.
TEST_F(GCMAccountTrackerTest, IsTokenFetchingRequired) {
  tracker()->Start();
  driver()->SetConnected(false);
  EXPECT_FALSE(IsFetchingRequired());
  StartPrimaryAccountAddition(kAccountId1);
  FinishAccountAddition(kAccountId1);
  EXPECT_TRUE(IsFetchingRequired());

  driver()->SetConnected(true);
  EXPECT_FALSE(IsFetchingRequired());  // Indicates that fetching has started.
  IssueAccessToken(kAccountId1);
  EXPECT_FALSE(IsFetchingRequired());

  driver()->SetConnected(false);
  StartAccountAddition(kAccountId2);
  FinishAccountAddition(kAccountId2);
  EXPECT_TRUE(IsFetchingRequired());

  IssueExpiredAccessToken(kAccountId2);
  // Make sure that if the token was expired it is still needed.
  EXPECT_TRUE(IsFetchingRequired());
}

// Tests what is the expected time to the next token fetching.
TEST_F(GCMAccountTrackerTest, GetTimeToNextTokenReporting) {
  tracker()->Start();
  // At this point the last token fetch time is never.
  EXPECT_EQ(base::TimeDelta(), GetTimeToNextTokenReporting());

  // Regular case. The tokens have been just reported.
  driver()->SetLastTokenFetchTime(base::Time::Now());
  EXPECT_TRUE(GetTimeToNextTokenReporting() <=
                  base::TimeDelta::FromSeconds(12 * 60 * 60));

  // A case when gcm driver is not yet initialized.
  driver()->SetLastTokenFetchTime(base::Time::Max());
  EXPECT_EQ(base::TimeDelta::FromSeconds(12 * 60 * 60),
            GetTimeToNextTokenReporting());

  // A case when token reporting calculation is expected to result in more than
  // 12 hours, in which case we expect exactly 12 hours.
  driver()->SetLastTokenFetchTime(base::Time::Now() +
      base::TimeDelta::FromDays(2));
  EXPECT_EQ(base::TimeDelta::FromSeconds(12 * 60 * 60),
            GetTimeToNextTokenReporting());
}

// Tests conditions when token reporting is required.
TEST_F(GCMAccountTrackerTest, IsTokenReportingRequired) {
  tracker()->Start();
  // Required because it is overdue.
  EXPECT_TRUE(IsTokenReportingRequired());

  // Not required because it just happened.
  driver()->SetLastTokenFetchTime(base::Time::Now());
  EXPECT_FALSE(IsTokenReportingRequired());

  AddPrimaryAccount(kAccountId1);
  IssueAccessToken(kAccountId1);
  driver()->ResetResults();
  // Reporting was triggered, which means testing for required will give false,
  // but we have the update call.
  RemoveAccount(kAccountId1);
  EXPECT_TRUE(driver()->update_accounts_called());
  EXPECT_FALSE(IsTokenReportingRequired());
}

// TODO(fgorski): Add test for adding account after removal >> make sure it does
// not mark removal.

}  // namespace gcm
