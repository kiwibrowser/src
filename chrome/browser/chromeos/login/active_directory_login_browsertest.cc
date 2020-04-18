// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/js_checker.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_webui.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/settings/stub_install_attributes.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_auth_policy_client.h"
#include "chromeos/dbus/fake_cryptohome_client.h"
#include "chromeos/login/auth/authpolicy_login_helper.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_names.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/test/rect_test_util.h"

using ::gfx::test::RectContains;

namespace chromeos {
namespace {

const char kPassword[] = "password";

constexpr char kAdOfflineAuthId[] = "offline-ad-auth";

constexpr char kAdMachineName[] = "machine_name";
constexpr char kTestActiveDirectoryUser[] = "test-user";
constexpr char kTestUserRealm[] = "user.realm";
constexpr char kAdMachineInput[] = "machineNameInput";
constexpr char kAdMoreOptionsButton[] = "moreOptionsBtn";
constexpr char kAdUserInput[] = "userInput";
constexpr char kAdPasswordInput[] = "passwordInput";
constexpr char kAdButton[] = "button";
constexpr char kAdWelcomMessage[] = "welcomeMsg";
constexpr char kAdAutocompleteRealm[] = "userInput /deep/ #domainLabel";

constexpr char kAdPasswordChangeId[] = "ad-password-change";
constexpr char kAdAnimatedPages[] = "animatedPages";
constexpr char kAdOldPasswordInput[] = "oldPassword";
constexpr char kAdNewPassword1Input[] = "newPassword1";
constexpr char kAdNewPassword2Input[] = "newPassword2";
constexpr char kNewPassword[] = "new_password";
constexpr char kDifferentNewPassword[] = "different_new_password";
constexpr char kDMToken[] = "dm_token";

constexpr char kCloseButtonId[] = "closeButton";

class TestAuthPolicyClient : public FakeAuthPolicyClient {
 public:
  TestAuthPolicyClient() { FakeAuthPolicyClient::set_started(true); }

  void AuthenticateUser(const authpolicy::AuthenticateUserRequest& request,
                        int password_fd,
                        AuthCallback callback) override {
    authpolicy::ActiveDirectoryAccountInfo account_info;
    if (auth_error_ == authpolicy::ERROR_NONE) {
      if (request.account_id().empty()) {
        account_info.set_account_id(
            base::MD5String(request.user_principal_name()));
      } else {
        account_info.set_account_id(request.account_id());
      }
    }
    base::SequencedTaskRunnerHandle::Get()->PostNonNestableTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), auth_error_, account_info));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAuthPolicyClient);
};

class ActiveDirectoryLoginTest : public LoginManagerTest {
 public:
  ActiveDirectoryLoginTest()
      : LoginManagerTest(true),
        // Using the same realm as supervised user domain. Should be treated as
        // normal realm.
        test_realm_(user_manager::kSupervisedUserDomain),
        autocomplete_realm_(test_realm_) {}

  ~ActiveDirectoryLoginTest() override = default;

  void SetUp() override {
    SetupTestAuthPolicyClient();
    LoginManagerTest::SetUp();
  }

