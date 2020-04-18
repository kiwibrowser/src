// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/policy/login_policy_test_base.h"
#include "chrome/browser/chromeos/policy/user_policy_test_helper.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/arc/arc_features.h"
#include "components/arc/arc_prefs.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// The Gaia ID supplied by FakeGaia for our mocked-out signin.
const char kTestGaiaId[] = "12345";

const char kIdTokenChildAccount[] =
    "dummy-header."
    // base64 encoded: { "services": ["uca"] }
    "eyAic2VydmljZXMiOiBbInVjYSJdIH0="
    ".dummy-signature";

// Services list for the child user. (This must be a correct JSON array.)
const char kChildServices[] = "[\"uca\"]";

// Helper class that counts the number of notifications of the specified
// type that have been received.
class CountNotificationObserver : public content::NotificationObserver {
 public:
  CountNotificationObserver(int notification_type_to_count,
                            const content::NotificationSource& source)
      : notification_count_(0) {
    registrar_.Add(this, notification_type_to_count, source);
  }

  // NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    // Count the number of notifications seen.
    notification_count_++;
  }

  int notification_count() const { return notification_count_; }

 private:
  int notification_count_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(CountNotificationObserver);
};

}  // namespace

namespace policy {

class UserCloudPolicyManagerTest : public LoginPolicyTestBase {
 protected:
  UserCloudPolicyManagerTest() {}

  void GetMandatoryPoliciesValue(base::DictionaryValue* policy) const override {
    std::unique_ptr<base::ListValue> list(new base::ListValue);
    list->AppendString("chrome://policy");
    list->AppendString("chrome://about");

    policy->Set(key::kRestoreOnStartupURLs, std::move(list));
    policy->SetInteger(key::kRestoreOnStartup,
                       SessionStartupPref::kPrefValueURLs);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UserCloudPolicyManagerTest);
};

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerTest, StartSession) {
  const char* const kStartupURLs[] = {"chrome://policy", "chrome://about"};
  // User hasn't signed in yet, so shouldn't know if the user requires policy.
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(kAccountId, kTestGaiaId);
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  SkipToLoginScreen();
  LogIn(kAccountId, kAccountPassword, kEmptyServices);

  // User should be marked as having a valid OAuth token.
  const user_manager::UserManager* const user_manager =
      user_manager::UserManager::Get();
  EXPECT_EQ(user_manager::User::OAUTH2_TOKEN_STATUS_VALID,
            user_manager->GetActiveUser()->oauth_token_status());

  // Check that the startup pages specified in policy were opened.
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  Browser* browser = browser_list->get(0);
  ASSERT_TRUE(browser);

  TabStripModel* tabs = browser->tab_strip_model();
  ASSERT_TRUE(tabs);
  const int expected_tab_count = static_cast<int>(arraysize(kStartupURLs));
  EXPECT_EQ(expected_tab_count, tabs->count());
  for (int i = 0; i < expected_tab_count && i < tabs->count(); ++i) {
    EXPECT_EQ(GURL(kStartupURLs[i]),
              tabs->GetWebContentsAt(i)->GetVisibleURL());
  }
  EXPECT_TRUE(user_manager->GetActiveUser()->profile_ever_initialized());

  // User should be marked as requiring policy.
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kPolicyRequired,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  // It is expected that if ArcEnabled policy is not set then it is managed
  // by default and user is not able manually set it.
  EXPECT_TRUE(
      ProfileManager::GetActiveUserProfile()->GetPrefs()->IsManagedPreference(
          arc::prefs::kArcEnabled));
}

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerTest, ErrorLoadingPolicy) {
  // Delete the policy file - this will cause a 500 error on policy requests.
  user_policy_helper()->DeletePolicyFile();
  SkipToLoginScreen();
  CountNotificationObserver observer(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());
  GetLoginDisplay()->ShowSigninScreenForTest(kAccountId, kAccountPassword,
                                             kEmptyServices);
  base::RunLoop().Run();
  // Should not receive a SESSION_STARTED notification.
  ASSERT_EQ(0, observer.notification_count());

  // User should not be marked as having completed profile initialization.
  const user_manager::UserManager* user_manager =
      user_manager::UserManager::Get();
  EXPECT_FALSE(user_manager->GetActiveUser()->profile_ever_initialized());
  // User should be marked as not knowing if policy is required yet.
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(kAccountId, kTestGaiaId);
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));
}

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerTest,
                       ErrorLoadingPolicyForUnmanagedUser) {
  // Mark user as not needing policy - errors loading policy should be
  // ignored (unlike previous ErrorLoadingPolicy test).
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(kAccountId, kTestGaiaId);
  user_manager::known_user::SetProfileRequiresPolicy(
      account_id,
      user_manager::known_user::ProfileRequiresPolicy::kNoPolicyRequired);

  // Delete the policy file - this will cause a 500 error on policy requests.
  user_policy_helper()->DeletePolicyFile();
  SkipToLoginScreen();
  LogIn(kAccountId, kAccountPassword, kEmptyServices);

  // User should be marked as having a valid OAuth token.
  const user_manager::UserManager* const user_manager =
      user_manager::UserManager::Get();
  EXPECT_EQ(user_manager::User::OAUTH2_TOKEN_STATUS_VALID,
            user_manager->GetActiveUser()->oauth_token_status());

  // User should be marked as having completed profile initialization.
  EXPECT_TRUE(user_manager->GetActiveUser()->profile_ever_initialized());

  // User should still be marked as not needing policy
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kNoPolicyRequired,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));
}

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerTest, MigrateForExistingUser) {
  // Mark user as already having initialized the profile - this should allow
  // the policy code to ignore errors (because we presume we already have a
  // valid cache). We can remove this test when we fix crbug.com/731726 and
  // remove the associated migration code from UserPolicyManagerFactoryChromeOS.
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(kAccountId, kTestGaiaId);
  chromeos::ChromeUserManager* user_manager =
      chromeos::ChromeUserManager::Get();
  user_manager::User* user =
      user_manager::User::CreateRegularUserForTesting(account_id);
  user->set_profile_ever_initialized(true);
  user_manager->AddUserRecordForTesting(user);

  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  user_manager::known_user::SetProfileEverInitialized(account_id, true);
  EXPECT_TRUE(user_manager::known_user::WasProfileEverInitialized(account_id));

  // Delete the policy file - this will cause a 500 error on policy requests.
  user_policy_helper()->DeletePolicyFile();
  SkipToLoginScreen();
  LogIn(kAccountId, kAccountPassword, kEmptyServices);

  // User should be marked as having a valid OAuth token.
  EXPECT_EQ(user_manager::User::OAUTH2_TOKEN_STATUS_VALID,
            user_manager->GetActiveUser()->oauth_token_status());

  // User should still be marked as having completed profile initialization.
  EXPECT_TRUE(user_manager->GetActiveUser()->profile_ever_initialized());

  // Policy status should still be unknown since we haven't managed to load
  // policy from disk nor have we been able to talk to the server.
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));
}

