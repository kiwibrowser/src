// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/login_policy_test_base.h"

#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/user_policy_test_helper.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/gaia/fake_gaia.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

constexpr char kTestAuthCode[] = "fake-auth-code";
constexpr char kTestGaiaUberToken[] = "fake-uber-token";
constexpr char kTestAuthLoginAccessToken[] = "fake-access-token";
constexpr char kTestRefreshToken[] = "fake-refresh-token";
constexpr char kTestAuthSIDCookie[] = "fake-auth-SID-cookie";
constexpr char kTestAuthLSIDCookie[] = "fake-auth-LSID-cookie";
constexpr char kTestSessionSIDCookie[] = "fake-session-SID-cookie";
constexpr char kTestSessionLSIDCookie[] = "fake-session-LSID-cookie";

}  // namespace

const char LoginPolicyTestBase::kAccountPassword[] = "letmein";
const char LoginPolicyTestBase::kAccountId[] = "user@example.com";
// Empty services list for userInfo.
const char LoginPolicyTestBase::kEmptyServices[] = "[]";

LoginPolicyTestBase::LoginPolicyTestBase() {
  set_open_about_blank_on_browser_launch(false);
}

LoginPolicyTestBase::~LoginPolicyTestBase() {
}

void LoginPolicyTestBase::SetUp() {
  base::DictionaryValue mandatory;
  GetMandatoryPoliciesValue(&mandatory);
  base::DictionaryValue recommended;
  GetRecommendedPoliciesValue(&recommended);
  user_policy_helper_.reset(new UserPolicyTestHelper(GetAccount()));
  user_policy_helper_->Init(mandatory, recommended);
  OobeBaseTest::SetUp();
}

void LoginPolicyTestBase::SetUpCommandLine(base::CommandLine* command_line) {
  user_policy_helper_->UpdateCommandLine(command_line);
  OobeBaseTest::SetUpCommandLine(command_line);
}

void LoginPolicyTestBase::SetUpOnMainThread() {
  SetMergeSessionParams();
  SetupFakeGaiaForLogin(GetAccount(), "", kTestRefreshToken);
  OobeBaseTest::SetUpOnMainThread();

  FakeGaia::MergeSessionParams params;
  params.id_token = GetIdToken();
  fake_gaia_->UpdateMergeSessionParams(params);
}

std::string LoginPolicyTestBase::GetAccount() const {
  return kAccountId;
}

std::string LoginPolicyTestBase::GetIdToken() const {
  return std::string();
}

void LoginPolicyTestBase::GetMandatoryPoliciesValue(
    base::DictionaryValue* policy) const {
}

void LoginPolicyTestBase::GetRecommendedPoliciesValue(
    base::DictionaryValue* policy) const {
}

void LoginPolicyTestBase::SetMergeSessionParams() {
  FakeGaia::MergeSessionParams params;
  params.auth_sid_cookie = kTestAuthSIDCookie;
  params.auth_lsid_cookie = kTestAuthLSIDCookie;
  params.auth_code = kTestAuthCode;
  params.refresh_token = kTestRefreshToken;
  params.access_token = kTestAuthLoginAccessToken;
  params.id_token = GetIdToken();
  params.gaia_uber_token = kTestGaiaUberToken;
  params.session_sid_cookie = kTestSessionSIDCookie;
  params.session_lsid_cookie = kTestSessionLSIDCookie;
  params.email = GetAccount();
  fake_gaia_->SetMergeSessionParams(params);
}

void LoginPolicyTestBase::SkipToLoginScreen() {
  chromeos::WizardController::SkipPostLoginScreensForTesting();
  chromeos::WizardController* const wizard_controller =
      chromeos::WizardController::default_controller();
  ASSERT_TRUE(wizard_controller);
  wizard_controller->SkipToLoginForTesting(chromeos::LoginScreenContext());

  content::WindowedNotificationObserver(
      chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
      content::NotificationService::AllSources()).Wait();
}

void LoginPolicyTestBase::LogIn(const std::string& user_id,
                                const std::string& password,
                                const std::string& services) {
  GetLoginDisplay()->ShowSigninScreenForTest(user_id, password, services);

  content::WindowedNotificationObserver(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources()).Wait();
}

}  // namespace policy
