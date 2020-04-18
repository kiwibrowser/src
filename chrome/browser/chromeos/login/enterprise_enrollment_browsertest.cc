// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/chromeos/login/enrollment/enrollment_screen.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper_impl.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper_mock.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/js_checker.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/enrollment_status_chromeos.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_auth_policy_client.h"
#include "chromeos/dbus/upstart_client.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"

using testing::_;
using testing::Invoke;
using testing::InvokeWithoutArgs;

namespace chromeos {

namespace {

constexpr char kAdMachineNameInput[] =
    "document.querySelector('#oauth-enroll-ad-join-ui /deep/ "
    "#machineNameInput')";
constexpr char kAdMachineOrgUnitInput[] =
    "document.querySelector('#oauth-enroll-ad-join-ui /deep/ "
    "#orgUnitInput')";
constexpr char kAdEncryptionTypesSelect[] =
    "document.querySelector('#oauth-enroll-ad-join-ui /deep/ "
    "#encryptionList')";
constexpr char kAdUsernameInput[] =
    "document.querySelector('#oauth-enroll-ad-join-ui /deep/ #userInput')";
constexpr char kAdPasswordInput[] =
    "document.querySelector('#oauth-enroll-ad-join-ui /deep/ #passwordInput')";
constexpr char kAdUserDomain[] = "user.domain.com";
constexpr char kAdMachineDomain[] = "machine.domain.com";
constexpr char kAdMachineDomainDN[] =
    "OU=leaf,OU=root,DC=machine,DC=domain,DC=com";
constexpr const char* kAdOrganizationlUnit[] = {"leaf", "root"};
constexpr char kAdTestUser[] = "test_user@user.domain.com";
constexpr char kDMToken[] = "dm_token";

class MockAuthPolicyClient : public FakeAuthPolicyClient {
 public:
  MockAuthPolicyClient() = default;
  ~MockAuthPolicyClient() override = default;
  void JoinAdDomain(const authpolicy::JoinDomainRequest& request,
                    int password_fd,
                    JoinCallback callback) override {
    if (expected_request_) {
      ASSERT_EQ(expected_request_->SerializeAsString(),
                request.SerializeAsString());
      expected_request_.reset();
    }
    FakeAuthPolicyClient::JoinAdDomain(request, password_fd,
                                       std::move(callback));
  }

  void set_expected_request(
      std::unique_ptr<authpolicy::JoinDomainRequest> expected_request) {
    expected_request_ = std::move(expected_request);
  }

 private:
  std::unique_ptr<authpolicy::JoinDomainRequest> expected_request_;
};

}  // namespace

class EnterpriseEnrollmentTest : public LoginManagerTest {
 public:
  EnterpriseEnrollmentTest()
      : LoginManagerTest(true /*should_launch_browser*/) {
    enrollment_setup_functions_.clear();

    EnterpriseEnrollmentHelper::SetupEnrollmentHelperMock(
        [](EnterpriseEnrollmentHelper::EnrollmentStatusConsumer*
               status_consumer,
           const policy::EnrollmentConfig& enrollment_config,
           const std::string& enrolling_user_domain) {

          auto* mock = new EnterpriseEnrollmentHelperMock(status_consumer);
          for (OnSetupEnrollmentHelper fn : enrollment_setup_functions_)
            fn(mock);
          return (EnterpriseEnrollmentHelper*)mock;
        });
  }

  using OnSetupEnrollmentHelper =
      std::function<void(EnterpriseEnrollmentHelperMock*)>;

  // The given function will be executed when the next enrollment helper is
  // created.
  void AddEnrollmentSetupFunction(OnSetupEnrollmentHelper on_setup) {
    enrollment_setup_functions_.push_back(on_setup);
  }

