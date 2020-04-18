// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/child_accounts/child_account_service.h"

#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/fake_gaia_cookie_manager_service_builder.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/supervised_user/child_accounts/child_account_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/fake_gaia_cookie_manager_service.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::unique_ptr<KeyedService> BuildTestSigninClient(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<TestSigninClient>(profile->GetPrefs());
}

class ChildAccountServiceTest : public ::testing::Test {
 public:
  ChildAccountServiceTest() : fake_url_fetcher_factory_(nullptr) {}

  void SetUp() override {
    ChromeSigninClientFactory::GetInstance()->SetTestingFactory(
        &profile_, &BuildTestSigninClient);
    GaiaCookieManagerServiceFactory::GetInstance()->SetTestingFactory(
        &profile_, &BuildFakeGaiaCookieManagerService);
    gaia_cookie_manager_service_ = static_cast<FakeGaiaCookieManagerService*>(
        GaiaCookieManagerServiceFactory::GetForProfile(&profile_));
    gaia_cookie_manager_service_->Init(&fake_url_fetcher_factory_);
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  net::FakeURLFetcherFactory fake_url_fetcher_factory_;
  FakeGaiaCookieManagerService* gaia_cookie_manager_service_ = nullptr;
};

TEST_F(ChildAccountServiceTest, GetGoogleAuthState) {
  gaia_cookie_manager_service_->SetListAccountsResponseNoAccounts();

  ChildAccountService* child_account_service =
      ChildAccountServiceFactory::GetForProfile(&profile_);

  // Initial state should be PENDING.
  EXPECT_EQ(ChildAccountService::AuthState::PENDING,
            child_account_service->GetGoogleAuthState());

  // Wait until the response to the ListAccount request triggered by the call
  // above comes back.
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(ChildAccountService::AuthState::NOT_AUTHENTICATED,
            child_account_service->GetGoogleAuthState());

  // A valid, signed-in account means authenticated.
  gaia_cookie_manager_service_->SetListAccountsResponseOneAccountWithParams(
      "me@example.com", "abcdef",
      /* is_email_valid = */ true,
      /* is_signed_out = */ false,
      /* verified = */ true);
  gaia_cookie_manager_service_->TriggerListAccounts("ChildAccountServiceTest");
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(ChildAccountService::AuthState::AUTHENTICATED,
            child_account_service->GetGoogleAuthState());

  // An invalid (but signed-in) account means not authenticated.
  gaia_cookie_manager_service_->SetListAccountsResponseOneAccountWithParams(
      "me@example.com", "abcdef",
      /* is_email_valid = */ false,
      /* is_signed_out = */ false,
      /* verified = */ true);
  gaia_cookie_manager_service_->TriggerListAccounts("ChildAccountServiceTest");
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(ChildAccountService::AuthState::NOT_AUTHENTICATED,
            child_account_service->GetGoogleAuthState());

  // A valid but not signed-in account means not authenticated.
  gaia_cookie_manager_service_->SetListAccountsResponseOneAccountWithParams(
      "me@example.com", "abcdef",
      /* is_email_valid = */ true,
      /* is_signed_out = */ true,
      /* verified = */ true);
  gaia_cookie_manager_service_->TriggerListAccounts("ChildAccountServiceTest");
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(ChildAccountService::AuthState::NOT_AUTHENTICATED,
            child_account_service->GetGoogleAuthState());
}

}  // namespace
