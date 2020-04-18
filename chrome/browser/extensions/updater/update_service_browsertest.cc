// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "chrome/browser/extensions/content_verifier_test_utils.h"
#include "chrome/browser/extensions/extension_management_test_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"
#include "chrome/browser/extensions/updater/extension_update_client_base_browsertest.h"
#include "chrome/browser/extensions/updater/extension_updater.h"
#include "chrome/common/chrome_switches.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/update_client/url_request_post_interceptor.h"
#include "extensions/browser/content_verifier.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/external_install_info.h"
#include "extensions/browser/mock_external_provider.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/updater/extension_downloader.h"
#include "extensions/browser/updater/manifest_fetch_data.h"
#include "extensions/common/extension_urls.h"
#include "net/url_request/test_url_request_interceptor.h"

namespace extensions {

namespace {

const char kExtensionId[] = "npnbmohejbjohgpjnmjagbafnjhkmgko";

using UpdateClientEvents = update_client::UpdateClient::Observer::Events;

}  // namespace

class UpdateServiceTest : public ExtensionUpdateClientBaseTest {
 public:
  UpdateServiceTest() {}
  ~UpdateServiceTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionUpdateClientBaseTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kExtensionContentVerification,
        switches::kExtensionContentVerificationEnforce);
  }

  bool ShouldEnableContentVerification() override { return true; }
};

