// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "components/image_fetcher/core/image_data_fetcher.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/account_fetcher_service.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/child_account_info_fetcher.h"
#include "components/signin/core/browser/fake_account_fetcher_service.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const std::string kTokenInfoResponseFormat =
    "{                        \
      \"id\": \"%s\",         \
      \"email\": \"%s\",      \
      \"hd\": \"\",           \
      \"name\": \"%s\",       \
      \"given_name\": \"%s\", \
      \"locale\": \"%s\",     \
      \"picture\": \"%s\"     \
    }";

const std::string kTokenInfoIncompleteResponseFormat =
    "{                        \
      \"id\": \"%s\",         \
      \"email\": \"%s\",      \
      \"hd\": \"\",           \
    }";

enum TrackingEventType {
  UPDATED,
  IMAGE_UPDATED,
  REMOVED,
};

std::string AccountIdToEmail(const std::string& account_id) {
  return account_id + "@gmail.com";
}

std::string AccountIdToGaiaId(const std::string& account_id) {
  return "gaia-" + account_id;
}

std::string AccountIdToFullName(const std::string& account_id) {
  return "full-name-" + account_id;
}

std::string AccountIdToGivenName(const std::string& account_id) {
  return "given-name-" + account_id;
}

std::string AccountIdToLocale(const std::string& account_id) {
  return "locale-" + account_id;
}

std::string AccountIdToPictureURL(const std::string& account_id) {
  return "https://example.com/-" + account_id +
         "/AAAAAAAAAAI/AAAAAAAAACQ/Efg/photo.jpg";
}

void CheckAccountDetails(const std::string& account_id,
                         const AccountInfo& info) {
  EXPECT_EQ(account_id, info.account_id);
  EXPECT_EQ(AccountIdToGaiaId(account_id), info.gaia);
  EXPECT_EQ(AccountIdToEmail(account_id), info.email);
  EXPECT_EQ(AccountTrackerService::kNoHostedDomainFound,
            info.hosted_domain);
  EXPECT_EQ(AccountIdToFullName(account_id), info.full_name);
  EXPECT_EQ(AccountIdToGivenName(account_id), info.given_name);
  EXPECT_EQ(AccountIdToLocale(account_id), info.locale);
}

void FakeUserInfoFetchSuccess(FakeAccountFetcherService* fetcher,
                              const std::string& account_id) {
  fetcher->FakeUserInfoFetchSuccess(
      account_id, AccountIdToEmail(account_id), AccountIdToGaiaId(account_id),
      AccountTrackerService::kNoHostedDomainFound,
      AccountIdToFullName(account_id), AccountIdToGivenName(account_id),
      AccountIdToLocale(account_id), AccountIdToPictureURL(account_id));
}

class TrackingEvent {
 public:
  TrackingEvent(TrackingEventType type,
                const std::string& account_id,
                const std::string& gaia_id)
      : type_(type),
        account_id_(account_id),
        gaia_id_(gaia_id) {}

  TrackingEvent(TrackingEventType type,
                const std::string& account_id)
      : type_(type),
        account_id_(account_id),
        gaia_id_(AccountIdToGaiaId(account_id)) {}

  bool operator==(const TrackingEvent& event) const {
    return type_ == event.type_ && account_id_ == event.account_id_ &&
        gaia_id_ == event.gaia_id_;
  }

  std::string ToString() const {
    const char * typestr = "INVALID";
    switch (type_) {
      case UPDATED:
        typestr = "UPD";
        break;
      case IMAGE_UPDATED:
        typestr = "IMG_UPD";
        break;
      case REMOVED:
        typestr = "REM";
        break;
    }
    return base::StringPrintf("{ type: %s, account_id: %s, gaia: %s }",
                              typestr,
                              account_id_.c_str(),
                              gaia_id_.c_str());
  }

 private:
  friend bool CompareByUser(TrackingEvent a, TrackingEvent b);

  TrackingEventType type_;
  std::string account_id_;
  std::string gaia_id_;
};

bool CompareByUser(TrackingEvent a, TrackingEvent b) {
  return a.account_id_ < b.account_id_;
}

std::string Str(const std::vector<TrackingEvent>& events) {
  std::string str = "[";
  bool needs_comma = false;
  for (std::vector<TrackingEvent>::const_iterator it =
       events.begin(); it != events.end(); ++it) {
    if (needs_comma)
      str += ",\n ";
    needs_comma = true;
    str += it->ToString();
  }
  str += "]";
  return str;
}

class AccountTrackerObserver : public AccountTrackerService::Observer {
 public:
  AccountTrackerObserver() {}
  ~AccountTrackerObserver() override {}

  void Clear();
  void SortEventsByUser();

  testing::AssertionResult CheckEvents();
  testing::AssertionResult CheckEvents(const TrackingEvent& e1);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2);
  testing::AssertionResult CheckEvents(const TrackingEvent& e1,
                                       const TrackingEvent& e2,
                                       const TrackingEvent& e3);

 private:
  // AccountTrackerService::Observer implementation
  void OnAccountUpdated(const AccountInfo& ids) override;
  void OnAccountImageUpdated(const std::string& account_id,
                             const gfx::Image& image) override;
  void OnAccountRemoved(const AccountInfo& ids) override;

  testing::AssertionResult CheckEvents(
      const std::vector<TrackingEvent>& events);

  std::vector<TrackingEvent> events_;
};

