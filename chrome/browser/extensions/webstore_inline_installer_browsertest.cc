// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/webstore_inline_installer.h"

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/extensions/webstore_inline_installer_factory.h"
#include "chrome/browser/extensions/webstore_installer_test.h"
#include "chrome/browser/extensions/webstore_standalone_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/permissions/permission_set.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "url/gurl.h"

using content::WebContents;

namespace extensions {

namespace {

const char kWebstoreDomain[] = "cws.com";
const char kAppDomain[] = "app.com";
const char kNonAppDomain[] = "nonapp.com";
const char kTestExtensionId[] = "ecglahbcnmdpdciemllbhojghbkagdje";
const char kTestDataPath[] = "extensions/api_test/webstore_inline_install";
const char kCrxFilename[] = "extension.crx";

const char kRedirect1Domain[] = "redirect1.com";
const char kRedirect2Domain[] = "redirect2.com";

// A struct for letting us store the actual parameters that were passed to
// the install callback.
struct InstallResult {
  bool success = false;
  std::string error;
  webstore_install::Result result = webstore_install::RESULT_LAST;
};

}  // namespace

class WebstoreInlineInstallerTest : public WebstoreInstallerTest {
 public:
  WebstoreInlineInstallerTest()
      : WebstoreInstallerTest(
            kWebstoreDomain,
            kTestDataPath,
            kCrxFilename,
            kAppDomain,
            kNonAppDomain) {}
};

class ProgrammableInstallPrompt : public ExtensionInstallPrompt {
 public:
  explicit ProgrammableInstallPrompt(WebContents* contents)
      : ExtensionInstallPrompt(contents)
  {}

  ~ProgrammableInstallPrompt() override { g_done_callback = nullptr; }

  void ShowDialog(
      const ExtensionInstallPrompt::DoneCallback& done_callback,
      const Extension* extension,
      const SkBitmap* icon,
      std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt,
      std::unique_ptr<const extensions::PermissionSet> custom_permissions,
      const ShowDialogCallback& show_dialog_callback) override {
    done_callback_ = done_callback;
    g_done_callback = &done_callback_;
  }

  static bool Ready() { return g_done_callback != nullptr; }

  static void Accept() {
    g_done_callback->Run(ExtensionInstallPrompt::Result::ACCEPTED);
  }

  static void Reject() {
    g_done_callback->Run(ExtensionInstallPrompt::Result::USER_CANCELED);
  }

 private:
  static ExtensionInstallPrompt::DoneCallback* g_done_callback;

  ExtensionInstallPrompt::DoneCallback done_callback_;

  DISALLOW_COPY_AND_ASSIGN(ProgrammableInstallPrompt);
};

ExtensionInstallPrompt::DoneCallback*
    ProgrammableInstallPrompt::g_done_callback = nullptr;

// Fake inline installer which creates a programmable prompt in place of
// the normal dialog UI.
class WebstoreInlineInstallerForTest : public WebstoreInlineInstaller {
 public:
  WebstoreInlineInstallerForTest(WebContents* contents,
                                 content::RenderFrameHost* host,
                                 const std::string& extension_id,
                                 const GURL& requestor_url,
                                 const Callback& callback,
                                 bool enable_safebrowsing_redirects)
      : WebstoreInlineInstaller(
            contents,
            host,
            kTestExtensionId,
            requestor_url,
            base::Bind(&WebstoreInlineInstallerForTest::InstallCallback,
                       base::Unretained(this))),
        install_result_target_(nullptr),
        enable_safebrowsing_redirects_(enable_safebrowsing_redirects),
        programmable_prompt_(nullptr) {}

  std::unique_ptr<ExtensionInstallPrompt> CreateInstallUI() override {
    programmable_prompt_ = new ProgrammableInstallPrompt(web_contents());
    return base::WrapUnique(programmable_prompt_);
  }

  // Added here to make it public so that test cases can call it below.
  bool CheckRequestorAlive() const override {
    return WebstoreInlineInstaller::CheckRequestorAlive();
  }

  bool SafeBrowsingNavigationEventsEnabled() const override {
    return enable_safebrowsing_redirects_;
  }

