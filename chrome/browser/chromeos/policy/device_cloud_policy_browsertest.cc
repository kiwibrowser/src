// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/dir_reader_posix.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/null_task_runner.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/extensions/signin_screen_policy_provider.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/device_policy_builder.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/policy/test/local_policy_test_server.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/ownership/owner_key_util.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_client.h"
#include "components/policy/core/common/cloud/policy_builder.h"
#include "components/policy/core/common/cloud/resource_cache.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_switches.h"
#include "components/policy/policy_constants.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "components/policy/proto/chrome_extension_policy.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "crypto/rsa_private_key.h"
#include "crypto/sha2.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/test/result_catcher.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace policy {

namespace {

class DeviceCloudPolicyBrowserTest : public InProcessBrowserTest {
 protected:
  DeviceCloudPolicyBrowserTest()
      : mock_client_(std::make_unique<MockCloudPolicyClient>()) {}

  std::unique_ptr<MockCloudPolicyClient> mock_client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCloudPolicyBrowserTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(DeviceCloudPolicyBrowserTest, Initializer) {
  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  // Initializer exists at first.
  EXPECT_TRUE(connector->GetDeviceCloudPolicyInitializer());

  // Initializer is deleted when the manager connects.
  connector->GetDeviceCloudPolicyManager()->StartConnection(
      std::move(mock_client_), connector->GetInstallAttributes());
  EXPECT_FALSE(connector->GetDeviceCloudPolicyInitializer());

  // Initializer is restarted when the manager disconnects.
  connector->GetDeviceCloudPolicyManager()->Disconnect();
  EXPECT_TRUE(connector->GetDeviceCloudPolicyInitializer());
}

namespace {

// Tests for the rotation of the signing keys used for the device policy.
//
// The test is performed against a test policy server, which is set up for
// rotating the policy key automatically with each policy fetch.
class KeyRotationDeviceCloudPolicyTest : public DevicePolicyCrosBrowserTest {
 protected:
  const int kTestPolicyValue = 123;
  const int kTestPolicyOtherValue = 456;
  const char* const kTestPolicyKey = key::kDevicePolicyRefreshRate;

  KeyRotationDeviceCloudPolicyTest() {}

  void SetUp() override {
    UpdateBuiltTestPolicyValue(kTestPolicyValue);
    StartPolicyTestServer();
    DevicePolicyCrosBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    DevicePolicyCrosBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(policy::switches::kDeviceManagementUrl,
                                    policy_test_server_.GetServiceURL().spec());
  }

  void SetUpInProcessBrowserTestFixture() override {
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
    InstallOwnerKey();
    MarkAsEnterpriseOwned();
    SetFakeDevicePolicy();
  }

  void SetUpOnMainThread() override {
    DevicePolicyCrosBrowserTest::SetUpOnMainThread();
    g_browser_process->platform_part()
        ->browser_policy_connector_chromeos()
        ->device_management_service()
        ->ScheduleInitialization(0);
    StartObservingTestPolicy();
  }

  void TearDownOnMainThread() override {
    policy_change_registrar_.reset();
    DevicePolicyCrosBrowserTest::TearDownOnMainThread();
  }

  void UpdateBuiltTestPolicyValue(int test_policy_value) {
    device_policy()
        ->payload()
        .mutable_device_policy_refresh_rate()
        ->set_device_policy_refresh_rate(test_policy_value);
    device_policy()->Build();
  }

  void UpdateServedTestPolicy() {
    EXPECT_TRUE(policy_test_server_.UpdatePolicy(
        dm_protocol::kChromeDevicePolicyType, std::string() /* entity_id */,
        device_policy()->payload().SerializeAsString()));
  }

  int GetTestPolicyValue() {
    PolicyService* const policy_service =
        g_browser_process->platform_part()
            ->browser_policy_connector_chromeos()
            ->GetPolicyService();
    const base::Value* policy_value =
        policy_service
            ->GetPolicies(PolicyNamespace(POLICY_DOMAIN_CHROME,
                                          std::string() /* component_id */))
            .GetValue(kTestPolicyKey);
    EXPECT_TRUE(policy_value);
    int refresh_rate = -1;
    EXPECT_TRUE(policy_value->GetAsInteger(&refresh_rate));
    return refresh_rate;
  }

