// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/command_line.h"
#include "base/scoped_observer.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_installer_test.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/install/extension_install_ui.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

using content::WebContents;
using extensions::DictionaryBuilder;
using extensions::Extension;
using extensions::ExtensionBuilder;
using extensions::ListBuilder;

const char kWebstoreDomain[] = "cws.com";
const char kAppDomain[] = "app.com";
const char kNonAppDomain[] = "nonapp.com";
const char kTestExtensionId[] = "ecglahbcnmdpdciemllbhojghbkagdje";
const char kTestDataPath[] = "extensions/api_test/webstore_inline_install";
const char kCrxFilename[] = "extension.crx";

class WebstoreStartupInstallerTest : public WebstoreInstallerTest {
 public:
  WebstoreStartupInstallerTest()
      : WebstoreInstallerTest(
            kWebstoreDomain,
            kTestDataPath,
            kCrxFilename,
            kAppDomain,
            kNonAppDomain) {}
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, Install) {
  AutoAcceptInstall();

  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));

  RunTest("runTest");

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  const extensions::Extension* extension =
      registry->enabled_extensions().GetByID(kTestExtensionId);
  EXPECT_TRUE(extension);
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest,
    InstallNotAllowedFromNonVerifiedDomains) {
  AutoCancelInstall();
  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kNonAppDomain, "install_non_verified_domain.html"));

  RunTest("runTest1");
  RunTest("runTest2");
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, FindLink) {
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "find_link.html"));

  RunTest("runTest");
}

// Flakes on all platforms: http://crbug.com/95713, http://crbug.com/229947
IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest,
                       DISABLED_ArgumentValidation) {
  AutoCancelInstall();

  // Each of these tests has to run separately, since one page/tab can
  // only have one in-progress install request. These tests don't all pass
  // callbacks to install, so they have no way to wait for the installation
  // to complete before starting the next test.
  bool is_finished = false;
  for (int i = 0; !is_finished; ++i) {
    ui_test_utils::NavigateToURL(
        browser(),
        GenerateTestServerUrl(kAppDomain, "argument_validation.html"));
    is_finished = !RunIndexedTest("runTest", i);
  }
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, MultipleInstallCalls) {
  AutoCancelInstall();

  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kAppDomain, "multiple_install_calls.html"));
  RunTest("runTest");
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, InstallNotSupported) {
  AutoCancelInstall();
  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kAppDomain, "install_not_supported.html"));

  ui_test_utils::WindowedTabAddedNotificationObserver observer(
      content::NotificationService::AllSources());
  RunTest("runTest");
  observer.Wait();

  // The inline install should fail, and a store-provided URL should be opened
  // in a new tab.
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(GURL("http://cws.com/show-me-the-money"), web_contents->GetURL());
}

// Regression test for http://crbug.com/144991.
IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, InstallFromHostedApp) {
  AutoAcceptInstall();

  const GURL kInstallUrl = GenerateTestServerUrl(kAppDomain, "install.html");

  // We're forced to construct a hosted app dynamically because we need the
  // app to run on a declared URL, but we don't know the port ahead of time.
  scoped_refptr<const Extension> hosted_app =
      ExtensionBuilder()
          .SetManifest(
              DictionaryBuilder()
                  .Set("name", "hosted app")
                  .Set("version", "1")
                  .Set(
                      "app",
                      DictionaryBuilder()
                          .Set("urls",
                               ListBuilder().Append(kInstallUrl.spec()).Build())
                          .Set("launch", DictionaryBuilder()
                                             .Set("web_url", kInstallUrl.spec())
                                             .Build())
                          .Build())
                  .Set("manifest_version", 2)
                  .Build())
          .Build();
  ASSERT_TRUE(hosted_app.get());

  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(browser()->profile())->
          extension_service();
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());

  extension_service->AddExtension(hosted_app.get());
  EXPECT_TRUE(registry->enabled_extensions().GetByID(hosted_app->id()));

  ui_test_utils::NavigateToURL(browser(), kInstallUrl);

  EXPECT_FALSE(registry->enabled_extensions().GetByID(kTestExtensionId));
  RunTest("runTest");
  EXPECT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));
}

class WebstoreStartupInstallerSupervisedUsersTest
    : public WebstoreStartupInstallerTest {
 public:
  // InProcessBrowserTest overrides:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebstoreStartupInstallerTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kSupervisedUserId, "asdf");
  }
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerSupervisedUsersTest,
                       InstallProhibited) {
  AutoAcceptInstall();

  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install_prohibited.html"));

  RunTest("runTest");

  // No error infobar should show up.
  WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  InfoBarService* infobar_service = InfoBarService::FromWebContents(contents);
  EXPECT_EQ(0u, infobar_service->infobar_count());
}

// The unpack failure test needs to use a different install .crx, which is
// specified via a command-line flag, so it needs its own test subclass.
class WebstoreStartupInstallUnpackFailureTest
    : public WebstoreStartupInstallerTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebstoreStartupInstallerTest::SetUpCommandLine(command_line);

    GURL crx_url = GenerateTestServerUrl(
        kWebstoreDomain, "malformed_extension.crx");
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kAppsGalleryUpdateURL, crx_url.spec());
  }

  void SetUpInProcessBrowserTestFixture() override {
    WebstoreStartupInstallerTest::SetUpInProcessBrowserTestFixture();
    extensions::ExtensionInstallUI::set_disable_ui_for_tests();
  }
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallUnpackFailureTest,
    WebstoreStartupInstallUnpackFailureTest) {
  AutoAcceptInstall();

  ui_test_utils::NavigateToURL(browser(),
      GenerateTestServerUrl(kAppDomain, "install_unpack_failure.html"));

  RunTest("runTest");
}