IN_PROC_BROWSER_TEST_F(UpdateServiceTest, NoUpdate) {
  // Verify that UpdateService runs correctly when there's no update.
  base::ScopedAllowBlockingForTesting allow_io;

  // Mock a no-update response.
  const base::FilePath update_response =
      test_data_dir_.AppendASCII("updater/updatecheck_reply_noupdate_1.xml");
  ASSERT_TRUE(update_interceptor_->ExpectRequest(
      std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
      update_response));

  const base::FilePath crx_path = test_data_dir_.AppendASCII("updater/v1.crx");
  const Extension* extension =
      InstallExtension(crx_path, 1, Manifest::EXTERNAL_POLICY_DOWNLOAD);
  ASSERT_TRUE(extension);
  EXPECT_EQ(kExtensionId, extension->id());

  extensions::ExtensionUpdater::CheckParams params;
  params.ids = {kExtensionId};
  extension_service()->updater()->CheckNow(std::move(params));

  // UpdateService should emit an not-updated event.
  EXPECT_EQ(UpdateClientEvents::COMPONENT_NOT_UPDATED,
            WaitOnComponentUpdaterCompleteEvent(kExtensionId));

  ASSERT_EQ(1, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();

  // No update, thus no download nor ping activities.
  EXPECT_EQ(0, get_interceptor_->GetHitCount());
  EXPECT_EQ(0, ping_interceptor_->GetCount())
      << ping_interceptor_->GetRequestsAsString();

  const std::string update_request =
      update_interceptor_->GetRequests()[0].first;
  EXPECT_THAT(update_request,
              ::testing::HasSubstr(base::StringPrintf(
                  R"(<app appid="%s" version="1")", kExtensionId)));
  EXPECT_THAT(update_request, ::testing::HasSubstr(R"(enabled="1")"));
}

IN_PROC_BROWSER_TEST_F(UpdateServiceTest, SuccessfulUpdate) {
  base::ScopedAllowBlockingForTesting allow_io;

  // Mock an update response.
  const base::FilePath update_response =
      test_data_dir_.AppendASCII("updater/updatecheck_reply_update_1.xml");
  const base::FilePath ping_response =
      test_data_dir_.AppendASCII("updater/ping_reply_1.xml");
  const base::FilePath crx_path = test_data_dir_.AppendASCII("updater/v1.crx");

  ASSERT_TRUE(update_interceptor_->ExpectRequest(
      std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
      update_response));
  ASSERT_TRUE(ping_interceptor_->ExpectRequest(
      std::make_unique<update_client::PartialMatch>("eventtype"),
      ping_response));
  get_interceptor_->SetResponseIgnoreQuery(
      GURL("http://localhost/download/v1.crx"), crx_path);

  const Extension* extension =
      InstallExtension(crx_path, 1, Manifest::EXTERNAL_POLICY_DOWNLOAD);
  ASSERT_TRUE(extension);
  EXPECT_EQ(kExtensionId, extension->id());

  base::RunLoop run_loop;

  extensions::ExtensionUpdater::CheckParams params;
  params.ids = {kExtensionId};
  params.callback = run_loop.QuitClosure();
  extension_service()->updater()->CheckNow(std::move(params));

  EXPECT_EQ(UpdateClientEvents::COMPONENT_UPDATED,
            WaitOnComponentUpdaterCompleteEvent(kExtensionId));

  run_loop.Run();

  ASSERT_EQ(1, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(1, get_interceptor_->GetHitCount());

  const std::string update_request =
      update_interceptor_->GetRequests()[0].first;
  EXPECT_THAT(update_request,
              ::testing::HasSubstr(base::StringPrintf(
                  R"(<app appid="%s" version="1")", kExtensionId)));
  EXPECT_THAT(update_request, ::testing::HasSubstr(R"(enabled="1")"));
}

IN_PROC_BROWSER_TEST_F(UpdateServiceTest, PolicyCorrupted) {
  base::ScopedAllowBlockingForTesting allow_io;

  ExtensionSystem* system = ExtensionSystem::Get(profile());
  ExtensionService* service = extension_service();

  const base::FilePath update_response =
      test_data_dir_.AppendASCII("updater/updatecheck_reply_update_1.xml");
  const base::FilePath ping_response =
      test_data_dir_.AppendASCII("updater/ping_reply_1.xml");
  const base::FilePath crx_path = test_data_dir_.AppendASCII("updater/v1.crx");

  ASSERT_TRUE(update_interceptor_->ExpectRequest(
      std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
      update_response));
  ASSERT_TRUE(ping_interceptor_->ExpectRequest(
      std::make_unique<update_client::PartialMatch>("eventtype"),
      ping_response));
  get_interceptor_->SetResponseIgnoreQuery(
      GURL("http://localhost/download/v1.crx"), crx_path);

  // Setup fake policy and update check objects.
  content_verifier_test::ForceInstallProvider policy(kExtensionId);
  system->management_policy()->RegisterProvider(&policy);
  auto external_provider = std::make_unique<MockExternalProvider>(
      service, Manifest::EXTERNAL_POLICY_DOWNLOAD);
  external_provider->UpdateOrAddExtension(
      std::make_unique<ExternalInstallInfoUpdateUrl>(
          kExtensionId, std::string() /* install_parameter */,
          extension_urls::GetWebstoreUpdateUrl(),
          Manifest::EXTERNAL_POLICY_DOWNLOAD, 0 /* creation_flags */,
          true /* mark_acknowledged */));
  service->AddProviderForTesting(std::move(external_provider));

  const Extension* extension =
      InstallExtension(crx_path, 1, Manifest::EXTERNAL_POLICY_DOWNLOAD);
  ASSERT_TRUE(extension);
  EXPECT_EQ(kExtensionId, extension->id());

  TestExtensionRegistryObserver registry_observer(
      ExtensionRegistry::Get(profile()), kExtensionId);
  ContentVerifier* verifier = system->content_verifier();
  verifier->VerifyFailed(kExtensionId, ContentVerifyJob::HASH_MISMATCH);

  // Make sure the extension first got disabled due to corruption.
  EXPECT_TRUE(registry_observer.WaitForExtensionUnloaded());
  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile());
  int reasons = prefs->GetDisableReasons(kExtensionId);
  EXPECT_TRUE(reasons & disable_reason::DISABLE_CORRUPTED);

  // Make sure the extension then got re-installed, and that after reinstall it
  // is no longer disabled due to corruption.
  EXPECT_EQ(UpdateClientEvents::COMPONENT_UPDATED,
            WaitOnComponentUpdaterCompleteEvent(kExtensionId));

  reasons = prefs->GetDisableReasons(kExtensionId);
  EXPECT_FALSE(reasons & disable_reason::DISABLE_CORRUPTED);

  ASSERT_EQ(1, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(1, get_interceptor_->GetHitCount());

  // Make sure that the update check request is formed correctly when the
  // extension is corrupted:
  // - version="0.0.0.0"
  // - installsource="reinstall"
  // - installedby="policy"
  // - enabled="0"
  // - <disabled reason="1024"/>
  const std::string update_request =
      update_interceptor_->GetRequests()[0].first;
  EXPECT_THAT(update_request,
              ::testing::HasSubstr(base::StringPrintf(
                  R"(<app appid="%s" version="0.0.0.0")", kExtensionId)));
  EXPECT_THAT(
      update_request,
      ::testing::HasSubstr(
          R"(installsource="reinstall" installedby="policy" enabled="0")"));
  EXPECT_THAT(update_request, ::testing::HasSubstr(base::StringPrintf(
                                  R"(<disabled reason="%d"/>)",
                                  disable_reason::DISABLE_CORRUPTED)));
}

IN_PROC_BROWSER_TEST_F(UpdateServiceTest, UninstallExtensionWhileUpdating) {
  // This test is to verify that the extension updater engine (update client)
  // works correctly when an extension is uninstalled when the extension updater
  // is in progress.
  base::ScopedAllowBlockingForTesting allow_io;

  const base::FilePath crx_path = test_data_dir_.AppendASCII("updater/v1.crx");

  const Extension* extension =
      InstallExtension(crx_path, 1, Manifest::EXTERNAL_POLICY_DOWNLOAD);
  ASSERT_TRUE(extension);
  EXPECT_EQ(kExtensionId, extension->id());

  base::RunLoop run_loop;

  extensions::ExtensionUpdater::CheckParams params;
  params.ids = {kExtensionId};
  params.callback = run_loop.QuitClosure();
  extension_service()->updater()->CheckNow(std::move(params));

  // Uninstall the extension right before the message loop is executed to
  // emulate uninstalling an extension in the middle of an extension update.
  extension_service()->UninstallExtension(
      kExtensionId, extensions::UNINSTALL_REASON_COMPONENT_REMOVED, nullptr);

  // Update client should issue an update error event for this extension.
  ASSERT_EQ(UpdateClientEvents::COMPONENT_UPDATE_ERROR,
            WaitOnComponentUpdaterCompleteEvent(kExtensionId));

  run_loop.Run();

  EXPECT_EQ(0, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(0, get_interceptor_->GetHitCount());
}

class PolicyUpdateServiceTest : public ExtensionUpdateClientBaseTest {
 public:
  PolicyUpdateServiceTest() {}
  ~PolicyUpdateServiceTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionUpdateClientBaseTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kExtensionContentVerification,
        switches::kExtensionContentVerificationEnforce);
  }

  void SetUpInProcessBrowserTestFixture() override {
    ExtensionUpdateClientBaseTest::SetUpInProcessBrowserTestFixture();

    EXPECT_CALL(policy_provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));

    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
        &policy_provider_);
    ExtensionManagementPolicyUpdater management_policy(&policy_provider_);
    management_policy.SetIndividualExtensionAutoInstalled(
        id_, extension_urls::kChromeWebstoreUpdateURL, true /* forced */);

    // The policy will force the new install of an extension, which the
    // component updater doesn't support yet. We still rely on the old updater
    // to install a new extension.
    const base::FilePath crx_path =
        test_data_dir_.AppendASCII("updater/v1.crx");
    ExtensionDownloader::set_test_delegate(&downloader_);
    downloader_.AddResponse(id_, "2", crx_path);
  }

  void SetUpNetworkInterceptors() override {
    ExtensionUpdateClientBaseTest::SetUpNetworkInterceptors();

    const base::FilePath crx_path =
        test_data_dir_.AppendASCII("updater/v1.crx");
    const base::FilePath update_response =
        test_data_dir_.AppendASCII("updater/updatecheck_reply_update_1.xml");
    const base::FilePath ping_response =
        test_data_dir_.AppendASCII("updater/ping_reply_1.xml");

    get_interceptor_->SetResponseIgnoreQuery(
        GURL("http://localhost/download/v1.crx"), crx_path);
    ASSERT_TRUE(update_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
        update_response));
    ASSERT_TRUE(update_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
        update_response));
    ASSERT_TRUE(update_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
        update_response));
    ASSERT_TRUE(update_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("<updatecheck/>"),
        update_response));
    ASSERT_TRUE(ping_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("eventtype"),
        ping_response));
    ASSERT_TRUE(ping_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("eventtype"),
        ping_response));
    ASSERT_TRUE(ping_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("eventtype"),
        ping_response));
    ASSERT_TRUE(ping_interceptor_->ExpectRequest(
        std::make_unique<update_client::PartialMatch>("eventtype"),
        ping_response));
  }

  std::vector<GURL> GetUpdateUrls() const override {
    return {GURL("https://policy-updatehost/service/update")};
  }

  std::vector<GURL> GetPingUrls() const override {
    return {GURL("https://policy-pinghost/service/ping")};
  }

 protected:
  // The id of the extension we want to have force-installed.
  std::string id_ = "npnbmohejbjohgpjnmjagbafnjhkmgko";

 private:
  policy::MockConfigurationPolicyProvider policy_provider_;
  content_verifier_test::DownloaderTestDelegate downloader_;
};

