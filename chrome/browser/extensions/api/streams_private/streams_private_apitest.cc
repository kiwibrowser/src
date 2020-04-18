// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/api/streams_private.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/download/public/common/download_item.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/download_test_observer.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "extensions/test/result_catcher.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/features.h"
#include "testing/gmock/include/gmock/gmock.h"

using content::BrowserContext;
using content::BrowserThread;
using content::DownloadManager;
using content::WebContents;
using download::DownloadItem;
using download::DownloadUrlParameters;
using extensions::Event;
using extensions::ExtensionSystem;
using extensions::ResultCatcher;
using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;
using testing::_;

namespace streams_private = extensions::api::streams_private;

namespace {

// Test server's request handler.
// Returns response that should be sent by the test server.
std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
  std::unique_ptr<BasicHttpResponse> response(new BasicHttpResponse());

  // For relative path "/doc_path.doc", return success response with MIME type
  // "application/msword".
  if (request.relative_url == "/doc_path.doc") {
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/msword");
    return std::move(response);
  }

  // For relative path "/spreadsheet_path.xls", return success response with
  // MIME type "application/xls".
  if (request.relative_url == "/spreadsheet_path.xls") {
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/msexcel");
    // Test that multiple headers with the same name are merged.
    response->AddCustomHeader("Test-Header", "part1");
    response->AddCustomHeader("Test-Header", "part2");
    return std::move(response);
  }

  // For relative path "/text_path_attch.txt", return success response with
  // MIME type "text/plain" and content "txt content". Also, set content
  // disposition to be attachment.
  if (request.relative_url == "/text_path_attch.txt") {
    response->set_code(net::HTTP_OK);
    response->set_content("txt content");
    response->set_content_type("text/plain");
    response->AddCustomHeader("Content-Disposition",
                              "attachment; filename=test_path.txt");
    return std::move(response);
  }

  // For relative path "/test_path_attch.txt", return success response with
  // MIME type "text/plain" and content "txt content".
  if (request.relative_url == "/text_path.txt") {
    response->set_code(net::HTTP_OK);
    response->set_content("txt content");
    response->set_content_type("text/plain");
    return std::move(response);
  }

  // A random HTML file to navigate to.
  if (request.relative_url == "/index.html") {
    response->set_code(net::HTTP_OK);
    response->set_content("html content");
    response->set_content_type("text/html");
    return std::move(response);
  }

  // RTF files for testing chrome.streamsPrivate.abort().
  if (request.relative_url == "/abort.rtf" ||
      request.relative_url == "/no_abort.rtf") {
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/rtf");
    return std::move(response);
  }

  // Respond to /favicon.ico for navigating to the page.
  if (request.relative_url == "/favicon.ico") {
    response->set_code(net::HTTP_NOT_FOUND);
    return std::move(response);
  }

  // No other requests should be handled in the tests.
  EXPECT_TRUE(false) << "NOTREACHED!";
  response->set_code(net::HTTP_NOT_FOUND);
  return std::move(response);
}

// Tests to verify that resources are correctly intercepted by
// StreamsResourceThrottle.
// The test extension expects the resources that should be handed to the
// extension to have MIME type 'application/msword' and the resources that
// should be downloaded by the browser to have MIME type 'text/plain'.
class StreamsPrivateApiTest : public extensions::ExtensionApiTest {
 public:
  StreamsPrivateApiTest() {}

  ~StreamsPrivateApiTest() override {}

  void SetUpOnMainThread() override {
    // Init test server.
    test_server_.reset(new net::EmbeddedTestServer);
    test_server_->RegisterRequestHandler(base::Bind(&HandleRequest));
    ASSERT_TRUE(test_server_->Start());
    host_resolver()->AddRule("*", "127.0.0.1");
    extensions::ExtensionApiTest::SetUpOnMainThread();
  }

  void TearDownOnMainThread() override {
    // Tear down the test server.
    EXPECT_TRUE(test_server_->ShutdownAndWaitUntilComplete());
    test_server_.reset();
    extensions::ExtensionApiTest::TearDownOnMainThread();
  }

  void InitializeDownloadSettings() {
    ASSERT_TRUE(browser());

    // Ensure there are no prompts for download during the test.
    browser()->profile()->GetPrefs()->SetBoolean(
        prefs::kPromptForDownload, false);

    DownloadManager* manager = GetDownloadManager();
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();
  }