  void SetUpInProcessBrowserTestFixture() override {
    LoginManagerTest::SetUpInProcessBrowserTestFixture();
    base::FilePath user_data_dir;
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
    chromeos::RegisterStubPathOverrides(user_data_dir);
    DBusThreadManager::GetSetterForTesting()->SetCryptohomeClient(
        std::make_unique<FakeCryptohomeClient>());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kOobeSkipPostLogin);
    LoginManagerTest::SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    // Set the threshold to a max value to disable the offline message screen
    // on slow configurations like MSAN, where it otherwise triggers on every
    // run.
    LoginDisplayHost::default_host()
        ->GetOobeUI()
        ->signin_screen_handler()
        ->SetOfflineTimeoutForTesting(base::TimeDelta::Max());
    fake_auth_policy_client()->DisableOperationDelayForTesting();
    LoginManagerTest::SetUpOnMainThread();
  }

  void MarkAsActiveDirectoryEnterprise() {
    StartupUtils::MarkOobeCompleted();
    AuthPolicyLoginHelper helper;
    {
      base::RunLoop loop;
      helper.set_dm_token(kDMToken);
      helper.JoinAdDomain(
          kAdMachineName, "" /* distinguished_name */,
          authpolicy::KerberosEncryptionTypes::ENC_TYPES_STRONG,
          kTestActiveDirectoryUser + ("@" + test_realm_), "" /* password */,
          base::BindOnce(
              [](base::OnceClosure closure, const std::string& expected_domain,
                 authpolicy::ErrorType error, const std::string& domain) {
                EXPECT_EQ(authpolicy::ERROR_NONE, error);
                EXPECT_EQ(expected_domain, domain);
                std::move(closure).Run();
              },
              loop.QuitClosure(), test_realm_));
      loop.Run();
    }
    ASSERT_TRUE(AuthPolicyLoginHelper::LockDeviceActiveDirectoryForTesting(
        test_realm_));
    {
      base::RunLoop loop;
      fake_auth_policy_client()->RefreshDevicePolicy(base::BindOnce(
          [](base::OnceClosure closure, authpolicy::ErrorType error) {
            EXPECT_EQ(authpolicy::ERROR_NONE, error);
            std::move(closure).Run();
          },
          loop.QuitClosure()));
      loop.Run();
    }
  }

  void TriggerPasswordChangeScreen() {
    OobeScreenWaiter screen_waiter(
        OobeScreen::SCREEN_ACTIVE_DIRECTORY_PASSWORD_CHANGE);

    fake_auth_policy_client()->set_auth_error(
        authpolicy::ERROR_PASSWORD_EXPIRED);
    SubmitActiveDirectoryCredentials(kTestActiveDirectoryUser, kPassword);
    screen_waiter.Wait();
    TestAdPasswordChangeError(std::string());
  }

  void ClosePasswordChangeScreen() {
    js_checker().Evaluate(JSElement(kAdPasswordChangeId, kCloseButtonId) +
                          ".fire('tap')");
  }

  void SetupTestAuthPolicyClient() {
    auto test_client = std::make_unique<TestAuthPolicyClient>();
    fake_auth_policy_client_ = test_client.get();
    DBusThreadManager::GetSetterForTesting()->SetAuthPolicyClient(
        std::move(test_client));
  }

  // Checks if Active Directory login is visible.
  void TestLoginVisible() {
    OobeScreenWaiter screen_waiter(OobeScreen::SCREEN_GAIA_SIGNIN);
    screen_waiter.Wait();
    // Checks if Gaia signin is hidden.
    JSExpect("document.querySelector('#signin-frame').hidden");

    // Checks if Active Directory signin is visible.
    JSExpect("!document.querySelector('#offline-ad-auth').hidden");
    JSExpect(JSElement(kAdOfflineAuthId, kAdMachineInput) + ".hidden");
    JSExpect(JSElement(kAdOfflineAuthId, kAdMoreOptionsButton) + ".hidden");
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdUserInput) + ".hidden");
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdPasswordInput) + ".hidden");

    const std::string innerText(".innerText");
    // Checks if Active Directory welcome message contains realm.
    EXPECT_EQ(l10n_util::GetStringFUTF8(IDS_AD_DOMAIN_AUTH_WELCOME_MESSAGE,
                                        base::UTF8ToUTF16(test_realm_)),
              js_checker().GetString(
                  JSElement(kAdOfflineAuthId, kAdWelcomMessage) + innerText));

    // Checks if realm is set to autocomplete username.
    EXPECT_EQ(
        "@" + autocomplete_realm_,
        js_checker().GetString(
            JSElement(kAdOfflineAuthId, kAdAutocompleteRealm) + innerText));

    // Checks if bottom bar is visible.
    JSExpect("!Oobe.getInstance().headerHidden");
  }

  // Checks if Active Directory password change screen is shown.
  void TestPasswordChangeVisible() {
    // Checks if Gaia signin is hidden.
    JSExpect("document.querySelector('#signin-frame').hidden");
    // Checks if Active Directory signin is visible.
    JSExpect("!document.querySelector('#ad-password-change').hidden");
    JSExpect(JSElement(kAdPasswordChangeId, kAdAnimatedPages) +
             ".selected == 0");
    JSExpect("!" + JSElement(kAdPasswordChangeId, kCloseButtonId) + ".hidden");
  }

  // Checks if user input is marked as invalid.
  void TestUserError() {
    TestLoginVisible();
    JSExpect(JSElement(kAdOfflineAuthId, kAdUserInput) + ".isInvalid");
  }

  // Checks if password input is marked as invalid.
  void TestPasswordError() {
    TestLoginVisible();
    JSExpect(JSElement(kAdOfflineAuthId, kAdPasswordInput) + ".isInvalid");
  }

  // Checks that machine, password and user inputs are valid.
  void TestNoError() {
    TestLoginVisible();
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdMachineInput) + ".isInvalid");
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdUserInput) + ".isInvalid");
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdPasswordInput) +
             ".isInvalid");
  }

  // Checks if autocomplete domain is visible for the user input.
  void TestDomainVisible() {
    JSExpect("!" + JSElement(kAdOfflineAuthId, kAdAutocompleteRealm) +
             ".hidden");
  }

  // Checks if autocomplete domain is hidden for the user input.
  void TestDomainHidden() {
    JSExpect(JSElement(kAdOfflineAuthId, kAdAutocompleteRealm) + ".hidden");
  }

  // Checks if Active Directory password change screen is shown. Also checks if
  // |invalid_element| is invalidated and all the other elements are valid.
  void TestAdPasswordChangeError(const std::string& invalid_element) {
    TestPasswordChangeVisible();
    for (const char* element :
         {kAdOldPasswordInput, kAdNewPassword1Input, kAdNewPassword2Input}) {
      std::string js_assertion =
          JSElement(kAdPasswordChangeId, element) + ".isInvalid";
      if (element != invalid_element)
        js_assertion = "!" + js_assertion;
      JSExpect(js_assertion);
    }
  }

  // Sets username and password for the Active Directory login and submits it.
  void SubmitActiveDirectoryCredentials(const std::string& username,
                                        const std::string& password) {
    js_checker().ExecuteAsync(JSElement(kAdOfflineAuthId, kAdUserInput) +
                              ".value='" + username + "'");
    js_checker().ExecuteAsync(JSElement(kAdOfflineAuthId, kAdPasswordInput) +
                              ".value='" + password + "'");
    js_checker().Evaluate(JSElement(kAdOfflineAuthId, kAdButton) +
                          ".fire('tap')");
  }

  // Sets username and password for the Active Directory login and submits it.
  void SubmitActiveDirectoryPasswordChangeCredentials(
      const std::string& old_password,
      const std::string& new_password1,
      const std::string& new_password2) {
    js_checker().ExecuteAsync(
        JSElement(kAdPasswordChangeId, kAdOldPasswordInput) + ".value='" +
        old_password + "'");
    js_checker().ExecuteAsync(
        JSElement(kAdPasswordChangeId, kAdNewPassword1Input) + ".value='" +
        new_password1 + "'");
    js_checker().ExecuteAsync(
        JSElement(kAdPasswordChangeId, kAdNewPassword2Input) + ".value='" +
        new_password2 + "'");
    js_checker().Evaluate(JSElement(kAdPasswordChangeId, kAdButton) +
                          ".fire('tap')");
  }

  void SetupActiveDirectoryJSNotifications() {
    js_checker().Evaluate(
        "var testInvalidateAd = login.GaiaSigninScreen.invalidateAd;"
        "login.GaiaSigninScreen.invalidateAd = function(user, errorState) {"
        "  testInvalidateAd(user, errorState);"
        "  window.domAutomationController.send('ShowAuthError');"
        "}");
  }

  void WaitForMessage(content::DOMMessageQueue* message_queue,
                      const std::string& expected_message) {
    std::string message;
    do {
      ASSERT_TRUE(message_queue->WaitForMessage(&message));
    } while (message != expected_message);
  }

 protected:
  // Returns string representing element with id=|element_id| inside Active
  // Directory login element.
  std::string JSElement(const std::string& parent_id,
                        const std::string& element_id) {
    return "document.querySelector('#" + parent_id + " /deep/ #" + element_id +
           "')";
  }
  TestAuthPolicyClient* fake_auth_policy_client() {
    return fake_auth_policy_client_;
  }

  const std::string test_realm_;
  std::string autocomplete_realm_;

 private:
  TestAuthPolicyClient* fake_auth_policy_client_;

  DISALLOW_COPY_AND_ASSIGN(ActiveDirectoryLoginTest);
};

