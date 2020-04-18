// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/test/bind_test_util.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/http_request.h"
#include "ppapi/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/test/ppapi/ppapi_test.h"
#endif

namespace content {
namespace {

class AcceptHeaderTest : public ContentBrowserTest {
 public:
  AcceptHeaderTest() {}

  void SetUpOnMainThread() override {
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &AcceptHeaderTest::Monitor, base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  std::string GetFor(const std::string& path) {
    {
      base::AutoLock auto_lock(map_lock_);
      auto it = url_accept_header_.find(path);
      if (it != url_accept_header_.end())
        return it->second;
    }

    {
      base::AutoLock auto_lock(waiting_lock_);
      waiting_for_path_ = path;
      waiting_run_loop_ = std::make_unique<base::RunLoop>(
          base::RunLoop::Type::kNestableTasksAllowed);
    }

    waiting_run_loop_->Run();

    {
      base::AutoLock auto_lock(waiting_lock_);
      waiting_for_path_ = std::string();
      waiting_run_loop_.reset();
    }

    auto it = url_accept_header_.find(path);
    return it->second;
  }

 private:
  void Monitor(const net::test_server::HttpRequest& request) {
    auto it = request.headers.find("Accept");
    if (it == request.headers.end())
      return;

    {
      base::AutoLock auto_lock(map_lock_);
      url_accept_header_[request.relative_url] = it->second;
    }

    {
      base::AutoLock auto_lock(waiting_lock_);
      if (request.relative_url == waiting_for_path_)
        waiting_run_loop_->Quit();
    }
  }

  base::Lock map_lock_;
  std::map<std::string, std::string> url_accept_header_;

  base::Lock waiting_lock_;
  std::unique_ptr<base::RunLoop> waiting_run_loop_;
  std::string waiting_for_path_;

  DISALLOW_COPY_AND_ASSIGN(AcceptHeaderTest);
};

IN_PROC_BROWSER_TEST_F(AcceptHeaderTest, Check) {
  NavigateToURL(shell(), embedded_test_server()->GetURL("/accept-header.html"));

  // RESOURCE_TYPE_MAIN_FRAME
  EXPECT_EQ(
      "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,"
      "image/apng,*/*;q=0.8",
      GetFor("/accept-header.html"));

  // RESOURCE_TYPE_SUB_FRAME
  EXPECT_EQ(
      "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,"
      "image/apng,*/*;q=0.8",
      GetFor("/iframe.html"));

  // RESOURCE_TYPE_STYLESHEET
  EXPECT_EQ("text/css,*/*;q=0.1", GetFor("/test.css"));

  // RESOURCE_TYPE_SCRIPT
  EXPECT_EQ("*/*", GetFor("/test.js"));

  // RESOURCE_TYPE_IMAGE
  EXPECT_EQ("image/webp,image/apng,image/*,*/*;q=0.8", GetFor("/image.gif"));

  // RESOURCE_TYPE_FONT_RESOURCE
  EXPECT_EQ("*/*", GetFor("/test.js"));

  // RESOURCE_TYPE_MEDIA
  EXPECT_EQ("*/*", GetFor("/media.mp4"));

  // RESOURCE_TYPE_WORKER
  EXPECT_EQ("*/*", GetFor("/worker.js"));

// Shared workers aren't implemented on Android.
// https://bugs.chromium.org/p/chromium/issues/detail?id=154571
#if !defined(OS_ANDROID)
  // RESOURCE_TYPE_SHARED_WORKER
  EXPECT_EQ("*/*", GetFor("/shared_worker.js"));
#endif

  // RESOURCE_TYPE_PREFETCH
  EXPECT_EQ("*/*", GetFor("/prefetch"));

  // RESOURCE_TYPE_XHR
  EXPECT_EQ("*/*", GetFor("/xhr"));

  // RESOURCE_TYPE_PING
  EXPECT_EQ("*/*", GetFor("/ping"));

  // RESOURCE_TYPE_SERVICE_WORKER
  EXPECT_EQ("*/*", GetFor("/service_worker.js"));

  // RESOURCE_TYPE_CSP_REPORT
  EXPECT_EQ("*/*", GetFor("/csp"));

  // Ensure that if an Accept header is already set, it is not overwritten.
  EXPECT_EQ("custom/type", GetFor("/xhr_with_accept_header"));

  shell()->web_contents()->GetManifest(
      base::BindOnce([](const GURL&, const blink::Manifest&) {}));

  // RESOURCE_TYPE_SUB_RESOURCE
  EXPECT_EQ("*/*", GetFor("/manifest"));

  // RESOURCE_TYPE_OBJECT and RESOURCE_TYPE_FAVICON are tested in src/chrome's
  // ChromeAcceptHeaderTest.ObjectAndFavicon.
}

#if BUILDFLAG(ENABLE_PLUGINS)

// Checks Accept header for RESOURCE_TYPE_PLUGIN_RESOURCE.
IN_PROC_BROWSER_TEST_F(OutOfProcessPPAPITest, PluginAcceptHeader) {
  net::EmbeddedTestServer server(net::EmbeddedTestServer::TYPE_HTTP);
  server.ServeFilesFromSourceDirectory("ppapi/tests");
  std::string plugin_accept_header;
  server.RegisterRequestMonitor(base::BindLambdaForTesting(
      [&](const net::test_server::HttpRequest& request) {
        if (request.relative_url == "/test_url_loader_data/hello.txt") {
          auto it = request.headers.find("Accept");
          if (it != request.headers.end())
            plugin_accept_header = it->second;
        }
      }));
  ASSERT_TRUE(server.Start());

  RunTestURL(
      server.GetURL(BuildQuery("/test_case.html?", "URLLoader_BasicGET")));

  ASSERT_EQ("*/*", plugin_accept_header);

  // Since the server uses local variables.
  ASSERT_TRUE(server.ShutdownAndWaitUntilComplete());
}

#endif  // BUILDFLAG(ENABLE_PLUGINS)
}  //  namespace
}  //  namespace content