  // Sends onExecuteContentHandler event with the MIME type "test/done" to the
  // test extension.
  // The test extension calls 'chrome.test.notifySuccess' when it receives the
  // event with the "test/done" MIME type (unless the 'chrome.test.notifyFail'
  // has already been called).
  void SendDoneEvent() {
    streams_private::StreamInfo info;
    info.mime_type = "test/done";
    info.original_url = "http://foo";
    info.stream_url = "blob://bar";
    info.tab_id = 10;
    info.expected_content_size = 20;

    std::unique_ptr<Event> event(new Event(
        extensions::events::STREAMS_PRIVATE_ON_EXECUTE_MIME_TYPE_HANDLER,
        streams_private::OnExecuteMimeTypeHandler::kEventName,
        streams_private::OnExecuteMimeTypeHandler::Create(info)));

    extensions::EventRouter::Get(browser()->profile())
        ->DispatchEventToExtension(test_extension_id_, std::move(event));
  }

  // Loads the test extension and set's up its file_browser_handler to handle
  // 'application/msword' and 'text/plain' MIME types.
  // The extension will notify success when it detects an event with the MIME
  // type 'application/msword' and notify fail when it detects an event with the
  // MIME type 'text/plain'.
  const extensions::Extension* LoadTestExtension() {
    // The test extension id is set by the key value in the manifest.
    test_extension_id_ = "oickdpebdnfbgkcaoklfcdhjniefkcji";

    const extensions::Extension* extension = LoadExtension(
        test_data_dir_.AppendASCII("streams_private/handle_mime_type"));
    if (!extension)
      return NULL;

    MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension);
    if (!handler) {
      message_ = "No mime type handlers defined.";
      return NULL;
    }

    DCHECK_EQ(test_extension_id_, extension->id());

    return extension;
  }

  // Returns the download manager for the current browser.
  DownloadManager* GetDownloadManager() const {
    DownloadManager* download_manager =
        BrowserContext::GetDownloadManager(browser()->profile());
    EXPECT_TRUE(download_manager);
    return download_manager;
  }

  // Deletes the download and waits until it's flushed.
  // The |manager| should have |download| in its list of downloads.
  void DeleteDownloadAndWaitForFlush(DownloadItem* download,
                                     DownloadManager* manager) {
    content::DownloadTestFlushObserver flush_observer(manager);
    download->Remove();
    flush_observer.WaitForFlush();
  }

 protected:
  std::string test_extension_id_;
  // The HTTP server used in the tests.
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
};

// Tests that navigating to a resource with a MIME type handleable by an
// installed, white-listed extension invokes the extension's
// onExecuteContentHandler event (and does not start a download).
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, Navigate) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/doc_path.doc"));

  // Wait for the response from the test server.
  base::RunLoop().RunUntilIdle();

  // There should be no downloads started by the navigation.
  DownloadManager* download_manager = GetDownloadManager();
  std::vector<DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(0u, downloads.size());

  // The test extension should receive onExecuteContentHandler event with MIME
  // type 'application/msword' (and call chrome.test.notifySuccess).
  EXPECT_TRUE(catcher.GetNextResult());
}

// Tests that navigating to a file URL also intercepts despite there being no
// HTTP headers. This is a regression test for https://crbug.com/416433.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, FileURL) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  ui_test_utils::NavigateToURL(browser(), ui_test_utils::GetTestUrl(
      base::FilePath(FILE_PATH_LITERAL("downloads")),
      base::FilePath(FILE_PATH_LITERAL("Picture_1.doc"))));

  // There should be no downloads started by the navigation.
  DownloadManager* download_manager = GetDownloadManager();
  std::vector<DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(0u, downloads.size());

  // The test extension should receive onExecuteContentHandler event with MIME
  // type 'application/msword' (and call chrome.test.notifySuccess).
  EXPECT_TRUE(catcher.GetNextResult());
}

// Tests that navigating cross-site to a resource with a MIME type handleable by
// an installed, white-listed extension invokes the extension's
// onExecuteContentHandler event (and does not start a download).
// Regression test for http://crbug.com/342999.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, NavigateCrossSite) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  // Navigate to a URL on a different hostname.
  static const char kInitialHost[] = "www.example.com";
  GURL::Replacements replacements;
  replacements.SetHostStr(kInitialHost);
  GURL initial_url =
      test_server_->GetURL("/index.html").ReplaceComponents(replacements);
  ui_test_utils::NavigateToURL(browser(), initial_url);

  // Now navigate to the doc file; the extension should pick it up normally.
  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/doc_path.doc"));

  // Wait for the response from the test server.
  base::RunLoop().RunUntilIdle();

  // There should be no downloads started by the navigation.
  DownloadManager* download_manager = GetDownloadManager();
  std::vector<DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(0u, downloads.size());

  // The test extension should receive onExecuteContentHandler event with MIME
  // type 'application/msword' (and call chrome.test.notifySuccess).
  EXPECT_TRUE(catcher.GetNextResult());
}