  // Set up expectations for enrollment credentials.
  void ExpectEnrollmentCredentials() {
    AddEnrollmentSetupFunction(
        [](EnterpriseEnrollmentHelperMock* enrollment_helper) {
          EXPECT_CALL(*enrollment_helper,
                      EnrollUsingAuthCode("test_auth_code", _));

          ON_CALL(*enrollment_helper, ClearAuth(_))
              .WillByDefault(Invoke(
                  [](const base::Closure& callback) { callback.Run(); }));
        });
  }

  // Submits regular enrollment credentials.
  void SubmitEnrollmentCredentials() {
    // Trigger an authCompleted event from the authenticator.
    // clang-format off
    js_checker().Evaluate(
      "$('oauth-enrollment').authenticator_.dispatchEvent("
          "new CustomEvent('authCompleted',"
                          "{"
                            "detail: {"
                              "email: 'testuser@test.com',"
                              "authCode: 'test_auth_code'"
                            "}"
                          "}));");
    // clang-format on
  }

  // Submits Active Directory domain join credentials.
  void SubmitActiveDirectoryCredentials(const std::string& machine_name,
                                        const std::string& machine_dn,
                                        const std::string& encryption_types,
                                        const std::string& username,
                                        const std::string& password) {
    EXPECT_TRUE(IsStepDisplayed("ad-join"));
    js_checker().ExpectFalse(std::string(kAdMachineNameInput) + ".hidden");
    js_checker().ExpectFalse(std::string(kAdUsernameInput) + ".hidden");
    js_checker().ExpectFalse(std::string(kAdPasswordInput) + ".hidden");
    const std::string set_machine_name =
        std::string(kAdMachineNameInput) + ".value = '" + machine_name + "'";
    const std::string set_username =
        std::string(kAdUsernameInput) + ".value = '" + username + "'";
    const std::string set_password =
        std::string(kAdPasswordInput) + ".value = '" + password + "'";
    const std::string set_machine_dn =
        std::string(kAdMachineOrgUnitInput) + ".value = '" + machine_dn + "'";
    js_checker().ExecuteAsync(set_machine_name);
    js_checker().ExecuteAsync(set_username);
    js_checker().ExecuteAsync(set_password);
    js_checker().ExecuteAsync(set_machine_dn);
    if (!encryption_types.empty()) {
      const std::string set_encryption_types =
          std::string(kAdEncryptionTypesSelect) + ".value = '" +
          encryption_types + "'";
      js_checker().ExecuteAsync(set_encryption_types);
    }
    js_checker().ExecuteAsync(
        "document.querySelector('#oauth-enroll-ad-join-ui /deep/ "
        "#button').fire('tap')");
    ExecutePendingJavaScript();
  }

  void ClickRetry() {
    js_checker().ExecuteAsync(
        "document.querySelector('"
        "#oauth-enroll-active-directory-join-error-card /deep/ #submitButton'"
        ").fire('tap')");
    ExecutePendingJavaScript();
  }

  void SetExpectedJoinRequest(
      const std::string& machine_name,
      const std::string& machine_domain,
      authpolicy::KerberosEncryptionTypes encryption_types,
      std::vector<std::string> organizational_unit,
      const std::string& username,
      const std::string& dm_token) {
    auto request = std::make_unique<authpolicy::JoinDomainRequest>();
    if (!machine_name.empty())
      request->set_machine_name(machine_name);
    if (!machine_domain.empty())
      request->set_machine_domain(machine_domain);
    for (std::string& it : organizational_unit)
      request->add_machine_ou()->swap(it);
    if (!username.empty())
      request->set_user_principal_name(username);
    if (!dm_token.empty())
      request->set_dm_token(dm_token);
    request->set_kerberos_encryption_types(encryption_types);
    mock_auth_policy_client()->set_expected_request(std::move(request));
  }

