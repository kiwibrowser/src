// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/browser/guest_view_manager_factory.h"
#include "components/guest_view/browser/test_guest_view_manager.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_paths.h"
#include "extensions/shell/browser/shell_app_delegate.h"
#include "extensions/shell/browser/shell_app_view_guest_delegate.h"
#include "extensions/shell/browser/shell_content_browser_client.h"
#include "extensions/shell/browser/shell_extension_system.h"
#include "extensions/shell/browser/shell_extensions_api_client.h"
#include "extensions/shell/browser/shell_extensions_browser_client.h"
#include "extensions/shell/test/shell_test.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using guest_view::GuestViewManager;
using guest_view::TestGuestViewManager;
using guest_view::TestGuestViewManagerFactory;

namespace {

class MockShellAppDelegate : public extensions::ShellAppDelegate {
 public:
  MockShellAppDelegate() : requested_(false) {
    DCHECK(instance_ == nullptr);
    instance_ = this;
  }
  ~MockShellAppDelegate() override { instance_ = nullptr; }

  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback,
      const extensions::Extension* extension) override {
    requested_ = true;
    if (request_message_loop_runner_.get())
      request_message_loop_runner_->Quit();
  }

  void WaitForRequestMediaPermission() {
    if (requested_)
      return;
    request_message_loop_runner_ = new content::MessageLoopRunner;
    request_message_loop_runner_->Run();
  }

  static MockShellAppDelegate* Get() { return instance_; }

 private:
  bool requested_;
  scoped_refptr<content::MessageLoopRunner> request_message_loop_runner_;
  static MockShellAppDelegate* instance_;
};

MockShellAppDelegate* MockShellAppDelegate::instance_ = nullptr;

class MockShellAppViewGuestDelegate
    : public extensions::ShellAppViewGuestDelegate {
 public:
  MockShellAppViewGuestDelegate() {}

  extensions::AppDelegate* CreateAppDelegate() override {
    return new MockShellAppDelegate();
  }
};

class MockExtensionsAPIClient : public extensions::ShellExtensionsAPIClient {
 public:
  MockExtensionsAPIClient() {}

  extensions::AppViewGuestDelegate* CreateAppViewGuestDelegate()
      const override {
    return new MockShellAppViewGuestDelegate();
  }
};

}  // namespace

namespace extensions {

class AppViewTest : public AppShellTest,
                    public testing::WithParamInterface<bool> {
 protected:
  AppViewTest() { GuestViewManager::set_factory_for_testing(&factory_); }

  TestGuestViewManager* GetGuestViewManager() {
    return static_cast<TestGuestViewManager*>(
        TestGuestViewManager::FromBrowserContext(
            ShellContentBrowserClient::Get()->GetBrowserContext()));
  }

  content::WebContents* GetFirstAppWindowWebContents() {
    const AppWindowRegistry::AppWindowList& app_window_list =
        AppWindowRegistry::Get(browser_context_)->app_windows();
    DCHECK(app_window_list.size() == 1);
    return (*app_window_list.begin())->web_contents();
  }

  const Extension* LoadApp(const std::string& app_location) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath test_data_dir;
    base::PathService::Get(DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(app_location.c_str());
    return extension_system_->LoadApp(test_data_dir);
  }

  void RunTest(const std::string& test_name,
               const std::string& app_location,
               const std::string& app_to_embed) {
    const Extension* app_embedder = LoadApp(app_location);
    ASSERT_TRUE(app_embedder);
    const Extension* app_embedded = LoadApp(app_to_embed);
    ASSERT_TRUE(app_embedded);

    extension_system_->LaunchApp(app_embedder->id());

    ExtensionTestMessageListener launch_listener("LAUNCHED", false);
    ASSERT_TRUE(launch_listener.WaitUntilSatisfied());

    embedder_web_contents_ = GetFirstAppWindowWebContents();

    ExtensionTestMessageListener done_listener("TEST_PASSED", false);
    done_listener.set_failure_message("TEST_FAILED");
    ASSERT_TRUE(
        content::ExecuteScript(embedder_web_contents_,
                               base::StringPrintf("runTest('%s', '%s')",
                                                  test_name.c_str(),
                                                  app_embedded->id().c_str())))
        << "Unable to start test.";
    ASSERT_TRUE(done_listener.WaitUntilSatisfied());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    AppShellTest::SetUpCommandLine(command_line);
    // This switch ensures that there will always be at least one media device,
    // even on machines without physical devices. This is required by tests that
    // request permission to use media devices.
    command_line->AppendSwitch("use-fake-device-for-media-stream");

    bool use_cross_process_frames_for_guests = GetParam();
    if (use_cross_process_frames_for_guests) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kGuestViewCrossProcessFrames);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kGuestViewCrossProcessFrames);
    }
  }

  content::WebContents* embedder_web_contents_;
  TestGuestViewManagerFactory factory_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_CASE_P(AppViewTests, AppViewTest, testing::Bool());

#if defined(OS_WIN)
#define MAYBE_TestAppViewGoodDataShouldSucceed \
  DISABLED_TestAppViewGoodDataShouldSucceed
#else
#define MAYBE_TestAppViewGoodDataShouldSucceed TestAppViewGoodDataShouldSucceed
#endif
// Tests that <appview> correctly processes parameters passed on connect.
IN_PROC_BROWSER_TEST_P(AppViewTest, MAYBE_TestAppViewGoodDataShouldSucceed) {
  RunTest("testAppViewGoodDataShouldSucceed",
          "app_view/apitest",
          "app_view/apitest/skeleton");
}

// Tests that <appview> can handle media permission requests.
IN_PROC_BROWSER_TEST_P(AppViewTest, TestAppViewMediaRequest) {
  static_cast<ShellExtensionsBrowserClient*>(ExtensionsBrowserClient::Get())
      ->SetAPIClientForTest(nullptr);
  static_cast<ShellExtensionsBrowserClient*>(ExtensionsBrowserClient::Get())
      ->SetAPIClientForTest(new MockExtensionsAPIClient);

  RunTest("testAppViewMediaRequest", "app_view/apitest",
          "app_view/apitest/media_request");

  MockShellAppDelegate::Get()->WaitForRequestMediaPermission();
}

// Tests that <appview> correctly processes parameters passed on connect.
// This test should fail to connect because the embedded app (skeleton) will
// refuse the data passed by the embedder app and deny the request.
IN_PROC_BROWSER_TEST_P(AppViewTest, TestAppViewRefusedDataShouldFail) {
  RunTest("testAppViewRefusedDataShouldFail",
          "app_view/apitest",
          "app_view/apitest/skeleton");
}

#if defined(OS_WIN) || defined(OS_CHROMEOS)
#define MAYBE_TestAppViewWithUndefinedDataShouldSucceed \
  DISABLED_TestAppViewWithUndefinedDataShouldSucceed
#else
#define MAYBE_TestAppViewWithUndefinedDataShouldSucceed \
  TestAppViewWithUndefinedDataShouldSucceed
#endif
// Tests that <appview> is able to navigate to another installed app.
IN_PROC_BROWSER_TEST_P(AppViewTest,
                       MAYBE_TestAppViewWithUndefinedDataShouldSucceed) {
  RunTest("testAppViewWithUndefinedDataShouldSucceed",
          "app_view/apitest",
          "app_view/apitest/skeleton");
}

}  // namespace extensions