class ActiveDirectoryLoginAutocompleteTest : public ActiveDirectoryLoginTest {
 public:
  ActiveDirectoryLoginAutocompleteTest() = default;
  void SetUpInProcessBrowserTestFixture() override {
    enterprise_management::ChromeDeviceSettingsProto device_settings;
    device_settings.mutable_login_screen_domain_auto_complete()
        ->set_login_screen_domain_auto_complete(kTestUserRealm);
    fake_auth_policy_client()->set_device_policy(device_settings);
    autocomplete_realm_ = kTestUserRealm;

    ActiveDirectoryLoginTest::SetUpInProcessBrowserTestFixture();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ActiveDirectoryLoginAutocompleteTest);
};

}  // namespace

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, PRE_LoginSuccess) {
  MarkAsActiveDirectoryEnterprise();
}

// Test successful Active Directory login.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, LoginSuccess) {
  TestNoError();
  TestDomainVisible();

  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());
  SubmitActiveDirectoryCredentials(kTestActiveDirectoryUser, kPassword);
  session_start_waiter.Wait();
}

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, PRE_LoginErrors) {
  MarkAsActiveDirectoryEnterprise();
}

// Test different UI errors for Active Directory login.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, LoginErrors) {
  SetupActiveDirectoryJSNotifications();
  TestNoError();
  TestDomainVisible();

  content::DOMMessageQueue message_queue;

  SubmitActiveDirectoryCredentials("", "");
  TestUserError();
  TestDomainVisible();

  SubmitActiveDirectoryCredentials(kTestActiveDirectoryUser, "");
  TestPasswordError();
  TestDomainVisible();

  SubmitActiveDirectoryCredentials(std::string(kTestActiveDirectoryUser) + "@",
                                   kPassword);
  TestUserError();
  TestDomainHidden();

  fake_auth_policy_client()->set_auth_error(authpolicy::ERROR_BAD_USER_NAME);
  SubmitActiveDirectoryCredentials(
      std::string(kTestActiveDirectoryUser) + "@" + test_realm_, kPassword);
  WaitForMessage(&message_queue, "\"ShowAuthError\"");
  TestUserError();
  TestDomainVisible();

  fake_auth_policy_client()->set_auth_error(authpolicy::ERROR_BAD_PASSWORD);
  SubmitActiveDirectoryCredentials(kTestActiveDirectoryUser, kPassword);
  WaitForMessage(&message_queue, "\"ShowAuthError\"");
  TestPasswordError();
  TestDomainVisible();

  fake_auth_policy_client()->set_auth_error(authpolicy::ERROR_UNKNOWN);
  SubmitActiveDirectoryCredentials(kTestActiveDirectoryUser, kPassword);
  WaitForMessage(&message_queue, "\"ShowAuthError\"");
  // Inputs are not invalidated for the unknown error.
  TestNoError();
  TestDomainVisible();
}

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest,
                       PRE_PasswordChange_LoginSuccess) {
  MarkAsActiveDirectoryEnterprise();
}