void AccountTrackerObserver::OnAccountUpdated(const AccountInfo& ids) {
  events_.push_back(TrackingEvent(UPDATED, ids.account_id, ids.gaia));
}

void AccountTrackerObserver::OnAccountImageUpdated(
    const std::string& account_id,
    const gfx::Image& image) {
  events_.push_back(TrackingEvent(IMAGE_UPDATED, account_id));
}

void AccountTrackerObserver::OnAccountRemoved(const AccountInfo& ids) {
  events_.push_back(TrackingEvent(REMOVED, ids.account_id, ids.gaia));
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

}  // namespace

class AccountTrackerServiceTest : public testing::Test {
 public:
  AccountTrackerServiceTest()
      : next_image_data_fetcher_id_(
            image_fetcher::ImageDataFetcher::kFirstUrlFetcherId) {}

  ~AccountTrackerServiceTest() override {}

  void SetUp() override {
    ChildAccountInfoFetcher::InitializeForTests();

    fake_oauth2_token_service_.reset(new FakeOAuth2TokenService());

    pref_service_.registry()->RegisterListPref(
        AccountTrackerService::kAccountInfoPref);
    pref_service_.registry()->RegisterIntegerPref(
        prefs::kAccountIdMigrationState,
        AccountTrackerService::MIGRATION_NOT_STARTED);
    pref_service_.registry()->RegisterInt64Pref(
        AccountFetcherService::kLastUpdatePref, 0);
    signin_client_.reset(new TestSigninClient(&pref_service_));
    signin_client_->SetURLRequestContext(new net::TestURLRequestContextGetter(
        scoped_task_environment_.GetMainThreadTaskRunner()));

    account_tracker_.reset(new AccountTrackerService());
    account_tracker_->Initialize(signin_client_.get());

    account_fetcher_.reset(new AccountFetcherService());
    account_fetcher_->Initialize(
        signin_client_.get(), fake_oauth2_token_service_.get(),
        account_tracker_.get(), std::make_unique<TestImageDecoder>());

    account_fetcher_->EnableNetworkFetchesForTest();
  }

  void TearDown() override {
    account_fetcher_->Shutdown();
    account_tracker_->Shutdown();
  }

  void SimulateTokenAvailable(const std::string& account_id) {
    fake_oauth2_token_service_->AddAccount(account_id);
  }

  void SimulateTokenRevoked(const std::string& account_id) {
    fake_oauth2_token_service_->RemoveAccount(account_id);
  }

  // Helpers to fake access token and user info fetching
  void IssueAccessToken(const std::string& account_id) {
    fake_oauth2_token_service_->IssueAllTokensForAccount(
        account_id, "access_token-" + account_id, base::Time::Max());
  }

  std::string GenerateValidTokenInfoResponse(const std::string& account_id) {
    return base::StringPrintf(
        kTokenInfoResponseFormat.c_str(),
        AccountIdToGaiaId(account_id).c_str(),
        AccountIdToEmail(account_id).c_str(),
        AccountIdToFullName(account_id).c_str(),
        AccountIdToGivenName(account_id).c_str(),
        AccountIdToLocale(account_id).c_str(),
        AccountIdToPictureURL(account_id).c_str());
  }

  std::string GenerateIncompleteTokenInfoResponse(
      const std::string& account_id) {
    return base::StringPrintf(
        kTokenInfoIncompleteResponseFormat.c_str(),
        AccountIdToGaiaId(account_id).c_str(),
        AccountIdToEmail(account_id).c_str());
  }
  void ReturnAccountInfoFetchSuccess(const std::string& account_id);
  void ReturnAccountInfoFetchSuccessIncomplete(const std::string& account_id);
  void ReturnAccountInfoFetchFailure(const std::string& account_id);
  void ReturnAccountImageFetchSuccess(const std::string& account_id);
  void ReturnAccountImageFetchFailure(const std::string& account_id);