class UserCloudPolicyManagerNonEnterpriseTest
    : public UserCloudPolicyManagerTest {
 protected:
  UserCloudPolicyManagerNonEnterpriseTest() = default;
  ~UserCloudPolicyManagerNonEnterpriseTest() override = default;

  // UserCloudPolicyManagerTest:
  void SetUp() override {
    // Recognize example.com as non-enterprise account. We don't use any
    // available public domain such as gmail.com in order to prevent possible
    // leak of verification keys/signatures.
    policy::BrowserPolicyConnector::SetNonEnterpriseDomainForTesting(
        "example.com");
    UserCloudPolicyManagerTest::SetUp();
  }

  void TearDown() override {
    UserCloudPolicyManagerTest::TearDown();
    policy::BrowserPolicyConnector::SetNonEnterpriseDomainForTesting(nullptr);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UserCloudPolicyManagerNonEnterpriseTest);
};

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerNonEnterpriseTest,
                       NoPolicyForConsumer) {
  EXPECT_TRUE(
      policy::BrowserPolicyConnector::IsNonEnterpriseUser(GetAccount()));
  // If a user signs in with a known non-enterprise account there should be no
  // policy.
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(GetAccount(), kTestGaiaId);
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  SkipToLoginScreen();
  LogIn(GetAccount(), kAccountPassword, kEmptyServices);

  // User should be marked as having a valid OAuth token.
  const user_manager::UserManager* const user_manager =
      user_manager::UserManager::Get();
  EXPECT_EQ(user_manager::User::OAUTH2_TOKEN_STATUS_VALID,
            user_manager->GetActiveUser()->oauth_token_status());

  EXPECT_TRUE(user_manager->GetActiveUser()->profile_ever_initialized());

  // User should be marked as not requiring policy.
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kNoPolicyRequired,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));
}

class UserCloudPolicyManagerChildTest
    : public UserCloudPolicyManagerNonEnterpriseTest {
 protected:
  UserCloudPolicyManagerChildTest() = default;
  ~UserCloudPolicyManagerChildTest() override = default;

  // LoginPolicyTestBase:
  std::string GetIdToken() const override { return kIdTokenChildAccount; }

  // UserCloudPolicyManagerNonEnterpriseTest:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        arc::kAvailableForChildAccountFeature);
    UserCloudPolicyManagerNonEnterpriseTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(UserCloudPolicyManagerChildTest);
};

IN_PROC_BROWSER_TEST_F(UserCloudPolicyManagerChildTest, PolicyForChildUser) {
  EXPECT_TRUE(
      policy::BrowserPolicyConnector::IsNonEnterpriseUser(GetAccount()));
  // If a user signs in with a known non-enterprise account there should be no
  // policy in case user type is child.
  AccountId account_id =
      AccountId::FromUserEmailGaiaId(GetAccount(), kTestGaiaId);
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kUnknown,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  SkipToLoginScreen();
  LogIn(GetAccount(), kAccountPassword, kChildServices);

  // User should be marked as having a valid OAuth token.
  const user_manager::UserManager* const user_manager =
      user_manager::UserManager::Get();
  EXPECT_EQ(user_manager::User::OAUTH2_TOKEN_STATUS_VALID,
            user_manager->GetActiveUser()->oauth_token_status());

  EXPECT_TRUE(user_manager->GetActiveUser()->profile_ever_initialized());

  // User of CHILD type should be marked as requiring policy.
  EXPECT_EQ(user_manager::known_user::ProfileRequiresPolicy::kPolicyRequired,
            user_manager::known_user::GetProfileRequiresPolicy(account_id));

  // It is expected that if ArcEnabled policy is not set then it is not managed
  // by default and user is able manually set it.
  EXPECT_FALSE(
      ProfileManager::GetActiveUserProfile()->GetPrefs()->IsManagedPreference(
          arc::prefs::kArcEnabled));
}

}  // namespace policy