// Flaky on Linux and ChromeOS: http://crbug.com/746526.
#if defined(OS_LINUX)
#define MAYBE_NavigateToAnAttachment DISABLED_NavigateToAnAttachment
#else
#define MAYBE_NavigateToAnAttachment NavigateToAnAttachment
#endif

// Tests that navigation to an attachment starts a download, even if there is an
// extension with a file browser handler that can handle the attachment's MIME
// type.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, MAYBE_NavigateToAnAttachment) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  InitializeDownloadSettings();

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  // The test should start a download.
  DownloadManager* download_manager = GetDownloadManager();
  std::unique_ptr<content::DownloadTestObserver> download_observer(
      new content::DownloadTestObserverInProgress(download_manager, 1));

  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/text_path_attch.txt"));

  // Wait for the download to start.
  download_observer->WaitForFinished();

  // There should be one download started by the navigation.
  DownloadManager::DownloadVector downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(1u, downloads.size());

  // Cancel and delete the download started in the test.
  DeleteDownloadAndWaitForFlush(downloads[0], download_manager);

  // The test extension should not receive any events by now. Send it an event
  // with MIME type "test/done", so it stops waiting for the events. (If there
  // was an event with MIME type 'text/plain', |catcher.GetNextResult()| will
  // fail regardless of the sent event; chrome.test.notifySuccess will not be
  // called by the extension).
  SendDoneEvent();
  EXPECT_TRUE(catcher.GetNextResult());
}

// Flaky on Linux and Win 10: http://crbug.com/746526.
#if defined(OS_LINUX) || defined(OS_WIN)
#define MAYBE_DirectDownload DISABLED_DirectDownload
#else
#define MAYBE_DirectDownload DirectDownload
#endif

// Tests that direct download requests don't get intercepted by
// StreamsResourceThrottle, even if there is an extension with a file
// browser handler that can handle the download's MIME type.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, MAYBE_DirectDownload) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  InitializeDownloadSettings();

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  DownloadManager* download_manager = GetDownloadManager();
  std::unique_ptr<content::DownloadTestObserver> download_observer(
      new content::DownloadTestObserverInProgress(download_manager, 1));

  // The resource's URL on the test server.
  GURL url = test_server_->GetURL("/text_path.txt");

  // The download's target file path.
  base::FilePath target_path =
      DownloadPrefs(browser()->profile())
          .DownloadPath()
          .Append(FILE_PATH_LITERAL("download_target.txt"));

  // Set the downloads parameters.
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  std::unique_ptr<DownloadUrlParameters> params(
      content::DownloadRequestUtils::CreateDownloadForWebContentsMainFrame(
          web_contents, url, TRAFFIC_ANNOTATION_FOR_TESTS));

  params->set_file_path(target_path);

  // Start download of the URL with a path "/text_path.txt" on the test server.
  download_manager->DownloadUrl(std::move(params));

  // Wait for the download to start.
  download_observer->WaitForFinished();

  // There should have been one download.
  std::vector<DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(1u, downloads.size());

  // Cancel and delete the download statred in the test.
  DeleteDownloadAndWaitForFlush(downloads[0], download_manager);

  // The test extension should not receive any events by now. Send it an event
  // with MIME type "test/done", so it stops waiting for the events. (If there
  // was an event with MIME type 'text/plain', |catcher.GetNextResult()| will
  // fail regardless of the sent event; chrome.test.notifySuccess will not be
  // called by the extension).
  SendDoneEvent();
  EXPECT_TRUE(catcher.GetNextResult());
}

// Tests that response headers are correctly passed to the API and that multiple
// repsonse headers with the same name are merged correctly.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, Headers) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;

  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/spreadsheet_path.xls"));

  // Wait for the response from the test server.
  base::RunLoop().RunUntilIdle();

  // There should be no downloads started by the navigation.
  DownloadManager* download_manager = GetDownloadManager();
  std::vector<DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  ASSERT_EQ(0u, downloads.size());

  // The test extension should receive onExecuteContentHandler event with MIME
  // type 'application/msexcel' (and call chrome.test.notifySuccess).
  EXPECT_TRUE(catcher.GetNextResult());
}

// Tests that chrome.streamsPrivate.abort() works correctly.
IN_PROC_BROWSER_TEST_F(StreamsPrivateApiTest, Abort) {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;  // Streams not used with network service.

  ASSERT_TRUE(LoadTestExtension()) << message_;

  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/no_abort.rtf"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(catcher.GetNextResult());

  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/abort.rtf"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(catcher.GetNextResult());
}

}  // namespace