  net::TestURLFetcherFactory* test_fetcher_factory() {
    return &test_fetcher_factory_;
  }
  AccountFetcherService* account_fetcher() { return account_fetcher_.get(); }
  AccountTrackerService* account_tracker() { return account_tracker_.get(); }
  OAuth2TokenService* token_service() {
    return fake_oauth2_token_service_.get();
  }
  SigninClient* signin_client() { return signin_client_.get(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  void ReturnFetchResults(int fetcher_id,
                          net::HttpStatusCode response_code,
                          const std::string& response_string);

  net::TestURLFetcherFactory test_fetcher_factory_;
  std::unique_ptr<FakeOAuth2TokenService> fake_oauth2_token_service_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AccountFetcherService> account_fetcher_;
  std::unique_ptr<AccountTrackerService> account_tracker_;
  std::unique_ptr<TestSigninClient> signin_client_;

  int next_image_data_fetcher_id_;
};

void AccountTrackerServiceTest::ReturnFetchResults(
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

void AccountTrackerServiceTest::ReturnAccountInfoFetchSuccess(
    const std::string& account_id) {
  IssueAccessToken(account_id);
  ReturnFetchResults(gaia::GaiaOAuthClient::kUrlFetcherId, net::HTTP_OK,
                     GenerateValidTokenInfoResponse(account_id));
}

void AccountTrackerServiceTest::ReturnAccountInfoFetchSuccessIncomplete(
    const std::string& account_id) {
  IssueAccessToken(account_id);
  ReturnFetchResults(gaia::GaiaOAuthClient::kUrlFetcherId, net::HTTP_OK,
                     GenerateIncompleteTokenInfoResponse(account_id));
}

void AccountTrackerServiceTest::ReturnAccountInfoFetchFailure(
    const std::string& account_id) {
  IssueAccessToken(account_id);
  ReturnFetchResults(gaia::GaiaOAuthClient::kUrlFetcherId,
                     net::HTTP_BAD_REQUEST, "");
}

void AccountTrackerServiceTest::ReturnAccountImageFetchSuccess(
    const std::string& account_id) {
  ReturnFetchResults(next_image_data_fetcher_id_++, net::HTTP_OK, "image data");
}

void AccountTrackerServiceTest::ReturnAccountImageFetchFailure(
    const std::string& account_id) {
  ReturnFetchResults(next_image_data_fetcher_id_++, net::HTTP_BAD_REQUEST, "");
}

TEST_F(AccountTrackerServiceTest, Basic) {
}

TEST_F(AccountTrackerServiceTest, TokenAvailable) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ASSERT_FALSE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents());
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailable_Revoked) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  SimulateTokenRevoked("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents());
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailable_UserInfo_ImageSuccess) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));

  ASSERT_TRUE(account_tracker()->GetAccountImage("alpha").IsEmpty());
  ReturnAccountImageFetchSuccess("alpha");
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(IMAGE_UPDATED, "alpha")));
  ASSERT_FALSE(account_tracker()->GetAccountImage("alpha").IsEmpty());

  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailable_UserInfo_ImageFailure) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));

  ASSERT_TRUE(account_tracker()->GetAccountImage("alpha").IsEmpty());
  ReturnAccountImageFetchFailure("alpha");
  ASSERT_TRUE(account_tracker()->GetAccountImage("alpha").IsEmpty());

  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailable_UserInfo_Revoked) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));
  SimulateTokenRevoked("alpha");
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, "alpha")));
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailable_UserInfoFailed) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchFailure("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents());
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAvailableTwice_UserInfoOnce) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));

  SimulateTokenAvailable("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents());
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TokenAlreadyExists) {
  SimulateTokenAvailable("alpha");
  AccountTrackerService tracker;
  AccountTrackerObserver observer;
  AccountFetcherService fetcher;

  tracker.AddObserver(&observer);
  tracker.Initialize(signin_client());

  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  ASSERT_FALSE(fetcher.IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents());
  tracker.RemoveObserver(&observer);
  tracker.Shutdown();
  fetcher.Shutdown();
}

TEST_F(AccountTrackerServiceTest, TwoTokenAvailable_TwoUserInfo) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  SimulateTokenAvailable("beta");
  ReturnAccountInfoFetchSuccess("alpha");
  ReturnAccountInfoFetchSuccess("beta");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha"),
                                   TrackingEvent(UPDATED, "beta")));
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, TwoTokenAvailable_OneUserInfo) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");
  SimulateTokenAvailable("beta");
  ReturnAccountInfoFetchSuccess("beta");
  ASSERT_FALSE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "beta")));
  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(account_fetcher()->IsAllUserInfoFetched());
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));
  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, GetAccounts) {
  SimulateTokenAvailable("alpha");
  SimulateTokenAvailable("beta");
  SimulateTokenAvailable("gamma");
  ReturnAccountInfoFetchSuccess("alpha");
  ReturnAccountInfoFetchSuccess("beta");
  ReturnAccountInfoFetchSuccess("gamma");

  std::vector<AccountInfo> infos = account_tracker()->GetAccounts();

  EXPECT_EQ(3u, infos.size());
  CheckAccountDetails("alpha", infos[0]);
  CheckAccountDetails("beta", infos[1]);
  CheckAccountDetails("gamma", infos[2]);
}

TEST_F(AccountTrackerServiceTest, GetAccountInfo_Empty) {
  AccountInfo info = account_tracker()->GetAccountInfo("alpha");
  ASSERT_EQ("", info.account_id);
}

TEST_F(AccountTrackerServiceTest, GetAccountInfo_TokenAvailable) {
  SimulateTokenAvailable("alpha");
  AccountInfo info = account_tracker()->GetAccountInfo("alpha");
  ASSERT_EQ("alpha", info.account_id);
  ASSERT_EQ("", info.gaia);
  ASSERT_EQ("", info.email);
}

TEST_F(AccountTrackerServiceTest, GetAccountInfo_TokenAvailable_UserInfo) {
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");
  AccountInfo info = account_tracker()->GetAccountInfo("alpha");
  CheckAccountDetails("alpha", info);
}

