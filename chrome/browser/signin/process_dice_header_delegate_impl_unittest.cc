// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/process_dice_header_delegate_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Constants defined for a better formatting of the test tables:
const signin::AccountConsistencyMethod kDiceFixAuthErrors =
    signin::AccountConsistencyMethod::kDiceFixAuthErrors;
const signin::AccountConsistencyMethod kDice =
    signin::AccountConsistencyMethod::kDice;
const signin::AccountConsistencyMethod kDicePrepareMigration =
    signin::AccountConsistencyMethod::kDicePrepareMigration;

class ProcessDiceHeaderDelegateImplTest
    : public ChromeRenderViewHostTestHarness {
 public:
  ProcessDiceHeaderDelegateImplTest()
      : signin_client_(&pref_service_),
        signin_manager_(&signin_client_,
                        &token_service_,
                        &account_tracker_service_,
                        nullptr),
        enable_sync_called_(false),
        show_error_called_(false),
        account_id_("12345"),
        email_("foo@bar.com"),
        auth_error_(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS) {
    signin::RegisterAccountConsistencyProfilePrefs(pref_service_.registry());
    AccountTrackerService::RegisterPrefs(pref_service_.registry());
    SigninManager::RegisterProfilePrefs(pref_service_.registry());
    account_tracker_service_.Initialize(&signin_client_);
  }

  ~ProcessDiceHeaderDelegateImplTest() override {
    signin_manager_.Shutdown();
    account_tracker_service_.Shutdown();
    token_service_.Shutdown();
    signin_client_.Shutdown();
  }

  // Creates a ProcessDiceHeaderDelegateImpl instance.
  std::unique_ptr<ProcessDiceHeaderDelegateImpl> CreateDelegate(
      bool is_sync_signin_tab) {
    return std::make_unique<ProcessDiceHeaderDelegateImpl>(
        web_contents(), &pref_service_, &signin_manager_, is_sync_signin_tab,
        base::BindOnce(&ProcessDiceHeaderDelegateImplTest::StartSyncCallback,
                       base::Unretained(this)),
        base::BindOnce(
            &ProcessDiceHeaderDelegateImplTest::ShowSigninErrorCallback,
            base::Unretained(this)));
  }

  // Callback for the ProcessDiceHeaderDelegateImpl.
  void StartSyncCallback(content::WebContents* contents,
                         const std::string& account_id) {
    EXPECT_EQ(web_contents(), contents);
    EXPECT_EQ(account_id_, account_id);
    enable_sync_called_ = true;
  }

  // Callback for the ProcessDiceHeaderDelegateImpl.
  void ShowSigninErrorCallback(content::WebContents* contents,
                               const std::string& error_message,
                               const std::string& email) {
    EXPECT_EQ(web_contents(), contents);
    EXPECT_EQ(auth_error_.ToString(), error_message);
    EXPECT_EQ(email_, email);
    show_error_called_ = true;
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
  TestSigninClient signin_client_;
  FakeProfileOAuth2TokenService token_service_;
  AccountTrackerService account_tracker_service_;
  FakeSigninManager signin_manager_;

  bool enable_sync_called_;
  bool show_error_called_;
  std::string account_id_;
  std::string email_;
  GoogleServiceAuthError auth_error_;
};

// Check that sync is enabled if the tab is closed during signin.
TEST_F(ProcessDiceHeaderDelegateImplTest, CloseTabWhileStartingSync) {
  // Setup the test.
  signin::ScopedAccountConsistencyDice account_consistency;
  GURL kSigninURL = GURL("https://accounts.google.com");
  NavigateAndCommit(kSigninURL);
  ASSERT_EQ(kSigninURL, web_contents()->GetVisibleURL());
  std::unique_ptr<ProcessDiceHeaderDelegateImpl> delegate =
      CreateDelegate(true);

  // Close the tab.
  DeleteContents();

  // Check expectations.
  delegate->EnableSync(account_id_);
  EXPECT_TRUE(enable_sync_called_);
  EXPECT_FALSE(show_error_called_);
}

// Check that the error is still shown if the tab is closed before the error is
// received.
TEST_F(ProcessDiceHeaderDelegateImplTest, CloseTabWhileFailingSignin) {
  // Setup the test.
  signin::ScopedAccountConsistencyDice account_consistency;
  GURL kSigninURL = GURL("https://accounts.google.com");
  NavigateAndCommit(kSigninURL);
  ASSERT_EQ(kSigninURL, web_contents()->GetVisibleURL());
  std::unique_ptr<ProcessDiceHeaderDelegateImpl> delegate =
      CreateDelegate(true);

  // Close the tab.
  DeleteContents();

  // Check expectations.
  delegate->HandleTokenExchangeFailure(email_, auth_error_);
  EXPECT_FALSE(enable_sync_called_);
  EXPECT_TRUE(show_error_called_);
}

struct TestConfiguration {
  // Test setup.
  signin::AccountConsistencyMethod account_consistency;
  bool signed_in;   // User was already signed in at the start of the flow.
  bool signin_tab;  // The tab is marked as a Sync signin tab.

  // Test expectations.
  bool callback_called;  // The relevant callback was called.
  bool show_ntp;         // The NTP was shown.
};

TestConfiguration kEnableSyncTestCases[] = {
    // clang-format off
    // AccountConsistency | signed_in | signin_tab | callback_called | show_ntp
    {kDiceFixAuthErrors,    false,      false,       false,            false},
    {kDiceFixAuthErrors,    false,      true,        false,            false},
    {kDicePrepareMigration, false,      false,       false,            false},
    {kDicePrepareMigration, false,      true,        true,             true},
    {kDice,                 false,      false,       false,            false},
    {kDice,                 false,      true,        true,             true},
    {kDiceFixAuthErrors,    true,       false,       false,            false},
    {kDiceFixAuthErrors,    true,       false,       false,            false},
    {kDicePrepareMigration, true,       false,       false,            false},
    {kDicePrepareMigration, true,       false,       false,            false},
    {kDice,                 true,       false,       false,            false},
    {kDice,                 true,       true,        false,            false},
    // clang-format on
};

// Parameterized version of ProcessDiceHeaderDelegateImplTest.
class ProcessDiceHeaderDelegateImplTestEnableSync
    : public ProcessDiceHeaderDelegateImplTest,
      public ::testing::WithParamInterface<TestConfiguration> {};

// Test the EnableSync() method in all configurations.
TEST_P(ProcessDiceHeaderDelegateImplTestEnableSync, EnableSync) {
  // Setup the test.
  signin::ScopedAccountConsistency account_consistency(
      GetParam().account_consistency);
  GURL kSigninURL = GURL("https://accounts.google.com");
  NavigateAndCommit(kSigninURL);
  ASSERT_EQ(kSigninURL, web_contents()->GetVisibleURL());
  if (GetParam().signed_in)
    signin_manager_.SignIn("gaia_id", "user", "pass");
  std::unique_ptr<ProcessDiceHeaderDelegateImpl> delegate =
      CreateDelegate(GetParam().signin_tab);

  // Check expectations.
  delegate->EnableSync(account_id_);
  EXPECT_EQ(GetParam().callback_called, enable_sync_called_);
  GURL expected_url =
      GetParam().show_ntp ? GURL(chrome::kChromeUINewTabURL) : kSigninURL;
  EXPECT_EQ(expected_url, web_contents()->GetVisibleURL());
  EXPECT_FALSE(show_error_called_);
}

INSTANTIATE_TEST_CASE_P(/* no prefix */,
                        ProcessDiceHeaderDelegateImplTestEnableSync,
                        ::testing::ValuesIn(kEnableSyncTestCases));

TestConfiguration kHandleTokenExchangeFailureTestCases[] = {
    // clang-format off
    // AccountConsistency | signed_in | signin_tab | callback_called | show_ntp
    {kDiceFixAuthErrors,    false,      false,       false,            false},
    {kDiceFixAuthErrors,    false,      true,        false,            false},
    {kDicePrepareMigration, false,      false,       false,            false},
    {kDicePrepareMigration, false,      true,        true,             true},
    {kDice,                 false,      false,       true,             false},
    {kDice,                 false,      true,        true,             true},
    {kDiceFixAuthErrors,    true,       false,       false,            false},
    {kDiceFixAuthErrors,    true,       false,       false,            false},
    {kDicePrepareMigration, true,       false,       false,            false},
    {kDicePrepareMigration, true,       false,       false,            false},
    {kDice,                 true,       false,       true,             false},
    {kDice,                 true,       true,        true,             false},
    // clang-format on
};

// Parameterized version of ProcessDiceHeaderDelegateImplTest.
class ProcessDiceHeaderDelegateImplTestHandleTokenExchangeFailure
    : public ProcessDiceHeaderDelegateImplTest,
      public ::testing::WithParamInterface<TestConfiguration> {};

// Test the HandleTokenExchangeFailure() method in all configurations.
TEST_P(ProcessDiceHeaderDelegateImplTestHandleTokenExchangeFailure,
       HandleTokenExchangeFailure) {
  // Setup the test.
  signin::ScopedAccountConsistency account_consistency(
      GetParam().account_consistency);
  GURL kSigninURL = GURL("https://accounts.google.com");
  NavigateAndCommit(kSigninURL);
  ASSERT_EQ(kSigninURL, web_contents()->GetVisibleURL());
  if (GetParam().signed_in)
    signin_manager_.SignIn("gaia_id", "user", "pass");
  std::unique_ptr<ProcessDiceHeaderDelegateImpl> delegate =
      CreateDelegate(GetParam().signin_tab);

  // Check expectations.
  delegate->HandleTokenExchangeFailure(email_, auth_error_);
  EXPECT_FALSE(enable_sync_called_);
  EXPECT_EQ(GetParam().callback_called, show_error_called_);
  GURL expected_url =
      GetParam().show_ntp ? GURL(chrome::kChromeUINewTabURL) : kSigninURL;
  EXPECT_EQ(expected_url, web_contents()->GetVisibleURL());
}

INSTANTIATE_TEST_CASE_P(
    /* no prefix */,
    ProcessDiceHeaderDelegateImplTestHandleTokenExchangeFailure,
    ::testing::ValuesIn(kHandleTokenExchangeFailureTestCases));

}  // namespace