  void DisableAttributePromptUpdate() {
    AddEnrollmentSetupFunction(
        [](EnterpriseEnrollmentHelperMock* enrollment_helper) {
          EXPECT_CALL(*enrollment_helper, GetDeviceAttributeUpdatePermission())
              .WillOnce(InvokeWithoutArgs([enrollment_helper]() {
                enrollment_helper->status_consumer()
                    ->OnDeviceAttributeUpdatePermission(false);
              }));
        });
  }

  // Forces an attribute prompt to display.
  void ExpectAttributePromptUpdate() {
    AddEnrollmentSetupFunction(
        [](EnterpriseEnrollmentHelperMock* enrollment_helper) {
          // Causes the attribute-prompt flow to activate.
          ON_CALL(*enrollment_helper, GetDeviceAttributeUpdatePermission())
              .WillByDefault(InvokeWithoutArgs([enrollment_helper]() {
                enrollment_helper->status_consumer()
                    ->OnDeviceAttributeUpdatePermission(true);
              }));

          // Ensures we receive the updates attributes.
          EXPECT_CALL(*enrollment_helper,
                      UpdateDeviceAttributes("asset_id", "location"));
        });
  }

  // Forces the Active Directory domain join flow during enterprise enrollment.
  void SetupActiveDirectoryJoin(const std::string& expected_domain) {
    AddEnrollmentSetupFunction(
        [this,
         expected_domain](EnterpriseEnrollmentHelperMock* enrollment_helper) {
          // Causes the attribute-prompt flow to activate.
          EXPECT_CALL(*enrollment_helper,
                      EnrollUsingAuthCode("test_auth_code", _))
              .WillOnce(InvokeWithoutArgs([this, expected_domain]() {
                this->enrollment_screen()->JoinDomain(
                    kDMToken, base::BindOnce(
                                  [](const std::string& expected_domain,
                                     const std::string& domain) {
                                    ASSERT_EQ(expected_domain, domain);
                                  },
                                  expected_domain));
              }));
        });
  }

  void SetUp() override {
    DBusThreadManager::GetSetterForTesting()->SetAuthPolicyClient(
        std::make_unique<MockAuthPolicyClient>());
    mock_auth_policy_client()->DisableOperationDelayForTesting();
    LoginManagerTest::SetUp();
  }

  MockAuthPolicyClient* mock_auth_policy_client() {
    return static_cast<MockAuthPolicyClient*>(
        DBusThreadManager::Get()->GetAuthPolicyClient());
  }

  void SetupActiveDirectoryJSNotifications() {
    js_checker().ExecuteAsync(
        "var testShowStep = login.OAuthEnrollmentScreen.showStep;\n"
        "login.OAuthEnrollmentScreen.showStep = function(step) {\n"
        "  testShowStep(step);\n"
        "  if (step == 'working') {\n"
        "    window.domAutomationController.send('ShowSpinnerScreen');\n"
        "  }"
        "}\n"
        "var testShowError = login.OAuthEnrollmentScreen.showError;\n"
        "login.OAuthEnrollmentScreen.showError = function(message, retry) {\n"
        "  testShowError(message, retry);\n"
        "  window.domAutomationController.send('ShowADJoinError');\n"
        "}\n");
    js_checker().ExecuteAsync(
        "var testInvalidateAd = login.OAuthEnrollmentScreen.invalidateAd;"
        "login.OAuthEnrollmentScreen.invalidateAd = function(machineName, "
        "user, errorState) {"
        "  testInvalidateAd(machineName, user, errorState);"
        "  window.domAutomationController.send('ShowJoinDomainError');"
        "}");
  }

  void WaitForMessage(content::DOMMessageQueue* message_queue,
                      const std::string& expected_message) {
    std::string message;
    do {
      ASSERT_TRUE(message_queue->WaitForMessage(&message));
    } while (message != expected_message);
  }