TEST_F(AccountTrackerServiceTest, GetAccountInfo_TokenAvailable_EnableNetwork) {
  // Shutdown the network-enabled tracker built into the test case.
  TearDown();

  // Create an account tracker and an account fetcher service but do not enable
  // network fetches.
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());

  AccountFetcherService fetcher_service;
  fetcher_service.Initialize(signin_client(), token_service(), &tracker,
                             std::make_unique<TestImageDecoder>());

  SimulateTokenAvailable("alpha");
  IssueAccessToken("alpha");
  // No fetcher has been created yet.
  net::TestURLFetcher* fetcher = test_fetcher_factory()->GetFetcherByID(
      gaia::GaiaOAuthClient::kUrlFetcherId);
  ASSERT_FALSE(fetcher);

  // Enable the network to create the fetcher then issue the access token.
  fetcher_service.EnableNetworkFetchesForTest();

  // Fetcher was created and executes properly.
  ReturnAccountInfoFetchSuccess("alpha");

  AccountInfo info = tracker.GetAccountInfo("alpha");
  CheckAccountDetails("alpha", info);
  fetcher_service.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, FindAccountInfoByGaiaId) {
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");

  std::string gaia_id = AccountIdToGaiaId("alpha");
  AccountInfo info = account_tracker()->FindAccountInfoByGaiaId(gaia_id);
  ASSERT_EQ("alpha", info.account_id);
  ASSERT_EQ(gaia_id, info.gaia);

  gaia_id = AccountIdToGaiaId("beta");
  info = account_tracker()->FindAccountInfoByGaiaId(gaia_id);
  ASSERT_EQ("", info.account_id);
}

TEST_F(AccountTrackerServiceTest, FindAccountInfoByEmail) {
  SimulateTokenAvailable("alpha");
  ReturnAccountInfoFetchSuccess("alpha");

  std::string email = AccountIdToEmail("alpha");
  AccountInfo info = account_tracker()->FindAccountInfoByEmail(email);
  ASSERT_EQ("alpha", info.account_id);
  ASSERT_EQ(email, info.email);

  // Should also work with "canonically-equal" email addresses.
  info = account_tracker()->FindAccountInfoByEmail("Alpha@Gmail.COM");
  ASSERT_EQ("alpha", info.account_id);
  ASSERT_EQ(email, info.email);
  info = account_tracker()->FindAccountInfoByEmail("al.pha@gmail.com");
  ASSERT_EQ("alpha", info.account_id);
  ASSERT_EQ(email, info.email);

  email = AccountIdToEmail("beta");
  info = account_tracker()->FindAccountInfoByEmail(email);
  ASSERT_EQ("", info.account_id);
}

TEST_F(AccountTrackerServiceTest, Persistence) {
  // Define a user data directory for the account image storage.
  base::ScopedTempDir scoped_user_data_dir;
  ASSERT_TRUE(scoped_user_data_dir.CreateUniqueTempDir());
  // Create a tracker and add two accounts.  This should cause the accounts
  // to be saved to persistence.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client(), scoped_user_data_dir.GetPath());
    SimulateTokenAvailable("alpha");
    ReturnAccountInfoFetchSuccess("alpha");
    ReturnAccountImageFetchSuccess("alpha");
    SimulateTokenAvailable("beta");
    ReturnAccountInfoFetchSuccess("beta");
    ReturnAccountImageFetchSuccess("beta");
    tracker.Shutdown();
  }

  // Create a new tracker and make sure it loads the accounts (including the
  // images) correctly from persistence.
  {
    AccountTrackerService tracker;
    AccountTrackerObserver observer;
    tracker.AddObserver(&observer);
    tracker.Initialize(signin_client(), scoped_user_data_dir.GetPath());
    ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha"),
                                     TrackingEvent(UPDATED, "beta")));
    // Wait until all account images are loaded.
    scoped_task_environment_.RunUntilIdle();
    ASSERT_TRUE(observer.CheckEvents(TrackingEvent(IMAGE_UPDATED, "alpha"),
                                     TrackingEvent(IMAGE_UPDATED, "beta")));

    tracker.RemoveObserver(&observer);

    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(2u, infos.size());
    CheckAccountDetails("alpha", infos[0]);
    CheckAccountDetails("beta", infos[1]);

    FakeAccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());
    fetcher.EnableNetworkFetchesForTest();
    // Remove an account.
    // This will allow testing removal as well as child accounts which is only
    // allowed for a single account.
    SimulateTokenRevoked("alpha");
#if defined(OS_ANDROID)
    fetcher.FakeSetIsChildAccount("beta", true);
#else
    tracker.SetIsChildAccount("beta", true);
#endif

    fetcher.Shutdown();
    tracker.Shutdown();
  }

  // Create a new tracker and make sure it loads the single account from
  // persistence. Also verify it is a child account.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());

    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(1u, infos.size());
    CheckAccountDetails("beta", infos[0]);
    ASSERT_TRUE(infos[0].is_child_account);
    tracker.Shutdown();
  }
}

TEST_F(AccountTrackerServiceTest, SeedAccountInfo) {
  std::vector<AccountInfo> infos = account_tracker()->GetAccounts();
  EXPECT_EQ(0u, infos.size());

  const std::string gaia_id = AccountIdToGaiaId("alpha");
  const std::string email = AccountIdToEmail("alpha");
  const std::string account_id =
      account_tracker()->PickAccountIdForAccount(gaia_id, email);
  account_tracker()->SeedAccountInfo(gaia_id, email);

  infos = account_tracker()->GetAccounts();
  EXPECT_EQ(1u, infos.size());
  EXPECT_EQ(account_id, infos[0].account_id);
  EXPECT_EQ(gaia_id, infos[0].gaia);
  EXPECT_EQ(email, infos[0].email);
}