  void WaitForTestPolicyValue(int expected_policy_value) {
    if (GetTestPolicyValue() == expected_policy_value)
      return;
    awaited_policy_value_ = expected_policy_value;
    // The run loop will be terminated by TestPolicyChangedCallback() once the
    // policy value becomes equal to the awaited value.
    DCHECK(!policy_change_waiting_run_loop_);
    policy_change_waiting_run_loop_ = std::make_unique<base::RunLoop>();
    policy_change_waiting_run_loop_->Run();
  }

 private:
  void StartPolicyTestServer() {
    policy_test_server_.RegisterClient(PolicyBuilder::kFakeToken,
                                       PolicyBuilder::kFakeDeviceId);
    UpdateServedTestPolicy();
    policy_test_server_.EnableAutomaticRotationOfSigningKeys();
    EXPECT_TRUE(policy_test_server_.Start());
  }

  void SetFakeDevicePolicy() {
    device_policy()
        ->payload()
        .mutable_device_policy_refresh_rate()
        ->set_device_policy_refresh_rate(kTestPolicyValue);
    device_policy()->Build();
    session_manager_client()->set_device_policy(device_policy()->GetBlob());
  }

  void StartObservingTestPolicy() {
    policy_change_registrar_ = std::make_unique<PolicyChangeRegistrar>(
        g_browser_process->platform_part()
            ->browser_policy_connector_chromeos()
            ->GetPolicyService(),
        PolicyNamespace(POLICY_DOMAIN_CHROME,
                        std::string() /* component_id */));
    policy_change_registrar_->Observe(
        kTestPolicyKey,
        base::BindRepeating(
            &KeyRotationDeviceCloudPolicyTest::TestPolicyChangedCallback,
            base::Unretained(this)));
  }

  void TestPolicyChangedCallback(const base::Value* old_value,
                                 const base::Value* new_value) {
    if (policy_change_waiting_run_loop_ &&
        GetTestPolicyValue() == awaited_policy_value_) {
      policy_change_waiting_run_loop_->Quit();
    }
  }

  LocalPolicyTestServer policy_test_server_;
  std::unique_ptr<PolicyChangeRegistrar> policy_change_registrar_;
  int awaited_policy_value_ = -1;
  std::unique_ptr<base::RunLoop> policy_change_waiting_run_loop_;

