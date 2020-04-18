// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/command_line.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/chromeos/login/session/user_session_manager_test_api.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater_factory.h"
#include "chrome/browser/policy/test/local_policy_test_server.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/chromeos_test_utils.h"
#include "chromeos/network/onc/onc_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_switches.h"
#include "components/policy/policy_constants.h"
#include "components/session_manager/core/session_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/cert_verifier.h"
#include "net/test/cert_test_util.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace em = enterprise_management;

namespace {

// Test data file storing an ONC blob with an Authority certificate.
constexpr char kRootCertOnc[] = "root-ca-cert.onc";
constexpr char kNetworkComponentDirectory[] = "network";
// A PEM-encoded certificate which was signed by the Authority specified in
// |kRootCertOnc|.
constexpr char kGoodCert[] = "ok_cert.pem";
constexpr char kDeviceLocalAccountId[] = "dla1@example.com";

// Allows waiting until the list of policy-pushed web-trusted certificates
// changes.
class WebTrustedCertsChangedObserver
    : public policy::UserNetworkConfigurationUpdater::WebTrustedCertsObserver {
 public:
  WebTrustedCertsChangedObserver() {}

  // UserNetworkConfigurationUpdater:
  void OnTrustAnchorsChanged(
      const net::CertificateList& trust_anchors) override {
    run_loop.QuitClosure().Run();
  }

  void Wait() { run_loop.Run(); }

 private:
  base::RunLoop run_loop;

  DISALLOW_COPY_AND_ASSIGN(WebTrustedCertsChangedObserver);
};

// Called on the IO thread to verify the |test_server_cert| using the
// CertVerifier from |request_context_getter|. The result will be written into
// |verification_result|.
void VerifyTestServerCertOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    scoped_refptr<net::X509Certificate> test_server_cert,
    int* verification_result) {
  net::CertVerifier* cert_verifier =
      request_context_getter->GetURLRequestContext()->cert_verifier();

  net::TestCompletionCallback test_callback;
  net::CertVerifyResult verify_result;
  std::unique_ptr<net::CertVerifier::Request> request;
  // CertVerifier will offload work to a worker pool and post a task back to IO
  // thread. We need to wait for that to happen. TestCompletionCallback performs
  // a RunLoop when waiting for the notification to allow tasks to run. As this
  // is effectively a _nested_ RunLoop, we need ScopedNestableTaskAllower to
  // allow it.
  base::MessageLoopCurrent::ScopedNestableTaskAllower allow_nested;
  *verification_result = test_callback.GetResult(cert_verifier->Verify(
      net::CertVerifier::RequestParams(test_server_cert.get(), "127.0.0.1", 0,
                                       std::string(), net::CertificateList()),
      nullptr, &verify_result, test_callback.callback(), &request,
      net::NetLogWithSource()));
}

bool IsSessionStarted() {
  return session_manager::SessionManager::Get()->IsSessionStarted();
}

}  // namespace

namespace policy {

// Base class for testing if policy-provided trust roots take effect.
class PolicyProvidedTrustRootsTestBase : public DevicePolicyCrosBrowserTest {
 protected:
  PolicyProvidedTrustRootsTestBase() {}

  // InProcessBrowserTest:
  ~PolicyProvidedTrustRootsTestBase() override {}

  void SetUpInProcessBrowserTestFixture() override {
    // Load the certificate which is only OK if the policy-provided authority is
    // actually trusted.
    base::FilePath cert_pem_file_path;
    chromeos::test_utils::GetTestDataPath(kNetworkComponentDirectory, kGoodCert,
                                          &cert_pem_file_path);
    test_server_cert_ = net::ImportCertFromFile(
        cert_pem_file_path.DirName(), cert_pem_file_path.BaseName().value());

    // Set up the mock policy provider.
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    DevicePolicyCrosBrowserTest::SetUpCommandLine(command_line);
  }

  // Sets the ONC-policy to the blob defined by |kRootCertOnc| and waits until
  // the notification that policy-provided trust roots have changed is sent from
  // |profile|'s UserNetworkConfigurationUpdater.
  void SetRootCertONCPolicy(Profile* profile) {
    policy::UserNetworkConfigurationUpdater*
        user_network_configuration_updater =
            policy::UserNetworkConfigurationUpdaterFactory::GetForProfile(
                profile);
    WebTrustedCertsChangedObserver trust_roots_changed_observer;
    user_network_configuration_updater->AddTrustedCertsObserver(
        &trust_roots_changed_observer);

    const std::string& user_policy_blob =
        chromeos::onc::test_utils::ReadTestData(kRootCertOnc);
    policy::PolicyMap policy;
    policy.Set(policy::key::kOpenNetworkConfiguration,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_CLOUD,
               std::make_unique<base::Value>(user_policy_blob), nullptr);
    provider_.UpdateChromePolicy(policy);
    // Note that this relies on the implementation detail that the notification
    // is sent even if the trust roots effectively remain the same.
    trust_roots_changed_observer.Wait();
    user_network_configuration_updater->RemoveTrustedCertsObserver(
        &trust_roots_changed_observer);
  }