TEST_F(AccountTrackerServiceTest, SeedAccountInfoFull) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);

  AccountInfo info;
  info.gaia = AccountIdToGaiaId("alpha");
  info.email = AccountIdToEmail("alpha");
  info.full_name = AccountIdToFullName("alpha");
  info.account_id = account_tracker()->SeedAccountInfo(info);

  // Validate that seeding an unexisting account works and sends a notification.
  AccountInfo stored_info = account_tracker()->GetAccountInfo(info.account_id);
  EXPECT_EQ(info.gaia, stored_info.gaia);
  EXPECT_EQ(info.email, stored_info.email);
  EXPECT_EQ(info.full_name, stored_info.full_name);
  EXPECT_TRUE(
      observer.CheckEvents(TrackingEvent(UPDATED, info.account_id, info.gaia)));

  // Validate that seeding new full informations to an existing account works
  // and sends a notification.
  info.given_name = AccountIdToGivenName("alpha");
  info.hosted_domain = AccountTrackerService::kNoHostedDomainFound;
  info.locale = AccountIdToLocale("alpha");
  info.picture_url = AccountIdToPictureURL("alpha");
  account_tracker()->SeedAccountInfo(info);
  stored_info = account_tracker()->GetAccountInfo(info.account_id);
  EXPECT_EQ(info.gaia, stored_info.gaia);
  EXPECT_EQ(info.email, stored_info.email);
  EXPECT_EQ(info.given_name, stored_info.given_name);
  EXPECT_TRUE(
      observer.CheckEvents(TrackingEvent(UPDATED, info.account_id, info.gaia)));

  // Validate that seeding invalid information to an existing account doesn't
  // work and doesn't send a notification.
  info.given_name = AccountIdToGivenName("beta");
  account_tracker()->SeedAccountInfo(info);
  stored_info = account_tracker()->GetAccountInfo(info.account_id);
  EXPECT_EQ(info.gaia, stored_info.gaia);
  EXPECT_NE(info.given_name, stored_info.given_name);
  EXPECT_TRUE(observer.CheckEvents());

  account_tracker()->RemoveObserver(&observer);
}

TEST_F(AccountTrackerServiceTest, UpgradeToFullAccountInfo) {
  // Start by simulating an incomplete account info and let it be saved to
  // prefs.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());
    fetcher.EnableNetworkFetchesForTest();
    SimulateTokenAvailable("incomplete");
    ReturnAccountInfoFetchSuccessIncomplete("incomplete");
    tracker.Shutdown();
    fetcher.Shutdown();
  }

  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());
    fetcher.EnableNetworkFetchesForTest();
    // Validate that the loaded AccountInfo from prefs is considered invalid.
    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(1u, infos.size());
    ASSERT_FALSE(infos[0].IsValid());

    // Simulate the same account getting a refresh token with all the info.
    SimulateTokenAvailable("incomplete");
    ReturnAccountInfoFetchSuccess("incomplete");

    // Validate that the account is now considered valid.
    infos = tracker.GetAccounts();
    ASSERT_EQ(1u, infos.size());
    ASSERT_TRUE(infos[0].IsValid());

    tracker.Shutdown();
    fetcher.Shutdown();
  }

  // Reinstantiate a tracker to validate that the AccountInfo saved to prefs is
  // now the upgraded one, considered valid.
  {
    AccountTrackerService tracker;
    AccountTrackerObserver observer;
    tracker.AddObserver(&observer);
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());

    ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "incomplete")));
    // Enabling network fetches shouldn't cause any actual fetch since the
    // AccountInfos loaded from prefs should be valid.
    fetcher.EnableNetworkFetchesForTest();

    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(1u, infos.size());
    ASSERT_TRUE(infos[0].IsValid());
    // Check that no network fetches were made.
    ASSERT_TRUE(observer.CheckEvents());

    tracker.RemoveObserver(&observer);
    tracker.Shutdown();
    fetcher.Shutdown();
  }
}

TEST_F(AccountTrackerServiceTest, TimerRefresh) {
  // Start by creating a tracker and adding a couple accounts to be persisted to
  // prefs.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());
    fetcher.EnableNetworkFetchesForTest();
    SimulateTokenAvailable("alpha");
    ReturnAccountInfoFetchSuccess("alpha");
    SimulateTokenAvailable("beta");
    ReturnAccountInfoFetchSuccess("beta");
    tracker.Shutdown();
    fetcher.Shutdown();
  }

  // Rewind the time by half a day, which shouldn't be enough to trigger a
  // network refresh.
  base::Time fake_update = base::Time::Now() - base::TimeDelta::FromHours(12);
  signin_client()->GetPrefs()->SetInt64(
      AccountFetcherService::kLastUpdatePref,
      fake_update.ToInternalValue());

  // Instantiate a new ATS, making sure the persisted accounts are still there
  // and that no network fetches happen.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());

    ASSERT_TRUE(fetcher.IsAllUserInfoFetched());
    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(2u, infos.size());
    ASSERT_TRUE(infos[0].IsValid());
    ASSERT_TRUE(infos[1].IsValid());

    fetcher.EnableNetworkFetchesForTest();
    ASSERT_TRUE(fetcher.IsAllUserInfoFetched());
    tracker.Shutdown();
    fetcher.Shutdown();
  }

  // Rewind the last updated time enough to trigger a network refresh.
  fake_update = base::Time::Now() - base::TimeDelta::FromHours(25);
  signin_client()->GetPrefs()->SetInt64(
      AccountFetcherService::kLastUpdatePref,
      fake_update.ToInternalValue());

  // Instantiate a new tracker and validate that even though the AccountInfos
  // are still valid, the network fetches are started.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());

    ASSERT_TRUE(fetcher.IsAllUserInfoFetched());
    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(2u, infos.size());
    ASSERT_TRUE(infos[0].IsValid());
    ASSERT_TRUE(infos[1].IsValid());

    fetcher.EnableNetworkFetchesForTest();
    ASSERT_FALSE(fetcher.IsAllUserInfoFetched());
    tracker.Shutdown();
    fetcher.Shutdown();
  }
}