  // Tests that care about the actual arguments to the install callback can use
  // this to receive a copy in |install_result_target|.
  void set_install_result_target(
      std::unique_ptr<InstallResult>* install_result_target) {
    install_result_target_ = install_result_target;
  }

 private:
  ~WebstoreInlineInstallerForTest() override {}

  friend class base::RefCountedThreadSafe<WebstoreStandaloneInstaller>;

  void InstallCallback(bool success,
                       const std::string& error,
                       webstore_install::Result result) {
    if (install_result_target_) {
      install_result_target_->reset(new InstallResult);
      (*install_result_target_)->success = success;
      (*install_result_target_)->error = error;
      (*install_result_target_)->result = result;
    }
  }

  // This can be set by tests that want to get the actual install callback
  // arguments.
  std::unique_ptr<InstallResult>* install_result_target_;

  // This can be set by tests that want to use the new SafeBrowsing redirect
  // tracker.
  bool enable_safebrowsing_redirects_;

  ProgrammableInstallPrompt* programmable_prompt_;
};

class WebstoreInlineInstallerForTestFactory :
    public WebstoreInlineInstallerFactory {
 public:
  WebstoreInlineInstallerForTestFactory()
      : last_installer_(nullptr), enable_safebrowsing_redirects_(false) {}
  explicit WebstoreInlineInstallerForTestFactory(
      bool enable_safebrowsing_redirects)
      : last_installer_(nullptr),
        enable_safebrowsing_redirects_(enable_safebrowsing_redirects) {}
  ~WebstoreInlineInstallerForTestFactory() override {}

  WebstoreInlineInstallerForTest* last_installer() { return last_installer_; }

  WebstoreInlineInstaller* CreateInstaller(
      WebContents* contents,
      content::RenderFrameHost* host,
      const std::string& webstore_item_id,
      const GURL& requestor_url,
      const WebstoreStandaloneInstaller::Callback& callback) override {
    last_installer_ = new WebstoreInlineInstallerForTest(
        contents, host, webstore_item_id, requestor_url, callback,
        enable_safebrowsing_redirects_);
    return last_installer_;
  }

 private:
  // The last installer that was created.
  WebstoreInlineInstallerForTest* last_installer_;

  bool enable_safebrowsing_redirects_;
};

IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       CloseTabBeforeInstallConfirmation) {
  GURL install_url = GenerateTestServerUrl(kAppDomain, "install.html");
  ui_test_utils::NavigateToURL(browser(), install_url);
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  TabHelper* tab_helper = TabHelper::FromWebContents(web_contents);
  tab_helper->SetWebstoreInlineInstallerFactoryForTests(
      new WebstoreInlineInstallerForTestFactory());
  RunTestAsync("runTest");
  while (!ProgrammableInstallPrompt::Ready())
    base::RunLoop().RunUntilIdle();
  web_contents->Close();
  ProgrammableInstallPrompt::Accept();
}

IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       NavigateBeforeInstallConfirmation) {
  GURL install_url = GenerateTestServerUrl(kAppDomain, "install.html");
  ui_test_utils::NavigateToURL(browser(), install_url);
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  TabHelper* tab_helper = TabHelper::FromWebContents(web_contents);
  WebstoreInlineInstallerForTestFactory* factory =
      new WebstoreInlineInstallerForTestFactory();
  tab_helper->SetWebstoreInlineInstallerFactoryForTests(factory);
  RunTestAsync("runTest");
  while (!ProgrammableInstallPrompt::Ready())
    base::RunLoop().RunUntilIdle();
  GURL new_url = GenerateTestServerUrl(kNonAppDomain, "empty.html");
  web_contents->GetController().LoadURL(
      new_url, content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  ASSERT_NE(factory->last_installer(), nullptr);
  EXPECT_NE(factory->last_installer()->web_contents(), nullptr);
  EXPECT_FALSE(factory->last_installer()->CheckRequestorAlive());

  // Right now the way we handle navigations away from the frame that began the
  // inline install is to just declare the requestor to be dead, but not to
  // kill the prompt (that would be a better UX, but more complicated to
  // implement). If we ever do change things to kill the prompt in this case,
  // the following code can be removed (it verifies that clicking ok on the
  // dialog does not result in an install).
  std::unique_ptr<InstallResult> install_result;
  factory->last_installer()->set_install_result_target(&install_result);
  ASSERT_TRUE(ProgrammableInstallPrompt::Ready());
  ProgrammableInstallPrompt::Accept();
  ASSERT_NE(install_result.get(), nullptr);
  EXPECT_EQ(install_result->success, false);
  EXPECT_EQ(install_result->result, webstore_install::ABORTED);
}

// Flaky: https://crbug.com/537526.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       DISABLED_ShouldBlockInlineInstallFromPopupWindow) {
  GURL install_url =
      GenerateTestServerUrl(kAppDomain, "install_from_popup.html");
  // Disable popup blocking for the test url.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(install_url, GURL(),
                                      CONTENT_SETTINGS_TYPE_POPUPS,
                                      std::string(), CONTENT_SETTING_ALLOW);
  ui_test_utils::NavigateToURL(browser(), install_url);
  // The test page opens a popup which is a new |browser| window.
  Browser* popup_browser =
      chrome::FindLastActiveWithProfile(browser()->profile());
  WebContents* popup_contents =
      popup_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(base::ASCIIToUTF16("POPUP"), popup_contents->GetTitle());
  RunTest(popup_contents, "runTest");
}

