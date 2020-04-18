// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_client.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/common/network_connection_tracker.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// ChromeOS has its own network delay logic.
#if !defined(OS_CHROMEOS)

namespace {

class MockNetworkConnectionTrackerNeverOffline
    : public content::NetworkConnectionTracker {
 public:
  MockNetworkConnectionTrackerNeverOffline()
      : content::NetworkConnectionTracker() {}
  ~MockNetworkConnectionTrackerNeverOffline() override {}

  bool GetConnectionType(network::mojom::ConnectionType* type,
                         ConnectionTypeCallback callback) override {
    *type = network::mojom::ConnectionType::CONNECTION_3G;
    return true;
  }
};

class MockNetworkConnectionTrackerGetConnectionTypeAsync
    : public content::NetworkConnectionTracker {
 public:
  MockNetworkConnectionTrackerGetConnectionTypeAsync()
      : content::NetworkConnectionTracker() {}
  ~MockNetworkConnectionTrackerGetConnectionTypeAsync() override {}

  void CompleteCallback() {
    OnInitialConnectionType(network::mojom::ConnectionType::CONNECTION_3G);
  }
};

class MockNetworkConnectionTrackerOfflineUntilChange
    : public content::NetworkConnectionTracker {
 public:
  MockNetworkConnectionTrackerOfflineUntilChange()
      : content::NetworkConnectionTracker(), online_(false) {}
  ~MockNetworkConnectionTrackerOfflineUntilChange() override {}

  bool GetConnectionType(network::mojom::ConnectionType* type,
                         ConnectionTypeCallback callback) override {
    if (online_) {
      *type = network::mojom::ConnectionType::CONNECTION_3G;
    } else {
      *type = network::mojom::ConnectionType::CONNECTION_NONE;
    }
    return true;
  }
  void GoOnline() {
    online_ = true;
    OnNetworkChanged(network::mojom::ConnectionType::CONNECTION_3G);
  }
 private:
  bool online_;
};

class CallbackTester {
 public:
  CallbackTester() : called_(0) {}

  void Increment();
  bool WasCalledExactlyOnce();

 private:
  int called_;
};

void CallbackTester::Increment() {
  called_++;
}

bool CallbackTester::WasCalledExactlyOnce() {
  return called_ == 1;
}

}  // namespace

class ChromeSigninClientTest : public testing::Test {
 public:
  ChromeSigninClientTest() {}

  void Initialize(std::unique_ptr<content::NetworkConnectionTracker> tracker) {
    TestingBrowserProcess::GetGlobal()->SetNetworkConnectionTracker(
        std::move(tracker));
    // Create a signed-in profile.
    TestingProfile::Builder builder;
    profile_ = builder.Build();

    signin_client_ = ChromeSigninClientFactory::GetForProfile(profile());
  }

  Profile* profile() { return profile_.get(); }
  SigninClient* signin_client() { return signin_client_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
  SigninClient* signin_client_;
};

TEST_F(ChromeSigninClientTest, DelayNetworkCallRunsImmediatelyWithNetwork) {
  Initialize(std::make_unique<MockNetworkConnectionTrackerNeverOffline>());
  CallbackTester tester;
  signin_client()->DelayNetworkCall(
      base::Bind(&CallbackTester::Increment, base::Unretained(&tester)));
  ASSERT_TRUE(tester.WasCalledExactlyOnce());
}

TEST_F(ChromeSigninClientTest, DelayNetworkCallRunsAfterGetConnectionType) {
  auto tracker =
      std::make_unique<MockNetworkConnectionTrackerGetConnectionTypeAsync>();
  MockNetworkConnectionTrackerGetConnectionTypeAsync* mock = tracker.get();
  Initialize(std::move(tracker));

  CallbackTester tester;
  signin_client()->DelayNetworkCall(base::Bind(&CallbackTester::Increment,
                                               base::Unretained(&tester)));
  ASSERT_FALSE(tester.WasCalledExactlyOnce());
  mock->CompleteCallback();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tester.WasCalledExactlyOnce());
}

TEST_F(ChromeSigninClientTest, DelayNetworkCallRunsAfterNetworkChange) {
  auto tracker =
      std::make_unique<MockNetworkConnectionTrackerOfflineUntilChange>();
  MockNetworkConnectionTrackerOfflineUntilChange* mock = tracker.get();
  Initialize(std::move(tracker));

  CallbackTester tester;
  signin_client()->DelayNetworkCall(base::Bind(&CallbackTester::Increment,
                                               base::Unretained(&tester)));
  ASSERT_FALSE(tester.WasCalledExactlyOnce());
  mock->GoOnline();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tester.WasCalledExactlyOnce());
}

#if !defined(OS_ANDROID)

class MockChromeSigninClient : public ChromeSigninClient {
 public:
  MockChromeSigninClient(Profile* profile, SigninErrorController* controller)
      : ChromeSigninClient(profile, controller) {}

  MOCK_METHOD1(ShowUserManager, void(const base::FilePath&));
  MOCK_METHOD1(LockForceSigninProfile, void(const base::FilePath&));
};

class MockSigninManager : public SigninManager {
 public:
  explicit MockSigninManager(SigninClient* client,
                             SigninErrorController* signin_error_controller)
      : SigninManager(client,
                      nullptr,
                      &fake_service_,
                      nullptr,
                      signin_error_controller,
                      signin::AccountConsistencyMethod::kDisabled) {
    DCHECK(signin_error_controller);
  }

  MOCK_METHOD3(DoSignOut,
               void(signin_metrics::ProfileSignout,
                    signin_metrics::SignoutDelete,
                    RemoveAccountsOption remove_option));

