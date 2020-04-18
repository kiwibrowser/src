// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_tracker.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_tracker_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/browser_sync/profile_sync_service_mock.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

class MockObserver : public SigninTracker::Observer {
 public:
  MockObserver() {}
  ~MockObserver() {}

  MOCK_METHOD1(SigninFailed, void(const GoogleServiceAuthError&));
  MOCK_METHOD0(SigninSuccess, void(void));
  MOCK_METHOD1(AccountAddedToCookie, void(const GoogleServiceAuthError&));
};

}  // namespace

class SigninTrackerTest : public testing::Test {
 public:
  SigninTrackerTest() {}
  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(ProfileOAuth2TokenServiceFactory::GetInstance(),
                              BuildFakeProfileOAuth2TokenService);
    builder.AddTestingFactory(SigninManagerFactory::GetInstance(),
                              BuildFakeSigninManagerBase);
    profile_ = builder.Build();

    fake_oauth2_token_service_ =
        static_cast<FakeProfileOAuth2TokenService*>(
            ProfileOAuth2TokenServiceFactory::GetForProfile(profile_.get()));

    fake_signin_manager_ = static_cast<FakeSigninManagerForTesting*>(
        SigninManagerFactory::GetForProfile(profile_.get()));

    tracker_ =
        SigninTrackerFactory::CreateForProfile(profile_.get(), &observer_);
  }

  void TearDown() override {
    tracker_.reset();
    profile_.reset();
  }

  // Seed the account tracker with information from logged in user.  Normally
  // this is done by UI code before calling SigninManager.  Returns the string
  // to use as the account_id.
  std::string AddToAccountTracker(const std::string& gaia_id,
                                  const std::string& email) {
    AccountTrackerService* service =
        AccountTrackerServiceFactory::GetForProfile(profile_.get());
    return service->SeedAccountInfo(gaia_id, email);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<SigninTracker> tracker_;
  std::unique_ptr<TestingProfile> profile_;
  FakeSigninManagerForTesting* fake_signin_manager_;
  FakeProfileOAuth2TokenService* fake_oauth2_token_service_;
  MockObserver observer_;
};

#if !defined(OS_CHROMEOS)
TEST_F(SigninTrackerTest, SignInFails) {
  const GoogleServiceAuthError error(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);

  // Signin failure should result in a SigninFailed callback.
  EXPECT_CALL(observer_, SigninSuccess()).Times(0);
  EXPECT_CALL(observer_, SigninFailed(error));

  fake_signin_manager_->FailSignin(error);
}
#endif  // !defined(OS_CHROMEOS)

TEST_F(SigninTrackerTest, SignInSucceeds) {
  EXPECT_CALL(observer_, SigninSuccess());
  EXPECT_CALL(observer_, SigninFailed(_)).Times(0);

  AccountTrackerService* service =
    AccountTrackerServiceFactory::GetForProfile(profile_.get());
  std::string gaia_id = "gaia_id";
  std::string email = "user@gmail.com";
  std::string account_id = service->SeedAccountInfo(gaia_id, email);

  fake_signin_manager_->SetAuthenticatedAccountInfo(gaia_id, email);
  fake_oauth2_token_service_->UpdateCredentials(account_id, "refresh_token");
}

#if !defined(OS_CHROMEOS)
TEST_F(SigninTrackerTest, SignInSucceedsWithExistingAccount) {
  EXPECT_CALL(observer_, SigninSuccess());
  EXPECT_CALL(observer_, SigninFailed(_)).Times(0);

  AccountTrackerService* service =
      AccountTrackerServiceFactory::GetForProfile(profile_.get());
  std::string gaia_id = "gaia_id";
  std::string email = "user@gmail.com";
  std::string account_id = service->SeedAccountInfo(gaia_id, email);
  fake_oauth2_token_service_->UpdateCredentials(account_id, "refresh_token");
  fake_signin_manager_->SignIn(gaia_id, email, std::string());
}
#endif