// Tests that if CheckForExternalUpdates() fails, then we retry reinstalling
// corrupted policy extensions. For example: if network is unavailable,
// CheckForExternalUpdates() will fail.
IN_PROC_BROWSER_TEST_F(PolicyUpdateServiceTest, FailedUpdateRetries) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  ExtensionService* service = extension_service();
  ContentVerifier* verifier =
      ExtensionSystem::Get(profile())->content_verifier();

  // Wait for the extension to be installed by the policy we set up in
  // SetUpInProcessBrowserTestFixture.
  if (!registry->GetInstalledExtension(id_)) {
    TestExtensionRegistryObserver registry_observer(registry, id_);
    EXPECT_TRUE(registry_observer.WaitForExtensionInstalled());
  }

  content_verifier_test::DelayTracker delay_tracker;
  service->set_external_updates_disabled_for_test(true);
  TestExtensionRegistryObserver registry_observer(registry, id_);
  verifier->VerifyFailed(id_, ContentVerifyJob::HASH_MISMATCH);
  EXPECT_TRUE(registry_observer.WaitForExtensionUnloaded());

  const std::vector<base::TimeDelta>& calls = delay_tracker.calls();
  ASSERT_EQ(1u, calls.size());
  EXPECT_EQ(base::TimeDelta(), delay_tracker.calls()[0]);

  delay_tracker.Proceed();

  // Remove the override and set ExtensionService to update again. The extension
  // should be now installed.
  service->set_external_updates_disabled_for_test(false);
  delay_tracker.StopWatching();
  delay_tracker.Proceed();

  EXPECT_EQ(UpdateClientEvents::COMPONENT_UPDATED,
            WaitOnComponentUpdaterCompleteEvent(id_));

  ASSERT_EQ(1, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(1, get_interceptor_->GetHitCount());

  // Make sure that the update check request is formed correctly when the
  // extension is corrupted:
  // - version="0.0.0.0"
  // - installsource="reinstall"
  // - installedby="policy"
  // - enabled="0"
  // - <disabled reason="1024"/>
  const std::string update_request =
      update_interceptor_->GetRequests()[0].first;
  EXPECT_THAT(update_request,
              ::testing::HasSubstr(base::StringPrintf(
                  R"(<app appid="%s" version="0.0.0.0")", id_.c_str())));
  EXPECT_THAT(
      update_request,
      ::testing::HasSubstr(
          R"(installsource="reinstall" installedby="policy" enabled="0")"));
  EXPECT_THAT(update_request, ::testing::HasSubstr(base::StringPrintf(
                                  R"(<disabled reason="%d"/>)",
                                  disable_reason::DISABLE_CORRUPTED)));
}