// Allow inline install while in browser fullscreen mode. Browser fullscreen
// is initiated by the user using F11 (windows), ctrl+cmd+F (mac) or the green
// maximize window button on mac. This will be allowed since it cannot be
// initiated by an API and because of the nuance with mac windows.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       AllowInlineInstallFromFullscreenForBrowser) {
  const GURL install_url = GenerateTestServerUrl(kAppDomain, "install.html");
  ui_test_utils::NavigateToURL(browser(), install_url);
  AutoAcceptInstall();

  // Enter browser fullscreen mode.
  FullscreenController* controller =
      browser()->exclusive_access_manager()->fullscreen_controller();
  controller->ToggleBrowserFullscreenMode();

  RunTest("runTest");

  // Ensure extension is installed.
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  EXPECT_TRUE(
      registry->GenerateInstalledExtensionsSet()->Contains(kTestExtensionId));
}

// Prevent inline install while in tab fullscreen mode. Tab fullscreen is
// initiated using the browser API.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       BlockInlineInstallFromFullscreenForTab) {
  const GURL install_url =
      GenerateTestServerUrl(kAppDomain, "install_from_fullscreen.html");
  ui_test_utils::NavigateToURL(browser(), install_url);
  AutoAcceptInstall();
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  FullscreenController* controller =
      browser()->exclusive_access_manager()->fullscreen_controller();

  // Enter tab fullscreen mode.
  controller->EnterFullscreenModeForTab(web_contents, install_url);

  RunTest("runTest");

  // Ensure extension is not installed.
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  EXPECT_FALSE(
      registry->GenerateInstalledExtensionsSet()->Contains(kTestExtensionId));
}

// Ensure that inline-installing a disabled extension simply re-enables it.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest,
                       ReinstallDisabledExtension) {
  // Install an extension via inline install, and confirm it is successful.
  AutoAcceptInstall();
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));
  RunTest("runTest");
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  ASSERT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));

  // Disable the extension.
  ExtensionService* extension_service =
      ExtensionSystem::Get(browser()->profile())->extension_service();
  extension_service->DisableExtension(kTestExtensionId,
                                      disable_reason::DISABLE_USER_ACTION);
  EXPECT_TRUE(registry->disabled_extensions().GetByID(kTestExtensionId));

  // Revisit the inline install site and reinstall the extension. It should
  // simply be re-enabled, rather than try to install again.
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));
  RunTest("runTest");
  EXPECT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));
  // Since it was disabled by user action, the prompt should have just been the
  // inline install prompt.
  EXPECT_EQ(ExtensionInstallPrompt::INLINE_INSTALL_PROMPT,
            ExtensionInstallPrompt::g_last_prompt_type_for_tests);

  // Disable the extension due to a permissions increase.
  extension_service->DisableExtension(
      kTestExtensionId, disable_reason::DISABLE_PERMISSIONS_INCREASE);
  EXPECT_TRUE(registry->disabled_extensions().GetByID(kTestExtensionId));
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));
  RunTest("runTest");
  EXPECT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));
  // The displayed prompt should be for the permissions increase, versus a
  // normal inline install prompt.
  EXPECT_EQ(ExtensionInstallPrompt::RE_ENABLE_PROMPT,
            ExtensionInstallPrompt::g_last_prompt_type_for_tests);

  ExtensionInstallPrompt::g_last_prompt_type_for_tests =
      ExtensionInstallPrompt::UNSET_PROMPT_TYPE;
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));
  RunTest("runTest");
  // If the extension was already enabled, we should still display an inline
  // install prompt (until we come up with something better).
  EXPECT_EQ(ExtensionInstallPrompt::INLINE_INSTALL_PROMPT,
            ExtensionInstallPrompt::g_last_prompt_type_for_tests);
}