TEST_F(AccountTrackerServiceTest, LegacyDottedAccountIds) {
  // Start by creating a tracker and adding an account with a dotted account id
  // because of an old bug in token service.  The token service would also add
  // a correct non-dotted account id for the same account.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());
    fetcher.EnableNetworkFetchesForTest();
    SimulateTokenAvailable("foo.bar@gmail.com");
    SimulateTokenAvailable("foobar@gmail.com");
    ReturnAccountInfoFetchSuccess("foo.bar@gmail.com");
    ReturnAccountInfoFetchSuccess("foobar@gmail.com");
    tracker.Shutdown();
    fetcher.Shutdown();
  }

  // Remove the bad account now from the token service to simulate that it
  // has been "fixed".
  SimulateTokenRevoked("foo.bar@gmail.com");

  // Instantiate a new tracker and validate that it has only one account, and
  // it is the correct non dotted one.
  {
    AccountTrackerService tracker;
    tracker.Initialize(signin_client());
    AccountFetcherService fetcher;
    fetcher.Initialize(signin_client(), token_service(), &tracker,
                       std::make_unique<TestImageDecoder>());

    ASSERT_TRUE(fetcher.IsAllUserInfoFetched());
    std::vector<AccountInfo> infos = tracker.GetAccounts();
    ASSERT_EQ(1u, infos.size());
    ASSERT_STREQ("foobar@gmail.com", infos[0].account_id.c_str());
    tracker.Shutdown();
    fetcher.Shutdown();
  }
}

TEST_F(AccountTrackerServiceTest, MigrateAccountIdToGaiaId) {
  if (account_tracker()->GetMigrationState() !=
      AccountTrackerService::MIGRATION_NOT_STARTED) {
    AccountTrackerService tracker;
    TestingPrefServiceSimple pref;
    AccountInfo account_info;

    std::string email_alpha = AccountIdToEmail("alpha");
    std::string gaia_alpha = AccountIdToGaiaId("alpha");
    std::string email_beta = AccountIdToEmail("beta");
    std::string gaia_beta = AccountIdToGaiaId("beta");

    pref.registry()->RegisterListPref(AccountTrackerService::kAccountInfoPref);
    pref.registry()->RegisterIntegerPref(
        prefs::kAccountIdMigrationState,
        AccountTrackerService::MIGRATION_NOT_STARTED);

    ListPrefUpdate update(&pref, AccountTrackerService::kAccountInfoPref);

    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_alpha));
    dict->SetString("email", base::UTF8ToUTF16(email_alpha));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_alpha));
    update->Append(std::move(dict));

    dict.reset(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_beta));
    dict->SetString("email", base::UTF8ToUTF16(email_beta));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_beta));
    update->Append(std::move(dict));

    std::unique_ptr<TestSigninClient> client;
    client.reset(new TestSigninClient(&pref));
    tracker.Initialize(client.get());

    ASSERT_EQ(tracker.GetMigrationState(),
              AccountTrackerService::MIGRATION_IN_PROGRESS);

    account_info = tracker.GetAccountInfo(gaia_alpha);
    ASSERT_EQ(account_info.account_id, gaia_alpha);
    ASSERT_EQ(account_info.gaia, gaia_alpha);
    ASSERT_EQ(account_info.email, email_alpha);

    account_info = tracker.GetAccountInfo(gaia_beta);
    ASSERT_EQ(account_info.account_id, gaia_beta);
    ASSERT_EQ(account_info.gaia, gaia_beta);
    ASSERT_EQ(account_info.email, email_beta);

    std::vector<AccountInfo> accounts = tracker.GetAccounts();
    ASSERT_EQ(2u, accounts.size());
  }
}