  // Fills out the UI with device attribute information and submits it.
  void SubmitAttributePromptUpdate() {
    // Fill out the attribute prompt info and submit it.
    js_checker().ExecuteAsync("$('oauth-enroll-asset-id').value = 'asset_id'");
    js_checker().ExecuteAsync("$('oauth-enroll-location').value = 'location'");
    js_checker().Evaluate(
        "$('oauth-enroll-attribute-prompt-card').fire('submit')");
  }

  // Completes the enrollment process.
  void CompleteEnrollment() {
    enrollment_screen()->OnDeviceEnrolled(std::string());

    // Make sure all other pending JS calls have complete.
    ExecutePendingJavaScript();
  }

  // Makes sure that all pending JS calls have been executed. It is important
  // to make this a separate call from the DOM checks because js_checker uses
  // a different IPC message for JS communication than the login code. This
  // means that the JS script ordering is not preserved between the login code
  // and the test code.
  void ExecutePendingJavaScript() { js_checker().Evaluate(";"); }

  // Returns true if there are any DOM elements with the given class.
  bool IsStepDisplayed(const std::string& step) {
    const std::string js =
        "document.getElementsByClassName('oauth-enroll-state-" + step +
        "').length";
    int count = js_checker().GetInt(js);
    return count > 0;
  }

  // Setup the enrollment screen.
  void ShowEnrollmentScreen() {
    LoginDisplayHost* host = LoginDisplayHost::default_host();
    ASSERT_TRUE(host != nullptr);
    host->StartWizard(OobeScreen::SCREEN_OOBE_ENROLLMENT);
    OobeScreenWaiter(OobeScreen::SCREEN_OOBE_ENROLLMENT).Wait();
    ASSERT_TRUE(enrollment_screen() != nullptr);
    ASSERT_TRUE(WizardController::default_controller() != nullptr);
    ASSERT_FALSE(StartupUtils::IsOobeCompleted());
  }

  // Helper method to return the current EnrollmentScreen instance.
  EnrollmentScreen* enrollment_screen() {
    return EnrollmentScreen::Get(
        WizardController::default_controller()->screen_manager());
  }

 private:
  static std::vector<OnSetupEnrollmentHelper> enrollment_setup_functions_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseEnrollmentTest);
};

std::vector<EnterpriseEnrollmentTest::OnSetupEnrollmentHelper>
    EnterpriseEnrollmentTest::enrollment_setup_functions_;

// Shows the enrollment screen and simulates an enrollment complete event. We
// verify that the enrollmenth helper receives the correct auth code.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestAuthCodeGetsProperlyReceivedFromGaia) {
  ShowEnrollmentScreen();
  ExpectEnrollmentCredentials();
  SubmitEnrollmentCredentials();

  // We need to reset enrollment_screen->enrollment_helper_, otherwise we will
  // get some errors on shutdown.
  enrollment_screen()->enrollment_helper_.reset();
}

// Shows the enrollment screen and simulates an enrollment failure. Verifies
// that the error screen is displayed.
// TODO(crbug.com/690634): Disabled due to timeout flakiness.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       DISABLED_TestProperPageGetsLoadedOnEnrollmentFailure) {
  ShowEnrollmentScreen();

  enrollment_screen()->OnEnrollmentError(policy::EnrollmentStatus::ForStatus(
      policy::EnrollmentStatus::REGISTRATION_FAILED));
  ExecutePendingJavaScript();

  // Verify that the error page is displayed.
  EXPECT_TRUE(IsStepDisplayed("error"));
  EXPECT_FALSE(IsStepDisplayed("success"));
}

// Shows the enrollment screen and simulates a successful enrollment. Verifies
// that the success screen is then displayed.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestProperPageGetsLoadedOnEnrollmentSuccess) {
  ShowEnrollmentScreen();
  DisableAttributePromptUpdate();
  SubmitEnrollmentCredentials();
  CompleteEnrollment();

  // Verify that the success page is displayed.
  EXPECT_TRUE(IsStepDisplayed("success"));
  EXPECT_FALSE(IsStepDisplayed("error"));

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