// Test calling chrome.webstore.install() twice without waiting for the first to
// finish.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerTest, DoubleInlineInstallTest) {
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "double_install.html"));
  RunTest("runTest");
}

class WebstoreInlineInstallerRedirectTest
    : public WebstoreInlineInstallerTest,
      public ::testing::WithParamInterface<bool> {
 public:
  WebstoreInlineInstallerRedirectTest() : cws_request_received_(false) {}
  ~WebstoreInlineInstallerRedirectTest() override {}

  void SetUpOnMainThread() override {
    WebstoreInstallerTest::SetUpOnMainThread();
    host_resolver()->AddRule(kRedirect1Domain, "127.0.0.1");
    host_resolver()->AddRule(kRedirect2Domain, "127.0.0.1");
  }

  void ProcessServerRequest(
      const net::test_server::HttpRequest& request) override {
    cws_request_received_ = true;
    if (request.content.empty())
      return;

    cws_request_proto_ =
        std::make_unique<safe_browsing::ExtensionWebStoreInstallRequest>();
    if (!cws_request_proto_->ParseFromString(request.content))
      cws_request_proto_.reset();
  }

  bool cws_request_received_;
  std::unique_ptr<safe_browsing::ExtensionWebStoreInstallRequest>
      cws_request_proto_;
};

// Test that an install from a page arrived at via redirects includes the
// redirect information in the webstore request.
IN_PROC_BROWSER_TEST_P(WebstoreInlineInstallerRedirectTest,
                       IncludesRedirectProtoData) {
  const bool using_safe_browsing_tracker = GetParam();
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  TabHelper* tab_helper = TabHelper::FromWebContents(web_contents);
  WebstoreInlineInstallerForTestFactory* factory =
      new WebstoreInlineInstallerForTestFactory(using_safe_browsing_tracker);
  tab_helper->SetWebstoreInlineInstallerFactoryForTests(factory);

  net::HostPortPair host_port = embedded_test_server()->host_port_pair();

  std::string final_url =
      GenerateTestServerUrl(kAppDomain, "install.html").spec();
  std::string redirect_url =
      base::StringPrintf("http://%s:%d/server-redirect?%s", kRedirect2Domain,
                         host_port.port(), final_url.c_str());
  std::string install_url =
      base::StringPrintf("http://%s:%d/server-redirect?%s", kRedirect1Domain,
                         host_port.port(), redirect_url.c_str());
  AutoAcceptInstall();
  ui_test_utils::NavigateToURL(browser(), GURL(install_url));

  RunTestAsync("runTest");
  while (!ProgrammableInstallPrompt::Ready())
    base::RunLoop().RunUntilIdle();
  web_contents->Close();

  EXPECT_TRUE(cws_request_received_);
  if (!using_safe_browsing_tracker) {
    ASSERT_EQ(nullptr, cws_request_proto_);
    return;
  }
  ASSERT_NE(nullptr, cws_request_proto_);
  ASSERT_EQ(1, cws_request_proto_->referrer_chain_size());

  safe_browsing::ReferrerChainEntry referrer_entry =
      cws_request_proto_->referrer_chain(0);

  // Check that the expected domains are in the redirect list.
  const std::set<std::string> expected_redirect_domains = {
      kRedirect1Domain, kRedirect2Domain, kAppDomain};

  EXPECT_EQ(final_url, referrer_entry.url());
  EXPECT_EQ(safe_browsing::ReferrerChainEntry::CLIENT_REDIRECT,
            referrer_entry.type());
  EXPECT_EQ(3, referrer_entry.server_redirect_chain_size());
  EXPECT_EQ(install_url, referrer_entry.server_redirect_chain(0).url());
  EXPECT_EQ(redirect_url, referrer_entry.server_redirect_chain(1).url());
  EXPECT_EQ(final_url, referrer_entry.server_redirect_chain(2).url());
  EXPECT_TRUE(cws_request_proto_->referrer_chain_options()
                  .has_recent_navigations_to_collect());
}