  DISALLOW_COPY_AND_ASSIGN(KeyRotationDeviceCloudPolicyTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(KeyRotationDeviceCloudPolicyTest, Basic) {
  // Initially, the policy has the first value.
  EXPECT_EQ(kTestPolicyValue, GetTestPolicyValue());

  const std::string original_owner_public_key =
      chromeos::DeviceSettingsService::Get()->GetPublicKey()->as_string();

  // The server is updated to serve the new policy value, and the client fetches
  // it.
  UpdateBuiltTestPolicyValue(kTestPolicyOtherValue);
  UpdateServedTestPolicy();
  g_browser_process->platform_part()
      ->browser_policy_connector_chromeos()
      ->GetDeviceCloudPolicyManager()
      ->RefreshPolicies();
  WaitForTestPolicyValue(kTestPolicyOtherValue);
  EXPECT_EQ(kTestPolicyOtherValue, GetTestPolicyValue());

  // The owner key has changed due to the key rotation performed by the policy
  // test server.
  EXPECT_NE(
      original_owner_public_key,
      chromeos::DeviceSettingsService::Get()->GetPublicKey()->as_string());
}

namespace {

// This class is the base class for the tests of the behavior regarding
// extensions installed on the signin screen (which is generally possible only
// through an admin policy, but the tests may install the extensions directly
// for the test purposes).
class SigninExtensionsDeviceCloudPolicyBrowserTestBase
    : public DevicePolicyCrosBrowserTest {
 protected:
  static constexpr const char* kTestExtensionId =
      "baogpbmpccplckhhehfipokjaflkmbno";
  static constexpr const char* kTestExtensionSourceDir =
      "extensions/signin_screen_managed_storage";
  static constexpr const char* kTestExtensionVersion = "1.0";
  static constexpr const char* kTestExtensionTestPage = "test.html";
  static constexpr const char* kFakePolicyUrl =
      "http://example.org/test-policy.json";
  static constexpr const char* kFakePolicy =
      "{\"string-policy\": {\"Value\": \"value\"}}";
  static constexpr int kFakePolicyPublicKeyVersion = 1;
  static constexpr const char* kPolicyProtoCacheKey = "signinextension-policy";
  static constexpr const char* kPolicyDataCacheKey =
      "signinextension-policy-data";

  SigninExtensionsDeviceCloudPolicyBrowserTestBase() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    DevicePolicyCrosBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(chromeos::switches::kLoginManager);
    command_line->AppendSwitch(chromeos::switches::kForceLoginManagerInTests);
    // This flag is required for the successful installation of the test
    // extension into the signin profile, due to some checks in
    // |ExtensionService|.
    command_line->AppendSwitchASCII(::switches::kDisableExtensionsExcept,
                                    GetTestExtensionSourcePath().value());
  }

  void SetUpInProcessBrowserTestFixture() override {
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
    InstallOwnerKey();
    MarkAsEnterpriseOwned();
    SetFakeDevicePolicy();
  }

  void SetUpOnMainThread() override {
    DevicePolicyCrosBrowserTest::SetUpOnMainThread();

    BrowserPolicyConnectorChromeOS* connector =
        g_browser_process->platform_part()->browser_policy_connector_chromeos();
    connector->device_management_service()->ScheduleInitialization(0);
  }

  static base::FilePath GetTestExtensionSourcePath() {
    base::FilePath test_data_dir;
    EXPECT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir));
    return test_data_dir.AppendASCII(kTestExtensionSourceDir);
  }

  static Profile* GetSigninProfile() {
    Profile* signin_profile =
        chromeos::ProfileHelper::GetSigninProfile()->GetOriginalProfile();
    EXPECT_TRUE(signin_profile);
    return signin_profile;
  }

  static const extensions::Extension* GetTestExtension() {
    extensions::ExtensionRegistry* registry =
        extensions::ExtensionRegistry::Get(GetSigninProfile());
    const extensions::Extension* extension =
        registry->enabled_extensions().GetByID(kTestExtensionId);
    EXPECT_TRUE(extension);
    return extension;
  }

  static enterprise_management::PolicyFetchResponse BuildTestComponentPolicy() {
    ComponentCloudPolicyBuilder builder;
    MakeTestComponentPolicyBuilder(&builder);
    return builder.policy();
  }

  static enterprise_management::ExternalPolicyData
  BuildTestComponentPolicyPayload() {
    ComponentCloudPolicyBuilder builder;
    MakeTestComponentPolicyBuilder(&builder);
    return builder.payload();
  }

 private:
  void SetFakeDevicePolicy() {
    device_policy()->policy_data().set_public_key_version(
        kFakePolicyPublicKeyVersion);
    device_policy()->Build();
    session_manager_client()->set_device_policy(device_policy()->GetBlob());
  }

  static void MakeTestComponentPolicyBuilder(
      ComponentCloudPolicyBuilder* builder) {
    builder->policy_data().set_policy_type(
        dm_protocol::kChromeSigninExtensionPolicyType);
    builder->policy_data().set_settings_entity_id(kTestExtensionId);
    builder->policy_data().set_public_key_version(kFakePolicyPublicKeyVersion);
    builder->payload().set_download_url(kFakePolicyUrl);
    builder->payload().set_secure_hash(crypto::SHA256HashString(kFakePolicy));
    builder->Build();
  }

