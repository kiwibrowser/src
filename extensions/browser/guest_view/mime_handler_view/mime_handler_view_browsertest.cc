// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/guest_view/browser/test_guest_view_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/browser/guest_view/mime_handler_view/test_mime_handler_view_guest.h"
#include "extensions/browser/process_manager.h"
#include "extensions/test/result_catcher.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"

using extensions::ExtensionsAPIClient;
using extensions::MimeHandlerViewGuest;
using extensions::TestMimeHandlerViewGuest;
using guest_view::GuestViewManager;
using guest_view::GuestViewManagerDelegate;
using guest_view::TestGuestViewManager;
using guest_view::TestGuestViewManagerFactory;

// The test extension id is set by the key value in the manifest.
const char kExtensionId[] = "oickdpebdnfbgkcaoklfcdhjniefkcji";

class MimeHandlerViewTest : public extensions::ExtensionApiTest,
                            public testing::WithParamInterface<bool> {
 public:
  MimeHandlerViewTest() {
    GuestViewManager::set_factory_for_testing(&factory_);
  }

  ~MimeHandlerViewTest() override {}

  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromDirectory(
        test_data_dir_.AppendASCII("mime_handler_view"));
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &MimeHandlerViewTest::MonitorRequest, base::Unretained(this)));
    ASSERT_TRUE(StartEmbeddedTestServer());
  }

  // TODO(ekaramad): These tests run for OOPIF guests too, except that they
  // still use BrowserPlugin code path. They are activated to make sure we can
  // still show PDF when the rest of the guests migrate to OOPIF. Eventually,
  // MimeHandlerViewGuest will be based on OOPIF and we can remove this comment
  // (https://crbug.com/642826).
  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);

    bool use_cross_process_frames_for_guests = GetParam();
    if (use_cross_process_frames_for_guests) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kGuestViewCrossProcessFrames);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kGuestViewCrossProcessFrames);
    }
  }

  // TODO(paulmeyer): This function is implemented over and over by the
  // different GuestView test classes. It really needs to be refactored out to
  // some kind of GuestViewTest base class.
  TestGuestViewManager* GetGuestViewManager() {
    TestGuestViewManager* manager = static_cast<TestGuestViewManager*>(
        TestGuestViewManager::FromBrowserContext(browser()->profile()));
    // TestGuestViewManager::WaitForSingleGuestCreated can and will get called
    // before a guest is created. Since GuestViewManager is usually not created
    // until the first guest is created, this means that |manager| will be
    // nullptr if trying to use the manager to wait for the first guest. Because
    // of this, the manager must be created here if it does not already exist.
    if (!manager) {
      manager = static_cast<TestGuestViewManager*>(
          GuestViewManager::CreateWithDelegate(
              browser()->profile(),
              ExtensionsAPIClient::Get()->CreateGuestViewManagerDelegate(
                  browser()->profile())));
    }
    return manager;
  }

  const extensions::Extension* LoadTestExtension() {
    const extensions::Extension* extension =
        LoadExtension(test_data_dir_.AppendASCII("mime_handler_view"));
    if (!extension)
      return nullptr;

    CHECK_EQ(std::string(kExtensionId), extension->id());

    return extension;
  }

  void RunTestWithUrl(const GURL& url) {
    // Use the testing subclass of MimeHandlerViewGuest.
    GetGuestViewManager()->RegisterTestGuestViewType<MimeHandlerViewGuest>(
        base::Bind(&TestMimeHandlerViewGuest::Create));

    const extensions::Extension* extension = LoadTestExtension();
    ASSERT_TRUE(extension);

    extensions::ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(), url);

    if (!catcher.GetNextResult())
      FAIL() << catcher.message();
  }

  void RunTest(const std::string& path) {
    RunTestWithUrl(embedded_test_server()->GetURL("/" + path));
  }

  int basic_count() const { return basic_count_; }

 private:
  void MonitorRequest(const net::test_server::HttpRequest& request) {
    if (request.relative_url == "/testBasic.csv")
      basic_count_++;
  }

  TestGuestViewManagerFactory factory_;
  base::test::ScopedFeatureList scoped_feature_list_;
  int basic_count_ = 0;
};

INSTANTIATE_TEST_CASE_P(MimeHandlerViewTests,
                        MimeHandlerViewTest,
                        testing::Bool());

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, PostMessage) {
  RunTest("test_postmessage.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, Basic) {
  RunTest("testBasic.csv");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, Embedded) {
  RunTest("test_embedded.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, Iframe) {
  RunTest("test_iframe.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, Abort) {
  RunTest("testAbort.csv");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, NonAsciiHeaders) {
  RunTest("testNonAsciiHeaders.csv");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, DataUrl) {
  const char* kDataUrlCsv = "data:text/csv;base64,Y29udGVudCB0byByZWFkCg==";
  RunTestWithUrl(GURL(kDataUrlCsv));
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, EmbeddedDataUrlObject) {
  RunTest("test_embedded_data_url_object.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, EmbeddedDataUrlEmbed) {
  RunTest("test_embedded_data_url_embed.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, EmbeddedDataUrlLong) {
  RunTest("test_embedded_data_url_long.html");
}

IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, ResizeBeforeAttach) {
  // Delay the creation of the guest's WebContents in order to delay the guest's
  // attachment to the embedder. This will allow us to resize the <object> tag
  // after the guest is created, but before it is attached in
  // "test_resize_before_attach.html".
  TestMimeHandlerViewGuest::DelayNextCreateWebContents(500);
  RunTest("test_resize_before_attach.html");

  // Wait for the guest to attach.
  content::WebContents* guest_web_contents =
      GetGuestViewManager()->WaitForSingleGuestCreated();
  TestMimeHandlerViewGuest* guest = static_cast<TestMimeHandlerViewGuest*>(
      MimeHandlerViewGuest::FromWebContents(guest_web_contents));
  guest->WaitForGuestAttached();

  // Ensure that the guest has the correct size after it has attached.
  auto guest_size = guest->size();
  CHECK_EQ(guest_size.width(), 500);
  CHECK_EQ(guest_size.height(), 400);
}

// Regression test for crbug.com/587709.
IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, SingleRequest) {
  GURL url(embedded_test_server()->GetURL("/testBasic.csv"));
  RunTest("testBasic.csv");
  EXPECT_EQ(1, basic_count());
}

// Test that a mime handler view can keep a background page alive.
IN_PROC_BROWSER_TEST_P(MimeHandlerViewTest, BackgroundPage) {
  extensions::ProcessManager::SetEventPageIdleTimeForTesting(1);
  extensions::ProcessManager::SetEventPageSuspendingTimeForTesting(1);
  RunTest("testBackgroundPage.csv");
}
