// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/ui/extensions/browser_action_test_util.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/test/result_catcher.h"
#include "extensions/test/test_extension_dir.h"
#include "media/base/media_switches.h"

class AutoplayExtensionBrowserTest : public extensions::ExtensionApiTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kDocumentUserActivationRequiredPolicy);
  }
};

IN_PROC_BROWSER_TEST_F(AutoplayExtensionBrowserTest, AutoplayAllowed) {
  ASSERT_TRUE(RunExtensionTest("autoplay")) << message_;
}

IN_PROC_BROWSER_TEST_F(AutoplayExtensionBrowserTest, AutoplayAllowedInIframe) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  const extensions::Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("autoplay_iframe"));
  ASSERT_TRUE(extension) << message_;

  std::unique_ptr<BrowserActionTestUtil> browser_action_test_util =
      BrowserActionTestUtil::Create(browser());
  extensions::ResultCatcher catcher;
  content::WindowedNotificationObserver popup_observer(
      content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
      content::NotificationService::AllSources());
  browser_action_test_util->Press(0);
  popup_observer.Wait();
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(AutoplayExtensionBrowserTest,
                       AutoplayAllowedInHostedApp) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  GURL app_url = embedded_test_server()->GetURL(
      "/extensions/autoplay_hosted_app/main.html");

  constexpr const char kHostedAppManifest[] =
      R"( { "name": "Hosted App Autoplay Test",
            "version": "1",
            "manifest_version": 2,
            "app": {
              "launch": {
                "web_url": "%s"
              }
            }
          } )";
  extensions::TestExtensionDir test_app_dir;
  test_app_dir.WriteManifest(
      base::StringPrintf(kHostedAppManifest, app_url.spec().c_str()));

  const extensions::Extension* extension =
      LoadExtension(test_app_dir.UnpackedPath());
  ASSERT_TRUE(extension) << message_;

  Browser* app_browser = LaunchAppBrowser(extension);
  content::WebContents* web_contents =
      app_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));

  bool result = false;
  EXPECT_TRUE(content::ExecuteScriptWithoutUserGestureAndExtractBool(
      web_contents, "runTest();", &result));
  EXPECT_TRUE(result);
}