// Test that an install from a page arrived at via redirects does not include
// redirect information when SafeBrowsing is disabled.
IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerRedirectTest,
                       NoRedirectDataWhenSafeBrowsingDisabled) {
  PrefService* pref_service = browser()->profile()->GetPrefs();
  EXPECT_TRUE(pref_service->GetBoolean(prefs::kSafeBrowsingEnabled));

  // Disable SafeBrowsing.
  pref_service->SetBoolean(prefs::kSafeBrowsingEnabled, false);

  // Hand craft a url that will cause the test server to issue redirects.
  const std::vector<std::string> redirects = {kRedirect1Domain,
                                              kRedirect2Domain};
  net::HostPortPair host_port = embedded_test_server()->host_port_pair();
  std::string redirect_chain;
  for (const auto& redirect : redirects) {
    std::string redirect_url = base::StringPrintf(
        "http://%s:%d/server-redirect?", redirect.c_str(), host_port.port());
    redirect_chain += redirect_url;
  }
  const GURL install_url =
      GURL(redirect_chain +
           GenerateTestServerUrl(kAppDomain, "install.html").spec());

  AutoAcceptInstall();
  ui_test_utils::NavigateToURL(browser(), install_url);
  RunTest("runTest");

  EXPECT_TRUE(cws_request_received_);
  ASSERT_EQ(nullptr, cws_request_proto_);
}

INSTANTIATE_TEST_CASE_P(NetRedirectTracking,
                        WebstoreInlineInstallerRedirectTest,
                        testing::Values(false));
INSTANTIATE_TEST_CASE_P(SafeBrowsingRedirectTracking,
                        WebstoreInlineInstallerRedirectTest,
                        testing::Values(true));

class WebstoreInlineInstallerListenerTest : public WebstoreInlineInstallerTest {
 public:
  WebstoreInlineInstallerListenerTest() {}
  ~WebstoreInlineInstallerListenerTest() override {}

 protected:
  void RunTest(const std::string& file_name) {
    AutoAcceptInstall();
    ui_test_utils::NavigateToURL(browser(),
                                 GenerateTestServerUrl(kAppDomain, file_name));
    WebstoreInstallerTest::RunTest("runTest");
  }
};

IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerListenerTest,
                       InstallStageListenerTest) {
  RunTest("install_stage_listener.html");
}

IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerListenerTest,
                       DownloadProgressListenerTest) {
  RunTest("download_progress_listener.html");
}

IN_PROC_BROWSER_TEST_F(WebstoreInlineInstallerListenerTest, BothListenersTest) {
  RunTest("both_listeners.html");
  // The extension should be installed.
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  EXPECT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));

  // Rinse and repeat: uninstall the extension, open a new tab, and install it
  // again. Regression test for crbug.com/613949.
  extension_service()->UninstallExtension(
      kTestExtensionId, UNINSTALL_REASON_FOR_TESTING, nullptr);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(registry->enabled_extensions().GetByID(kTestExtensionId));
  int old_tab_index = browser()->tab_strip_model()->active_index();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GenerateTestServerUrl(kAppDomain, "both_listeners.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  DCHECK_NE(old_tab_index, browser()->tab_strip_model()->active_index());
  browser()->tab_strip_model()->CloseWebContentsAt(old_tab_index,
                                                   TabStripModel::CLOSE_NONE);
  WebstoreInstallerTest::RunTest("runTest");
  EXPECT_TRUE(registry->enabled_extensions().GetByID(kTestExtensionId));
}

}  // namespace extensions