// Shows the enrollment screen and mocks the enrollment helper to request an
// attribute prompt screen. Verifies the attribute prompt screen is displayed.
// Verifies that the data the user enters into the attribute prompt screen is
// received by the enrollment helper.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestAttributePromptPageGetsLoaded) {
  ShowEnrollmentScreen();
  ExpectAttributePromptUpdate();
  SubmitEnrollmentCredentials();
  CompleteEnrollment();

  // Make sure the attribute-prompt view is open.
  EXPECT_TRUE(IsStepDisplayed("attribute-prompt"));
  EXPECT_FALSE(IsStepDisplayed("success"));
  EXPECT_FALSE(IsStepDisplayed("error"));

  SubmitAttributePromptUpdate();

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

// Shows the enrollment screen and mocks the enrollment helper to show Active
// Directory domain join screen. Verifies the domain join screen is displayed.
// Submits Active Directory credentials. Verifies that the AuthpolicyClient
// calls us back with the correct realm.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestActiveDirectoryEnrollment_Success) {
  ShowEnrollmentScreen();
  DisableAttributePromptUpdate();
  SetupActiveDirectoryJoin(kAdUserDomain);
  SubmitEnrollmentCredentials();

  chromeos::DBusThreadManager::Get()
      ->GetUpstartClient()
      ->StartAuthPolicyService();

  content::DOMMessageQueue message_queue;
  SetupActiveDirectoryJSNotifications();
  authpolicy::KerberosEncryptionTypes enc_types =
      authpolicy::KerberosEncryptionTypes::ENC_TYPES_ALL;
  SetExpectedJoinRequest("machine_name", "" /* machine_domain */, enc_types,
                         {} /* machine_ou */, kAdTestUser, kDMToken);
  SubmitActiveDirectoryCredentials("machine_name", "" /* machine_dn */,
                                   std::to_string(enc_types), kAdTestUser,
                                   "password");
  WaitForMessage(&message_queue, "\"ShowSpinnerScreen\"");
  EXPECT_FALSE(IsStepDisplayed("ad-join"));

  CompleteEnrollment();
  // Verify that the success page is displayed.
  EXPECT_TRUE(IsStepDisplayed("success"));
  EXPECT_FALSE(IsStepDisplayed("error"));

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

// Verifies that the distinguished name specified on the Active Directory join
// domain screen correctly parsed and passed into AuthPolicyClient.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestActiveDirectoryEnrollment_DistinguishedName) {
  ShowEnrollmentScreen();
  DisableAttributePromptUpdate();
  SetupActiveDirectoryJoin(kAdMachineDomain);
  SubmitEnrollmentCredentials();

  chromeos::DBusThreadManager::Get()
      ->GetUpstartClient()
      ->StartAuthPolicyService();

  content::DOMMessageQueue message_queue;
  SetupActiveDirectoryJSNotifications();
  SetExpectedJoinRequest(
      "machine_name", kAdMachineDomain,
      authpolicy::KerberosEncryptionTypes::ENC_TYPES_STRONG,
      std::vector<std::string>(
          kAdOrganizationlUnit,
          kAdOrganizationlUnit + arraysize(kAdOrganizationlUnit)),
      kAdTestUser, kDMToken);
  SubmitActiveDirectoryCredentials("machine_name", kAdMachineDomainDN,
                                   "" /* encryption_types */, kAdTestUser,
                                   "password");
  WaitForMessage(&message_queue, "\"ShowSpinnerScreen\"");
  EXPECT_FALSE(IsStepDisplayed("ad-join"));

  CompleteEnrollment();
  // Verify that the success page is displayed.
  EXPECT_TRUE(IsStepDisplayed("success"));
  EXPECT_FALSE(IsStepDisplayed("error"));

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

