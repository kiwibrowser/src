// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/blacklist_state.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/test/result_catcher.h"
#include "extensions/test/test_extension_dir.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/url_constants.h"

namespace extensions {

// Tests the privileged components of chrome.runtime.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimePrivileged) {
  ASSERT_TRUE(RunExtensionTest("runtime/privileged")) << message_;
}

// Tests the unprivileged components of chrome.runtime.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeUnprivileged) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(
      LoadExtension(test_data_dir_.AppendASCII("runtime/content_script")));

  // The content script runs on this page.
  extensions::ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  EXPECT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeUninstallURL) {
  // Auto-confirm the uninstall dialog.
  extensions::ScopedTestDialogAutoConfirm auto_confirm(
      extensions::ScopedTestDialogAutoConfirm::ACCEPT);
  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("runtime")
                                .AppendASCII("uninstall_url")
                                .AppendASCII("sets_uninstall_url")));
  ASSERT_TRUE(RunExtensionTest("runtime/uninstall_url")) << message_;
}

namespace {

const char kUninstallUrl[] = "http://www.google.com/";

std::string GetActiveUrl(Browser* browser) {
  return browser->tab_strip_model()
      ->GetActiveWebContents()
      ->GetLastCommittedURL()
      .spec();
}

class RuntimeAPIUpdateTest : public ExtensionApiTest {
 public:
  RuntimeAPIUpdateTest() {}

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    EXPECT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
  }

  struct ExtensionCRXData {
    std::string unpacked_relative_path;
    base::FilePath crx_path;
    explicit ExtensionCRXData(const std::string& unpacked_relative_path)
        : unpacked_relative_path(unpacked_relative_path) {}
  };

  void SetUpCRX(const std::string& root_dir,
                const std::string& pem_filename,
                std::vector<ExtensionCRXData>* crx_data_list) {
    const base::FilePath test_dir = test_data_dir_.AppendASCII(root_dir);
    const base::FilePath pem_path = test_dir.AppendASCII(pem_filename);
    for (ExtensionCRXData& crx_data : *crx_data_list) {
      crx_data.crx_path = PackExtensionWithOptions(
          test_dir.AppendASCII(crx_data.unpacked_relative_path),
          scoped_temp_dir_.GetPath().AppendASCII(
              crx_data.unpacked_relative_path + ".crx"),
          pem_path, base::FilePath());
    }
  }

  bool CrashEnabledExtension(const std::string& extension_id) {
    ExtensionHost* background_host =
        ProcessManager::Get(browser()->profile())
            ->GetBackgroundHostForExtension(extension_id);
    if (!background_host)
      return false;
    content::CrashTab(background_host->host_contents());
    return true;
  }

 private:
  base::ScopedTempDir scoped_temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeAPIUpdateTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeOpenOptionsPage) {
  ASSERT_TRUE(RunExtensionTest("runtime/open_options_page"));
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeOpenOptionsPageError) {
  ASSERT_TRUE(RunExtensionTest("runtime/open_options_page_error"));
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ChromeRuntimeGetPlatformInfo) {
  std::unique_ptr<base::Value> result(
      extension_function_test_utils::RunFunctionAndReturnSingleResult(
          new RuntimeGetPlatformInfoFunction(), "[]", browser()));
  ASSERT_TRUE(result.get() != NULL);
  base::DictionaryValue* dict =
      extension_function_test_utils::ToDictionary(result.get());
  ASSERT_TRUE(dict != NULL);
  EXPECT_TRUE(dict->HasKey("os"));
  EXPECT_TRUE(dict->HasKey("arch"));
  EXPECT_TRUE(dict->HasKey("nacl_arch"));
}

// Tests chrome.runtime.getPackageDirectory with an app.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest,
                       ChromeRuntimeGetPackageDirectoryEntryApp) {
  ASSERT_TRUE(RunPlatformAppTest("api_test/runtime/get_package_directory/app"))
      << message_;
}

// Tests chrome.runtime.getPackageDirectory with an extension.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest,
                       ChromeRuntimeGetPackageDirectoryEntryExtension) {
  ASSERT_TRUE(RunExtensionTest("runtime/get_package_directory/extension"))
      << message_;
}