IN_PROC_BROWSER_TEST_F(PolicyUpdateServiceTest, Backoff) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  ContentVerifier* verifier =
      ExtensionSystem::Get(profile())->content_verifier();

  // Wait for the extension to be installed by the policy we set up in
  // SetUpInProcessBrowserTestFixture.
  if (!registry->GetInstalledExtension(id_)) {
    TestExtensionRegistryObserver registry_observer(registry, id_);
    EXPECT_TRUE(registry_observer.WaitForExtensionInstalled());
  }

  // Setup to intercept reinstall action, so we can see what the delay would
  // have been for the real action.
  content_verifier_test::DelayTracker delay_tracker;

  // Do 4 iterations of disabling followed by reinstall.
  const size_t iterations = 4;
  for (size_t i = 0; i < iterations; i++) {
    TestExtensionRegistryObserver registry_observer(registry, id_);
    verifier->VerifyFailed(id_, ContentVerifyJob::HASH_MISMATCH);
    EXPECT_TRUE(registry_observer.WaitForExtensionUnloaded());
    // Resolve the request to |delay_tracker|, so the reinstallation can
    // proceed.
    delay_tracker.Proceed();
    EXPECT_EQ(UpdateClientEvents::COMPONENT_UPDATED,
              WaitOnComponentUpdaterCompleteEvent(id_));
  }

  ASSERT_EQ(4, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(4, get_interceptor_->GetHitCount());

  const std::vector<base::TimeDelta>& calls = delay_tracker.calls();

  // After |delay_tracker| resolves the 4 (|iterations|) reinstallation
  // requests, it will get an additional request (right away) for retrying
  // reinstallation.
  // Note: the additional request in non-test environment will arrive with
  // a (backoff) delay. But during test, |delay_tracker| issues the request
  // immediately.
  ASSERT_EQ(iterations, calls.size() - 1);
  // Assert that the first reinstall action happened with a delay of 0, and
  // then kept growing each additional time.
  EXPECT_EQ(base::TimeDelta(), delay_tracker.calls()[0]);
  for (size_t i = 1; i < delay_tracker.calls().size(); i++) {
    EXPECT_LT(calls[i - 1], calls[i]);
  }
}