  AccountTrackerService fake_service_;
};

class ChromeSigninClientSignoutTest : public BrowserWithTestWindowTest {
 public:
  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    signin_util::SetForceSigninForTesting(true);
    CreateClient(browser()->profile());
    manager_ = std::make_unique<MockSigninManager>(client_.get(),
                                                   fake_controller_.get());
  }

  void TearDown() override {
    BrowserWithTestWindowTest::TearDown();
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  void CreateClient(Profile* profile) {
    SigninErrorController* controller = new SigninErrorController(
        SigninErrorController::AccountMode::ANY_ACCOUNT);
    client_.reset(new MockChromeSigninClient(profile, controller));
    fake_controller_.reset(controller);
  }

  std::unique_ptr<SigninErrorController> fake_controller_;
  std::unique_ptr<MockChromeSigninClient> client_;
  std::unique_ptr<MockSigninManager> manager_;
};

TEST_F(ChromeSigninClientSignoutTest, SignOut) {
  signin_metrics::ProfileSignout source_metric =
      signin_metrics::ProfileSignout::ABORT_SIGNIN;
  signin_metrics::SignoutDelete delete_metric =
      signin_metrics::SignoutDelete::IGNORE_METRIC;

  EXPECT_CALL(*client_, ShowUserManager(browser()->profile()->GetPath()))
      .Times(1);
  EXPECT_CALL(*client_, LockForceSigninProfile(browser()->profile()->GetPath()))
      .Times(1);
  EXPECT_CALL(
      *manager_,
      DoSignOut(source_metric, delete_metric,
                SigninManager::RemoveAccountsOption::kRemoveAllAccounts))
      .Times(1);

  manager_->SignOut(source_metric, delete_metric);
}

TEST_F(ChromeSigninClientSignoutTest, SignOutWithoutManager) {
  signin_metrics::ProfileSignout source_metric =
      signin_metrics::ProfileSignout::ABORT_SIGNIN;
  signin_metrics::SignoutDelete delete_metric =
      signin_metrics::SignoutDelete::IGNORE_METRIC;

  MockSigninManager other_manager(client_.get(), fake_controller_.get());
  other_manager.CopyCredentialsFrom(*manager_.get());

  EXPECT_CALL(*client_, ShowUserManager(browser()->profile()->GetPath()))
      .Times(0);
  EXPECT_CALL(*client_, LockForceSigninProfile(browser()->profile()->GetPath()))
      .Times(1);
  EXPECT_CALL(
      *manager_,
      DoSignOut(source_metric, delete_metric,
                SigninManager::RemoveAccountsOption::kRemoveAllAccounts))
      .Times(1);
  manager_->SignOut(source_metric, delete_metric);

  ::testing::Mock::VerifyAndClearExpectations(manager_.get());

  EXPECT_CALL(*client_, ShowUserManager(browser()->profile()->GetPath()))
      .Times(1);
  EXPECT_CALL(*client_, LockForceSigninProfile(browser()->profile()->GetPath()))
      .Times(1);
  EXPECT_CALL(
      *manager_,
      DoSignOut(source_metric, delete_metric,
                SigninManager::RemoveAccountsOption::kRemoveAllAccounts))
      .Times(1);
  manager_->SignOut(source_metric, delete_metric);
}

TEST_F(ChromeSigninClientSignoutTest, SignOutWithoutForceSignin) {
  signin_util::SetForceSigninForTesting(false);
  CreateClient(browser()->profile());
  manager_ = std::make_unique<MockSigninManager>(client_.get(),
                                                 fake_controller_.get());

  signin_metrics::ProfileSignout source_metric =
      signin_metrics::ProfileSignout::ABORT_SIGNIN;
  signin_metrics::SignoutDelete delete_metric =
      signin_metrics::SignoutDelete::IGNORE_METRIC;

  EXPECT_CALL(*client_, ShowUserManager(browser()->profile()->GetPath()))
      .Times(0);
  EXPECT_CALL(*client_, LockForceSigninProfile(browser()->profile()->GetPath()))
      .Times(0);
  EXPECT_CALL(
      *manager_,
      DoSignOut(source_metric, delete_metric,
                SigninManager::RemoveAccountsOption::kRemoveAllAccounts))
      .Times(1);
  manager_->SignOut(source_metric, delete_metric);
}

TEST_F(ChromeSigninClientSignoutTest, SignOutGuestSession) {
  TestingProfile::Builder builder;
  builder.SetGuestSession();
  std::unique_ptr<TestingProfile> profile = builder.Build();

  CreateClient(profile.get());
  manager_ = std::make_unique<MockSigninManager>(client_.get(),
                                                 fake_controller_.get());

  signin_metrics::ProfileSignout source_metric =
      signin_metrics::ProfileSignout::ABORT_SIGNIN;
  signin_metrics::SignoutDelete delete_metric =
      signin_metrics::SignoutDelete::IGNORE_METRIC;

  EXPECT_CALL(*client_, ShowUserManager(browser()->profile()->GetPath()))
      .Times(0);
  EXPECT_CALL(*client_, LockForceSigninProfile(browser()->profile()->GetPath()))
      .Times(0);
  EXPECT_CALL(
      *manager_,
      DoSignOut(source_metric, delete_metric,
                SigninManager::RemoveAccountsOption::kRemoveAllAccounts))
      .Times(1);
  manager_->SignOut(source_metric, delete_metric);
}

#endif  // !defined(OS_ANDROID)
#endif  // !defined(OS_CHROMEOS)