  // Verifies |test_server_cert_| with |profile|'s CertVerifier and returns the
  // result.
  int VerifyTestServerCert(Profile* profile) {
    base::RunLoop().RunUntilIdle();
    base::RunLoop run_loop;
    int verification_result;
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter =
        profile->GetRequestContext();
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&VerifyTestServerCertOnIOThread,
                       url_request_context_getter, test_server_cert_,
                       &verification_result),
        run_loop.QuitClosure());
    run_loop.Run();
    return verification_result;
  }

 private:
  MockConfigurationPolicyProvider provider_;

 protected:
  // Certificate which is signed by authority specified in |kRootCertOnc|.
  scoped_refptr<net::X509Certificate> test_server_cert_;
};

class PolicyProvidedTrustRootsRegularUserTest
    : public PolicyProvidedTrustRootsTestBase {};

IN_PROC_BROWSER_TEST_F(PolicyProvidedTrustRootsRegularUserTest,
                       AllowedForRegularUser) {
  SetRootCertONCPolicy(browser()->profile());
  EXPECT_EQ(net::OK, VerifyTestServerCert(browser()->profile()));
}

// Base class for testing policy-provided trust roots with device-local
// accounts. Needs device policy.
class PolicyProvidedTrustRootsDeviceLocalAccountTest
    : public PolicyProvidedTrustRootsTestBase {
 protected:
  void SetUp() override {
    // Configure and start the test server.
    std::unique_ptr<crypto::RSAPrivateKey> signing_key(
        PolicyBuilder::CreateTestSigningKey());
    ASSERT_TRUE(policy_server_.SetSigningKeyAndSignature(
        signing_key.get(), PolicyBuilder::GetTestSigningKeySignature()));
    signing_key.reset();
    policy_server_.RegisterClient(PolicyBuilder::kFakeToken,
                                  PolicyBuilder::kFakeDeviceId);
    ASSERT_TRUE(policy_server_.Start());

    PolicyProvidedTrustRootsTestBase::SetUp();
  }

  virtual void SetupDevicePolicy() = 0;

  void SetUpInProcessBrowserTestFixture() override {
    PolicyProvidedTrustRootsTestBase::SetUpInProcessBrowserTestFixture();

    InstallOwnerKey();
    MarkAsEnterpriseOwned();

    device_policy()->policy_data().set_public_key_version(1);
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    proto.mutable_show_user_names()->set_show_user_names(true);

    SetupDevicePolicy();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PolicyProvidedTrustRootsTestBase::SetUpCommandLine(command_line);
    command_line->AppendSwitch(chromeos::switches::kLoginManager);
    command_line->AppendSwitch(chromeos::switches::kForceLoginManagerInTests);
    command_line->AppendSwitchASCII(chromeos::switches::kLoginProfile, "user");
    command_line->AppendSwitch(chromeos::switches::kOobeSkipPostLogin);
    command_line->AppendSwitchASCII(policy::switches::kDeviceManagementUrl,
                                    policy_server_.GetServiceURL().spec());
  }

  void WaitForSessionStart() {
    if (IsSessionStarted())
      return;
    content::WindowedNotificationObserver(chrome::NOTIFICATION_SESSION_STARTED,
                                          base::Bind(IsSessionStarted))
        .Wait();
  }

  LocalPolicyTestServer policy_server_;

  const AccountId device_local_account_id_ =
      AccountId::FromUserEmail(GenerateDeviceLocalAccountUserId(
          kDeviceLocalAccountId,
          DeviceLocalAccount::TYPE_PUBLIC_SESSION));
};

// Sets up device policy for public session and provides functions to sing into
// it.
class PolicyProvidedTrustRootsPublicSessionTest
    : public PolicyProvidedTrustRootsDeviceLocalAccountTest {
 protected:
  // PolicyProvidedTrustRootsDeviceLocalAccountTest:
  void SetupDevicePolicy() override {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    em::DeviceLocalAccountInfoProto* account =
        proto.mutable_device_local_accounts()->add_account();
    account->set_account_id(kDeviceLocalAccountId);
    account->set_type(
        em::DeviceLocalAccountInfoProto::ACCOUNT_TYPE_PUBLIC_SESSION);
    RefreshDevicePolicy();
    ASSERT_TRUE(
        policy_server_.UpdatePolicy(dm_protocol::kChromeDevicePolicyType,
                                    std::string(), proto.SerializeAsString()));
  }

  void StartLogin() {
    chromeos::WizardController::SkipPostLoginScreensForTesting();
    chromeos::WizardController* const wizard_controller =
        chromeos::WizardController::default_controller();
    ASSERT_TRUE(wizard_controller);
    wizard_controller->SkipToLoginForTesting(chromeos::LoginScreenContext());

    content::WindowedNotificationObserver(
        chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
        content::NotificationService::AllSources())
        .Wait();

    // Login into the public session.
    chromeos::ExistingUserController* controller =
        chromeos::ExistingUserController::current_controller();
    ASSERT_TRUE(controller);
    chromeos::UserContext user_context(user_manager::USER_TYPE_PUBLIC_ACCOUNT,
                                       device_local_account_id_);
    controller->Login(user_context, chromeos::SigninSpecifics());
  }
};

IN_PROC_BROWSER_TEST_F(PolicyProvidedTrustRootsPublicSessionTest,
                       AllowedInPublicSession) {
  StartLogin();
  WaitForSessionStart();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  Browser* browser = browser_list->get(0);
  ASSERT_TRUE(browser);

  SetRootCertONCPolicy(browser->profile());
  EXPECT_EQ(net::OK, VerifyTestServerCert(browser->profile()));
}

}  // namespace policy