// We want to test what happens at startup with a corroption-disabled policy
// force installed extension. So we set that up in the PRE test here.
IN_PROC_BROWSER_TEST_F(PolicyUpdateServiceTest, PRE_PolicyCorruptedOnStartup) {
  // This is to not allow any corrupted resintall to proceed.
  content_verifier_test::DelayTracker delay_tracker;
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  TestExtensionRegistryObserver registry_observer(registry, id_);

  // Wait for the extension to be installed by policy we set up in
  // SetUpInProcessBrowserTestFixture.
  if (!registry->GetInstalledExtension(id_))
    EXPECT_TRUE(registry_observer.WaitForExtensionInstalled());

  // Simulate corruption of the extension so that we can test what happens
  // at startup in the non-PRE test.
  ContentVerifier* verifier =
      ExtensionSystem::Get(profile())->content_verifier();
  verifier->VerifyFailed(id_, ContentVerifyJob::HASH_MISMATCH);
  EXPECT_TRUE(registry_observer.WaitForExtensionUnloaded());

  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile());
  int reasons = prefs->GetDisableReasons(id_);
  EXPECT_TRUE(reasons & disable_reason::DISABLE_CORRUPTED);
  EXPECT_EQ(1u, delay_tracker.calls().size());

  EXPECT_EQ(0, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(0, get_interceptor_->GetHitCount());
}

// Now actually test what happens on the next startup after the PRE test above.
IN_PROC_BROWSER_TEST_F(PolicyUpdateServiceTest, PolicyCorruptedOnStartup) {
  // Depdending on timing, the extension may have already been reinstalled
  // between SetUpInProcessBrowserTestFixture and now (usually not during local
  // testing on a developer machine, but sometimes on a heavily loaded system
  // such as the build waterfall / trybots). If the reinstall didn't already
  // happen, wait for it.

  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile());
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  int disable_reasons = prefs->GetDisableReasons(id_);
  if (disable_reasons & disable_reason::DISABLE_CORRUPTED) {
    EXPECT_EQ(UpdateClientEvents::COMPONENT_UPDATED,
              WaitOnComponentUpdaterCompleteEvent(id_));
    disable_reasons = prefs->GetDisableReasons(id_);
  }

  EXPECT_FALSE(disable_reasons & disable_reason::DISABLE_CORRUPTED);
  EXPECT_TRUE(registry->enabled_extensions().Contains(id_));

  ASSERT_EQ(1, update_interceptor_->GetCount())
      << update_interceptor_->GetRequestsAsString();
  EXPECT_EQ(1, get_interceptor_->GetHitCount());

  const std::string update_request =
      update_interceptor_->GetRequests()[0].first;
  EXPECT_THAT(update_request,
              ::testing::HasSubstr(base::StringPrintf(
                  R"(<app appid="%s" version="0.0.0.0")", id_.c_str())));
  EXPECT_THAT(
      update_request,
      ::testing::HasSubstr(
          R"(installsource="reinstall" installedby="policy" enabled="0")"));
  EXPECT_THAT(update_request, ::testing::HasSubstr(base::StringPrintf(
                                  R"(<disabled reason="%d"/>)",
                                  disable_reason::DISABLE_CORRUPTED)));
}

}  // namespace extensions