TEST_F(AccountTrackerServiceTest, CanNotMigrateAccountIdToGaiaId) {
  if ((account_tracker()->GetMigrationState() !=
       AccountTrackerService::MIGRATION_NOT_STARTED)) {
    AccountTrackerService tracker;
    TestingPrefServiceSimple pref;
    AccountInfo account_info;

    std::string email_alpha = AccountIdToEmail("alpha");
    std::string gaia_alpha = AccountIdToGaiaId("alpha");
    std::string email_beta = AccountIdToEmail("beta");

    pref.registry()->RegisterListPref(AccountTrackerService::kAccountInfoPref);
    pref.registry()->RegisterIntegerPref(
        prefs::kAccountIdMigrationState,
        AccountTrackerService::MIGRATION_NOT_STARTED);

    ListPrefUpdate update(&pref, AccountTrackerService::kAccountInfoPref);

    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_alpha));
    dict->SetString("email", base::UTF8ToUTF16(email_alpha));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_alpha));
    update->Append(std::move(dict));

    dict.reset(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_beta));
    dict->SetString("email", base::UTF8ToUTF16(email_beta));
    dict->SetString("gaia", base::UTF8ToUTF16(std::string()));
    update->Append(std::move(dict));

    std::unique_ptr<TestSigninClient> client;
    client.reset(new TestSigninClient(&pref));
    tracker.Initialize(client.get());

    ASSERT_EQ(tracker.GetMigrationState(),
              AccountTrackerService::MIGRATION_NOT_STARTED);

    account_info = tracker.GetAccountInfo(email_alpha);
    ASSERT_EQ(account_info.account_id, email_alpha);
    ASSERT_EQ(account_info.gaia, gaia_alpha);
    ASSERT_EQ(account_info.email, email_alpha);

    account_info = tracker.GetAccountInfo(email_beta);
    ASSERT_EQ(account_info.account_id, email_beta);
    ASSERT_EQ(account_info.email, email_beta);

    std::vector<AccountInfo> accounts = tracker.GetAccounts();
    ASSERT_EQ(2u, accounts.size());
  }
}

TEST_F(AccountTrackerServiceTest, GaiaIdMigrationCrashInTheMiddle) {
  if (account_tracker()->GetMigrationState() !=
      AccountTrackerService::MIGRATION_NOT_STARTED) {
    AccountTrackerService tracker;
    TestingPrefServiceSimple pref;
    AccountInfo account_info;

    std::string email_alpha = AccountIdToEmail("alpha");
    std::string gaia_alpha = AccountIdToGaiaId("alpha");
    std::string email_beta = AccountIdToEmail("beta");
    std::string gaia_beta = AccountIdToGaiaId("beta");

    pref.registry()->RegisterListPref(AccountTrackerService::kAccountInfoPref);
    pref.registry()->RegisterIntegerPref(
        prefs::kAccountIdMigrationState,
        AccountTrackerService::MIGRATION_IN_PROGRESS);

    ListPrefUpdate update(&pref, AccountTrackerService::kAccountInfoPref);

    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_alpha));
    dict->SetString("email", base::UTF8ToUTF16(email_alpha));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_alpha));
    update->Append(std::move(dict));

    dict.reset(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(email_beta));
    dict->SetString("email", base::UTF8ToUTF16(email_beta));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_beta));
    update->Append(std::move(dict));

    // Succeed miggrated account.
    dict.reset(new base::DictionaryValue());
    dict->SetString("account_id", base::UTF8ToUTF16(gaia_alpha));
    dict->SetString("email", base::UTF8ToUTF16(email_alpha));
    dict->SetString("gaia", base::UTF8ToUTF16(gaia_alpha));
    update->Append(std::move(dict));

    std::unique_ptr<TestSigninClient> client;
    client.reset(new TestSigninClient(&pref));
    tracker.Initialize(client.get());

    ASSERT_EQ(tracker.GetMigrationState(),
              AccountTrackerService::MIGRATION_IN_PROGRESS);

    account_info = tracker.GetAccountInfo(gaia_alpha);
    ASSERT_EQ(account_info.account_id, gaia_alpha);
    ASSERT_EQ(account_info.gaia, gaia_alpha);
    ASSERT_EQ(account_info.email, email_alpha);

    account_info = tracker.GetAccountInfo(gaia_beta);
    ASSERT_EQ(account_info.account_id, gaia_beta);
    ASSERT_EQ(account_info.gaia, gaia_beta);
    ASSERT_EQ(account_info.email, email_beta);

    std::vector<AccountInfo> accounts = tracker.GetAccounts();
    ASSERT_EQ(2u, accounts.size());

    tracker.SetMigrationDone();
    tracker.Shutdown();
    tracker.Initialize(client.get());

    ASSERT_EQ(tracker.GetMigrationState(),
              AccountTrackerService::MIGRATION_DONE);

    account_info = tracker.GetAccountInfo(gaia_alpha);
    ASSERT_EQ(account_info.account_id, gaia_alpha);
    ASSERT_EQ(account_info.gaia, gaia_alpha);
    ASSERT_EQ(account_info.email, email_alpha);

    account_info = tracker.GetAccountInfo(gaia_beta);
    ASSERT_EQ(account_info.account_id, gaia_beta);
    ASSERT_EQ(account_info.gaia, gaia_beta);
    ASSERT_EQ(account_info.email, email_beta);

    accounts.clear();
    accounts = tracker.GetAccounts();
    ASSERT_EQ(2u, accounts.size());
  }
}