  DISALLOW_COPY_AND_ASSIGN(SigninExtensionsDeviceCloudPolicyBrowserTestBase);
};

// This class tests whether the component policy is successfully processed and
// passed to the extension that is installed into the signin profile after the
// initialization finishes.
//
// The fake component policy is injected by using the local policy test server.
// The test extension is installed into the profile manually by the test code
// (i.e. this class doesn't test the installation of the signin screen
// extensions through the admin policy).
class SigninExtensionsDeviceCloudPolicyBrowserTest
    : public SigninExtensionsDeviceCloudPolicyBrowserTestBase {
 protected:
  SigninExtensionsDeviceCloudPolicyBrowserTest()
      : fetcher_factory_(&fetcher_impl_factory_) {}

  scoped_refptr<const extensions::Extension> InstallAndLoadTestExtension()
      const {
    extensions::ChromeTestExtensionLoader loader(GetSigninProfile());
    return loader.LoadExtension(GetTestExtensionSourcePath());
  }

 private:
  void SetUp() override {
    StartPolicyTestServer();
    DevicePolicyCrosBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SigninExtensionsDeviceCloudPolicyBrowserTestBase::SetUpCommandLine(
        command_line);
    command_line->AppendSwitchASCII(policy::switches::kDeviceManagementUrl,
                                    policy_test_server_.GetServiceURL().spec());
  }

  void SetUpInProcessBrowserTestFixture() override {
    SigninExtensionsDeviceCloudPolicyBrowserTestBase::
        SetUpInProcessBrowserTestFixture();
    signin_policy_provided_disabler_ =
        chromeos::GetScopedSigninScreenPolicyProviderDisablerForTesting();
    EXPECT_TRUE(
        base::PathService::Get(chromeos::DIR_SIGNIN_PROFILE_COMPONENT_POLICY,
                               &component_policy_cache_dir_));
    PrepareFakeComponentPolicyResponse();
  }

  void TearDownInProcessBrowserTestFixture() override {
    // Check that the component policy cache was not cleared during browser
    // teardown.
    ResourceCache cache(component_policy_cache_dir_, new base::NullTaskRunner);
    std::string stub;
    EXPECT_TRUE(cache.Load(kPolicyProtoCacheKey, kTestExtensionId, &stub));
    EXPECT_TRUE(cache.Load(kPolicyDataCacheKey, kTestExtensionId, &stub));

    SigninExtensionsDeviceCloudPolicyBrowserTestBase::
        TearDownInProcessBrowserTestFixture();
  }

  void StartPolicyTestServer() {
    std::unique_ptr<crypto::RSAPrivateKey> signing_key(
        PolicyBuilder::CreateTestSigningKey());
    EXPECT_TRUE(policy_test_server_.SetSigningKeyAndSignature(
        signing_key.get(), PolicyBuilder::GetTestSigningKeySignature()));
    policy_test_server_.RegisterClient(PolicyBuilder::kFakeToken,
                                       PolicyBuilder::kFakeDeviceId);
    EXPECT_TRUE(policy_test_server_.Start());
  }

  void PrepareFakeComponentPolicyResponse() {
    EXPECT_TRUE(policy_test_server_.UpdatePolicy(
        dm_protocol::kChromeSigninExtensionPolicyType, kTestExtensionId,
        BuildTestComponentPolicyPayload().SerializeAsString()));
    fetcher_factory_.SetFakeResponse(GURL(kFakePolicyUrl), kFakePolicy,
                                     net::HTTP_OK,
                                     net::URLRequestStatus::SUCCESS);
  }

  LocalPolicyTestServer policy_test_server_;
  net::URLFetcherImplFactory fetcher_impl_factory_;
  net::FakeURLFetcherFactory fetcher_factory_;
  base::FilePath component_policy_cache_dir_;
  std::unique_ptr<base::AutoReset<bool>> signin_policy_provided_disabler_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(SigninExtensionsDeviceCloudPolicyBrowserTest,
                       InstallAndRunInWindow) {
  scoped_refptr<const extensions::Extension> extension =
      InstallAndLoadTestExtension();
  ASSERT_TRUE(extension);
  Browser* browser = CreateBrowser(GetSigninProfile());
  extensions::ResultCatcher result_catcher;
  ui_test_utils::NavigateToURL(
      browser, extension->GetResourceURL(kTestExtensionTestPage));
  EXPECT_TRUE(result_catcher.GetNextResult());
  CloseBrowserSynchronously(browser);
}

namespace {

// This class tests that the cached component policy is successfully loaded and
// passed to the extension that is already installed into the signin profile.
//
// To accomplish this, the class prefills the signin profile with, first, the
// installed test extension, and, second, with the cached component policy.
class PreinstalledSigninExtensionsDeviceCloudPolicyBrowserTest
    : public SigninExtensionsDeviceCloudPolicyBrowserTestBase {
 protected:
  static constexpr const char* kFakeProfileSourceDir =
      "extensions/profiles/signin_screen_managed_storage";

  bool SetUpUserDataDirectory() override {
    PrefillSigninProfile();
    PrefillComponentPolicyCache();
    return DevicePolicyCrosBrowserTest::SetUpUserDataDirectory();
  }

  void SetUpInProcessBrowserTestFixture() override {
    SigninExtensionsDeviceCloudPolicyBrowserTestBase::
        SetUpInProcessBrowserTestFixture();
    signin_policy_provided_disabler_ =
        chromeos::GetScopedSigninScreenPolicyProviderDisablerForTesting();
  }

 private:
  static void PrefillSigninProfile() {
    base::FilePath profile_source_path;
    EXPECT_TRUE(
        base::PathService::Get(chrome::DIR_TEST_DATA, &profile_source_path));
    profile_source_path = profile_source_path.AppendASCII(kFakeProfileSourceDir)
                              .AppendASCII(chrome::kInitialProfile);

    base::FilePath profile_target_path;
    EXPECT_TRUE(
        base::PathService::Get(chrome::DIR_USER_DATA, &profile_target_path));

    EXPECT_TRUE(
        base::CopyDirectory(profile_source_path, profile_target_path, true));

    base::FilePath extension_target_path =
        profile_target_path.AppendASCII(chrome::kInitialProfile)
            .AppendASCII(extensions::kInstallDirectoryName)
            .AppendASCII(kTestExtensionId)
            .AppendASCII(kTestExtensionVersion);
    EXPECT_TRUE(base::CopyDirectory(GetTestExtensionSourcePath(),
                                    extension_target_path, true));
  }

  void PrefillComponentPolicyCache() {
    base::FilePath user_data_dir;
    EXPECT_TRUE(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
    chromeos::RegisterStubPathOverrides(user_data_dir);

    base::FilePath cache_dir;
    EXPECT_TRUE(base::PathService::Get(
        chromeos::DIR_SIGNIN_PROFILE_COMPONENT_POLICY, &cache_dir));

    ResourceCache cache(cache_dir, new base::NullTaskRunner);
    EXPECT_TRUE(cache.Store(kPolicyProtoCacheKey, kTestExtensionId,
                            BuildTestComponentPolicy().SerializeAsString()));
    EXPECT_TRUE(
        cache.Store(kPolicyDataCacheKey, kTestExtensionId, kFakePolicy));
  }

  std::unique_ptr<base::AutoReset<bool>> signin_policy_provided_disabler_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(PreinstalledSigninExtensionsDeviceCloudPolicyBrowserTest,
                       OfflineStart) {
  const extensions::Extension* extension = GetTestExtension();
  ASSERT_TRUE(extension);
  Browser* browser = CreateBrowser(GetSigninProfile());
  extensions::ResultCatcher result_catcher;
  ui_test_utils::NavigateToURL(
      browser, extension->GetResourceURL(kTestExtensionTestPage));
  EXPECT_TRUE(result_catcher.GetNextResult());
  CloseBrowserSynchronously(browser);
}

}  // namespace policy