// Test successful Active Directory login from the password change screen.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, PasswordChange_LoginSuccess) {
  TestLoginVisible();
  TestDomainVisible();

  TriggerPasswordChangeScreen();

  // Password accepted by AuthPolicyClient.
  fake_auth_policy_client()->set_auth_error(authpolicy::ERROR_NONE);
  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());
  SubmitActiveDirectoryPasswordChangeCredentials(kPassword, kNewPassword,
                                                 kNewPassword);
  session_start_waiter.Wait();
}

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, PRE_PasswordChange_UIErrors) {
  MarkAsActiveDirectoryEnterprise();
}

// Test different UI errors for Active Directory password change screen.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest, PasswordChange_UIErrors) {
  TestLoginVisible();
  TestDomainVisible();

  TriggerPasswordChangeScreen();
  // Password rejected by UX.
  // Empty passwords.
  SubmitActiveDirectoryPasswordChangeCredentials("", "", "");
  TestAdPasswordChangeError(kAdOldPasswordInput);

  // Empty new password.
  SubmitActiveDirectoryPasswordChangeCredentials(kPassword, "", "");
  TestAdPasswordChangeError(kAdNewPassword1Input);

  // Empty confirmation of the new password.
  SubmitActiveDirectoryPasswordChangeCredentials(kPassword, kNewPassword, "");
  TestAdPasswordChangeError(kAdNewPassword2Input);

  // Confirmation of password is different from new password.
  SubmitActiveDirectoryPasswordChangeCredentials(kPassword, kNewPassword,
                                                 kDifferentNewPassword);
  TestAdPasswordChangeError(kAdNewPassword2Input);

  // Password rejected by AuthPolicyClient.
  fake_auth_policy_client()->set_auth_error(authpolicy::ERROR_BAD_PASSWORD);
  SubmitActiveDirectoryPasswordChangeCredentials(kPassword, kNewPassword,
                                                 kNewPassword);
  TestAdPasswordChangeError(kAdOldPasswordInput);
}

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest,
                       PRE_PasswordChange_ReopenClearErrors) {
  MarkAsActiveDirectoryEnterprise();
}

// Test reopening Active Directory password change screen clears errors.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginTest,
                       PasswordChange_ReopenClearErrors) {
  TestLoginVisible();
  TestDomainVisible();

  TriggerPasswordChangeScreen();

  // Empty new password.
  SubmitActiveDirectoryPasswordChangeCredentials("", "", "");
  TestAdPasswordChangeError(kAdOldPasswordInput);

  ClosePasswordChangeScreen();
  TestLoginVisible();
  TriggerPasswordChangeScreen();
}

// Marks as Active Directory enterprise device and OOBE as completed.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginAutocompleteTest,
                       PRE_TestAutocomplete) {
  MarkAsActiveDirectoryEnterprise();
}

// Tests that DeviceLoginScreenDomainAutoComplete policy overrides device realm
// for user autocomplete.
IN_PROC_BROWSER_TEST_F(ActiveDirectoryLoginAutocompleteTest, TestAutocomplete) {
  TestLoginVisible();
  TestDomainVisible();
}

}  // namespace chromeos