// Shows the enrollment screen and mocks the enrollment helper to show Active
// Directory domain join screen. Verifies the domain join screen is displayed.
// Submits Active Directory different incorrect credentials. Verifies that the
// correct error is displayed.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestActiveDirectoryEnrollment_UIErrors) {
  ShowEnrollmentScreen();
  SetupActiveDirectoryJoin(kAdUserDomain);
  SubmitEnrollmentCredentials();

  chromeos::DBusThreadManager::Get()
      ->GetUpstartClient()
      ->StartAuthPolicyService();

  content::DOMMessageQueue message_queue;
  // Checking error in case of empty password. Whether password is not empty
  // being checked in the UI. Machine name length is checked after that in the
  // authpolicyd.
  SetupActiveDirectoryJSNotifications();
  SubmitActiveDirectoryCredentials("too_long_machine_name", "" /* machine_dn */,
                                   "" /* encryption_types */, kAdTestUser,
                                   "" /* password */);
  EXPECT_TRUE(IsStepDisplayed("ad-join"));
  js_checker().ExpectFalse(std::string(kAdMachineNameInput) + ".isInvalid");
  js_checker().ExpectFalse(std::string(kAdUsernameInput) + ".isInvalid");
  js_checker().ExpectTrue(std::string(kAdPasswordInput) + ".isInvalid");

  // Checking error in case of too long machine name.
  SubmitActiveDirectoryCredentials("too_long_machine_name", "" /* machine_dn */,
                                   "" /* encryption_types */, kAdTestUser,
                                   "password");
  WaitForMessage(&message_queue, "\"ShowJoinDomainError\"");
  EXPECT_TRUE(IsStepDisplayed("ad-join"));
  js_checker().ExpectTrue(std::string(kAdMachineNameInput) + ".isInvalid");
  js_checker().ExpectFalse(std::string(kAdUsernameInput) + ".isInvalid");
  js_checker().ExpectFalse(std::string(kAdPasswordInput) + ".isInvalid");

  // Checking error in case of bad username (without realm).
  SubmitActiveDirectoryCredentials("machine_name", "" /* machine_dn */,
                                   "" /* encryption_types */, "test_user",
                                   "password");
  WaitForMessage(&message_queue, "\"ShowJoinDomainError\"");
  EXPECT_TRUE(IsStepDisplayed("ad-join"));
  js_checker().ExpectFalse(std::string(kAdMachineNameInput) + ".isInvalid");
  js_checker().ExpectTrue(std::string(kAdUsernameInput) + ".isInvalid");
  js_checker().ExpectFalse(std::string(kAdPasswordInput) + ".isInvalid");

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

// Check that correct error card is shown (Active Directory one). Also checks
// that hitting retry shows Active Directory screen again.
IN_PROC_BROWSER_TEST_F(EnterpriseEnrollmentTest,
                       TestActiveDirectoryEnrollment_ErrorCard) {
  ShowEnrollmentScreen();
  SetupActiveDirectoryJoin(kAdUserDomain);
  SubmitEnrollmentCredentials();

  chromeos::DBusThreadManager::Get()
      ->GetUpstartClient()
      ->StartAuthPolicyService();

  content::DOMMessageQueue message_queue;
  SetupActiveDirectoryJSNotifications();
  // Legacy type triggers error card.
  authpolicy::KerberosEncryptionTypes enc_types =
      authpolicy::KerberosEncryptionTypes::ENC_TYPES_LEGACY;
  SubmitActiveDirectoryCredentials("machine_name", "" /* machine_dn */,
                                   std::to_string(enc_types), "test_user",
                                   "password");
  WaitForMessage(&message_queue, "\"ShowADJoinError\"");
  EXPECT_TRUE(IsStepDisplayed("active-directory-join-error"));
  ClickRetry();
  EXPECT_TRUE(IsStepDisplayed("ad-join"));

  // We have to remove the enrollment_helper before the dtor gets called.
  enrollment_screen()->enrollment_helper_.reset();
}

}  // namespace chromeos
