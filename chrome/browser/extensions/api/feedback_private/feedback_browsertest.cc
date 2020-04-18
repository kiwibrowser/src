// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/feedback_private/feedback_private_api.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/feedback_private.h"

using extensions::api::feedback_private::FeedbackFlow;

namespace {

void StopMessageLoopCallback() {
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

}  // namespace

namespace extensions {

class FeedbackTest : public ExtensionBrowserTest {
 public:
  void SetUp() override {
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(::switches::kEnableUserMediaScreenCapturing);
  }

 protected:
  bool IsFeedbackAppAvailable() {
    return extensions::EventRouter::Get(browser()->profile())
        ->ExtensionHasEventListener(
            extension_misc::kFeedbackExtensionId,
            extensions::api::feedback_private::OnFeedbackRequested::kEventName);
  }

  void StartFeedbackUI(FeedbackFlow flow,
                       const std::string& extra_diagnostics) {
    base::Closure callback = base::Bind(&StopMessageLoopCallback);
    extensions::FeedbackPrivateGetStringsFunction::set_test_callback(&callback);
    InvokeFeedbackUI(flow, extra_diagnostics);
    content::RunMessageLoop();
    extensions::FeedbackPrivateGetStringsFunction::set_test_callback(NULL);
  }

  void VerifyFeedbackAppLaunch() {
    AppWindow* window =
        PlatformAppBrowserTest::GetFirstAppWindowForBrowser(browser());
    ASSERT_TRUE(window);
    const Extension* feedback_app = window->GetExtension();
    ASSERT_TRUE(feedback_app);
    EXPECT_EQ(feedback_app->id(),
              std::string(extension_misc::kFeedbackExtensionId));
  }

 private:
  void InvokeFeedbackUI(FeedbackFlow flow,
                        const std::string& extra_diagnostics) {
    extensions::FeedbackPrivateAPI* api =
        extensions::FeedbackPrivateAPI::GetFactoryInstance()->Get(
            browser()->profile());
    api->RequestFeedbackForFlow("Test description", "Test placeholder",
                                "Test tag", extra_diagnostics,
                                GURL("http://www.test.com"), flow);
  }
};

// Disabled for ASan due to flakiness on Mac ASan 64 Tests (1).
// See crbug.com/757243.
#if defined(ADDRESS_SANITIZER)
#define MAYBE_ShowFeedback DISABLED_ShowFeedback
#else
#define MAYBE_ShowFeedback ShowFeedback
#endif
IN_PROC_BROWSER_TEST_F(FeedbackTest, MAYBE_ShowFeedback) {
  WaitForExtensionViewsToLoad();

  ASSERT_TRUE(IsFeedbackAppAvailable());
  StartFeedbackUI(FeedbackFlow::FEEDBACK_FLOW_REGULAR, std::string());
  VerifyFeedbackAppLaunch();
}

// Disabled for ASan due to flakiness on Mac ASan 64 Tests (1).
// See crbug.com/757243.
#if defined(ADDRESS_SANITIZER)
#define MAYBE_ShowLoginFeedback DISABLED_ShowLoginFeedback
#else
#define MAYBE_ShowLoginFeedback ShowLoginFeedback
#endif
IN_PROC_BROWSER_TEST_F(FeedbackTest, MAYBE_ShowLoginFeedback) {
  WaitForExtensionViewsToLoad();

  ASSERT_TRUE(IsFeedbackAppAvailable());
  StartFeedbackUI(FeedbackFlow::FEEDBACK_FLOW_LOGIN, std::string());
  VerifyFeedbackAppLaunch();

  AppWindow* const window =
      PlatformAppBrowserTest::GetFirstAppWindowForBrowser(browser());
  ASSERT_TRUE(window);
  content::WebContents* const content = window->web_contents();

  bool bool_result = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      content,
      "domAutomationController.send("
        "$('page-url').hidden && $('attach-file-container').hidden && "
        "$('attach-file-note').hidden);",
      &bool_result));
  EXPECT_TRUE(bool_result);
}

// Disabled for ASan due to flakiness on Mac ASan 64 Tests (1).
// See crbug.com/757243.
#if defined(ADDRESS_SANITIZER)
#define MAYBE_AnonymousUser DISABLED_AnonymousUser
#else
#define MAYBE_AnonymousUser AnonymousUser
#endif
// Tests that there's an option in the email drop down box with a value
// 'anonymous_user'.
IN_PROC_BROWSER_TEST_F(FeedbackTest, MAYBE_AnonymousUser) {
  WaitForExtensionViewsToLoad();

  ASSERT_TRUE(IsFeedbackAppAvailable());
  StartFeedbackUI(FeedbackFlow::FEEDBACK_FLOW_REGULAR, std::string());
  VerifyFeedbackAppLaunch();

  AppWindow* const window =
      PlatformAppBrowserTest::GetFirstAppWindowForBrowser(browser());
  ASSERT_TRUE(window);
  content::WebContents* const content = window->web_contents();

  bool bool_result = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      content,
      "domAutomationController.send("
      "  ((function() {"
      "      var options = $('user-email-drop-down').options;"
      "      for (var option in options) {"
      "        if (options[option].value == 'anonymous_user')"
      "          return true;"
      "      }"
      "      return false;"
      "    })()));",
      &bool_result));

  EXPECT_TRUE(bool_result);
}

// Disabled for ASan due to flakiness on Mac ASan 64 Tests (1).
// See crbug.com/757243.
#if defined(ADDRESS_SANITIZER)
#define MAYBE_ExtraDiagnostics DISABLED_ExtraDiagnostics
#else
#define MAYBE_ExtraDiagnostics ExtraDiagnostics
#endif
// Ensures that when extra diagnostics are provided with feedback, they are
// injected properly in the system information.
IN_PROC_BROWSER_TEST_F(FeedbackTest, MAYBE_ExtraDiagnostics) {
  WaitForExtensionViewsToLoad();

  ASSERT_TRUE(IsFeedbackAppAvailable());
  StartFeedbackUI(FeedbackFlow::FEEDBACK_FLOW_REGULAR, "Some diagnostics");
  VerifyFeedbackAppLaunch();

  AppWindow* const window =
      PlatformAppBrowserTest::GetFirstAppWindowForBrowser(browser());
  ASSERT_TRUE(window);
  content::WebContents* const content = window->web_contents();

  bool bool_result = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      content,
      "domAutomationController.send("
      "  ((function() {"
      "      var sysInfo = feedbackInfo.systemInformation;"
      "      for (var info in sysInfo) {"
      "        if (sysInfo[info].key == 'EXTRA_DIAGNOSTICS' &&"
      "            sysInfo[info].value == 'Some diagnostics') {"
      "          return true;"
      "        }"
      "      }"
      "      return false;"
      "    })()));",
      &bool_result));

  EXPECT_TRUE(bool_result);
}

}  // namespace extensions