TEST_F(AccountTrackerServiceTest, ChildAccountBasic) {
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());
  FakeAccountFetcherService fetcher;
  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  AccountTrackerObserver observer;
  tracker.AddObserver(&observer);
  std::string child_id("child");
  {
    SimulateTokenAvailable(child_id);
    IssueAccessToken(child_id);
#if defined(OS_ANDROID)
    fetcher.FakeSetIsChildAccount(child_id, true);
#else
    tracker.SetIsChildAccount(child_id, true);
#endif
    // Response was processed but observer is not notified as the account state
    // is invalid.
    ASSERT_TRUE(observer.CheckEvents());
    AccountInfo info = tracker.GetAccountInfo(child_id);
    ASSERT_TRUE(info.is_child_account);
    SimulateTokenRevoked(child_id);
  }
  tracker.RemoveObserver(&observer);
  fetcher.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, ChildAccountUpdatedAndRevoked) {
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());
  FakeAccountFetcherService fetcher;
  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  AccountTrackerObserver observer;
  tracker.AddObserver(&observer);
  std::string child_id("child");

  SimulateTokenAvailable(child_id);
  IssueAccessToken(child_id);
#if defined(OS_ANDROID)
  fetcher.FakeSetIsChildAccount(child_id, false);
#else
  tracker.SetIsChildAccount(child_id, false);
#endif
  FakeUserInfoFetchSuccess(&fetcher, child_id);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id)));
  AccountInfo info = tracker.GetAccountInfo(child_id);
  ASSERT_FALSE(info.is_child_account);
  SimulateTokenRevoked(child_id);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, child_id)));

  tracker.RemoveObserver(&observer);
  fetcher.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, ChildAccountUpdatedAndRevokedWithUpdate) {
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());
  FakeAccountFetcherService fetcher;
  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  AccountTrackerObserver observer;
  tracker.AddObserver(&observer);
  std::string child_id("child");

  SimulateTokenAvailable(child_id);
  IssueAccessToken(child_id);
#if defined(OS_ANDROID)
  fetcher.FakeSetIsChildAccount(child_id, true);
#else
  tracker.SetIsChildAccount(child_id, true);
#endif
  FakeUserInfoFetchSuccess(&fetcher, child_id);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id)));
  AccountInfo info = tracker.GetAccountInfo(child_id);
  ASSERT_TRUE(info.is_child_account);
  SimulateTokenRevoked(child_id);
#if defined(OS_ANDROID)
  // On Android, is_child_account is set to false before removing it.
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id),
                                   TrackingEvent(REMOVED, child_id)));
#else
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, child_id)));
#endif

  tracker.RemoveObserver(&observer);
  fetcher.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, ChildAccountUpdatedTwiceThenRevoked) {
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());
  FakeAccountFetcherService fetcher;
  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  AccountTrackerObserver observer;
  tracker.AddObserver(&observer);
  std::string child_id("child");

  SimulateTokenAvailable(child_id);
  IssueAccessToken(child_id);
  // Observers notified the first time.
  FakeUserInfoFetchSuccess(&fetcher, child_id);
  // Since the account state is already valid, this will notify the
  // observers for the second time.
#if defined(OS_ANDROID)
  fetcher.FakeSetIsChildAccount(child_id, true);
#else
  tracker.SetIsChildAccount(child_id, true);
#endif
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id),
                                   TrackingEvent(UPDATED, child_id)));
  SimulateTokenRevoked(child_id);
#if defined(OS_ANDROID)
  // On Android, is_child_account is set to false before removing it.
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id),
                                   TrackingEvent(REMOVED, child_id)));
#else
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, child_id)));
#endif

  tracker.RemoveObserver(&observer);
  fetcher.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, ChildAccountGraduation) {
  AccountTrackerService tracker;
  tracker.Initialize(signin_client());
  FakeAccountFetcherService fetcher;
  fetcher.Initialize(signin_client(), token_service(), &tracker,
                     std::make_unique<TestImageDecoder>());
  fetcher.EnableNetworkFetchesForTest();
  AccountTrackerObserver observer;
  tracker.AddObserver(&observer);
  std::string child_id("child");

  SimulateTokenAvailable(child_id);
  IssueAccessToken(child_id);

  // Set and verify this is a child account.
#if defined(OS_ANDROID)
  fetcher.FakeSetIsChildAccount(child_id, true);
#else
  tracker.SetIsChildAccount(child_id, true);
#endif
  AccountInfo info = tracker.GetAccountInfo(child_id);
  ASSERT_TRUE(info.is_child_account);
  FakeUserInfoFetchSuccess(&fetcher, child_id);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id)));

  // Now simulate child account graduation.
#if defined(OS_ANDROID)
  fetcher.FakeSetIsChildAccount(child_id, false);
#else
  tracker.SetIsChildAccount(child_id, false);
#endif
  info = tracker.GetAccountInfo(child_id);
  ASSERT_FALSE(info.is_child_account);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, child_id)));

  SimulateTokenRevoked(child_id);
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, child_id)));

  tracker.RemoveObserver(&observer);
  fetcher.Shutdown();
  tracker.Shutdown();
}

TEST_F(AccountTrackerServiceTest, RemoveAccountBeforeImageFetchDone) {
  AccountTrackerObserver observer;
  account_tracker()->AddObserver(&observer);
  SimulateTokenAvailable("alpha");

  ReturnAccountInfoFetchSuccess("alpha");
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(UPDATED, "alpha")));

  SimulateTokenRevoked("alpha");
  ReturnAccountImageFetchFailure("alpha");
  ASSERT_TRUE(observer.CheckEvents(TrackingEvent(REMOVED, "alpha")));

  account_tracker()->RemoveObserver(&observer);
}