// Tests chrome.runtime.reload
// This test is flaky: crbug.com/366181
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_ChromeRuntimeReload) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  const char kManifest[] =
      "{"
      "  \"name\": \"reload\","
      "  \"version\": \"1.0\","
      "  \"background\": {"
      "    \"scripts\": [\"background.js\"]"
      "  },"
      "  \"manifest_version\": 2"
      "}";

  TestExtensionDir dir;
  dir.WriteManifest(kManifest);
  dir.WriteFile(FILE_PATH_LITERAL("background.js"), "console.log('loaded');");

  const Extension* extension = LoadExtension(dir.UnpackedPath());
  ASSERT_TRUE(extension);
  const std::string extension_id = extension->id();

  // Somewhat arbitrary upper limit of 30 iterations. If the extension manages
  // to reload itself that often without being terminated, the test fails
  // anyway.
  for (int i = 0; i < 30; i++) {
    TestExtensionRegistryObserver unload_observer(registry, extension_id);
    TestExtensionRegistryObserver load_observer(registry, extension_id);
    ASSERT_TRUE(ExecuteScriptInBackgroundPageNoWait(
        extension_id, "chrome.runtime.reload();"));
    unload_observer.WaitForExtensionUnloaded();
    base::RunLoop().RunUntilIdle();

    if (registry->GetExtensionById(extension_id,
                                   ExtensionRegistry::TERMINATED)) {
      break;
    } else {
      load_observer.WaitForExtensionLoaded();
      // We need to let other registry observers handle the notification to
      // finish initialization
      base::RunLoop().RunUntilIdle();
      WaitForExtensionViewsToLoad();
    }
  }
  ASSERT_TRUE(
      registry->GetExtensionById(extension_id, ExtensionRegistry::TERMINATED));
}

// Tests that updating a terminated extension sends runtime.onInstalled event
// with correct previousVersion.
// Regression test for https://crbug.com/724563.
IN_PROC_BROWSER_TEST_F(RuntimeAPIUpdateTest,
                       TerminatedExtensionUpdateHasCorrectPreviousVersion) {
  std::vector<ExtensionCRXData> data;
  data.emplace_back("v1");
  data.emplace_back("v2");
  SetUpCRX("runtime/update_terminated_extension", "pem.pem", &data);

  ExtensionId extension_id;
  {
    // Install version 1 of the extension.
    ResultCatcher catcher;
    const int expected_change = 1;
    const Extension* extension_v1 =
        InstallExtension(data[0].crx_path, expected_change);
    extension_id = extension_v1->id();
    ASSERT_TRUE(extension_v1);
    EXPECT_TRUE(catcher.GetNextResult());
  }
  ASSERT_TRUE(CrashEnabledExtension(extension_id));
  {
    // Update to version 2, expect runtime.onInstalled with
    // previousVersion = '1'.
    ResultCatcher catcher;
    const int expected_change = 1;
    const Extension* extension_v2 =
        UpdateExtension(extension_id, data[1].crx_path, expected_change);
    ASSERT_TRUE(extension_v2);
    EXPECT_TRUE(catcher.GetNextResult());
  }
}

// Tests that when a blacklisted extension with a set uninstall url is
// uninstalled, its uninstall url does not open.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest,
                       DoNotOpenUninstallUrlForBlacklistedExtensions) {
  // Load an extension that has set an uninstall url.
  scoped_refptr<const extensions::Extension> extension =
      LoadExtension(test_data_dir_.AppendASCII("runtime")
                        .AppendASCII("uninstall_url")
                        .AppendASCII("sets_uninstall_url"));

  ASSERT_TRUE(extension.get());
  extension_service()->AddExtension(extension.get());
  ASSERT_TRUE(extension_service()->IsExtensionEnabled(extension->id()));

  // Uninstall the extension and expect its uninstall url to open.
  extension_service()->UninstallExtension(
      extension->id(), extensions::UNINSTALL_REASON_USER_INITIATED, NULL);
  TabStripModel* tabs = browser()->tab_strip_model();

  EXPECT_EQ(2, tabs->count());
  content::WaitForLoadStop(tabs->GetActiveWebContents());
  // Verify the uninstall url
  EXPECT_EQ(kUninstallUrl, GetActiveUrl(browser()));

  // Close the tab pointing to the uninstall url.
  tabs->CloseWebContentsAt(tabs->active_index(), 0);
  EXPECT_EQ(1, tabs->count());
  EXPECT_EQ("about:blank", GetActiveUrl(browser()));

  // Load the same extension again, except blacklist it after installation.
  extension = LoadExtension(test_data_dir_.AppendASCII("runtime")
                                .AppendASCII("uninstall_url")
                                .AppendASCII("sets_uninstall_url"));
  extension_service()->AddExtension(extension.get());
  ASSERT_TRUE(extension_service()->IsExtensionEnabled(extension->id()));

  // Blacklist extension.
  extensions::ExtensionPrefs::Get(profile())->SetExtensionBlacklistState(
      extension->id(), extensions::BlacklistState::BLACKLISTED_MALWARE);

  // Uninstalling a blacklisted extension should not open its uninstall url.
  TestExtensionRegistryObserver observer(ExtensionRegistry::Get(profile()),
                                         extension->id());
  extension_service()->UninstallExtension(
      extension->id(), extensions::UNINSTALL_REASON_USER_INITIATED, NULL);
  observer.WaitForExtensionUninstalled();

  EXPECT_EQ(1, tabs->count());
  content::WaitForLoadStop(tabs->GetActiveWebContents());
  EXPECT_EQ(url::kAboutBlankURL, GetActiveUrl(browser()));
}

}  // namespace extensions
