// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/url_loader_interceptor.h"
#include "base/command_line.h"
#include "base/single_thread_task_runner.h"
#include "base/test/bind_test_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/features.h"
#include "services/network/test/test_url_loader_client.h"

namespace content {
namespace {

class URLLoaderInterceptorTest : public ContentBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void Test() { EXPECT_TRUE(NavigateToURL(shell(), GetPageURL())); }

  GURL GetPageURL() {
    return embedded_test_server()->GetURL("/page_with_image.html");
  }

  bool DidImageLoad() {
    int height = 0;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(
        shell(),
        "window.domAutomationController.send("
        "document.getElementsByTagName('img')[0].naturalHeight)",
        &height));
    return !!height;
  }

  GURL GetImageURL() { return embedded_test_server()->GetURL("/blank.jpg"); }
};

IN_PROC_BROWSER_TEST_F(URLLoaderInterceptorTest, MonitorFrame) {
  bool seen = false;
  GURL url = GetPageURL();
  URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](URLLoaderInterceptor::RequestParams* params) {
        if (params->url_request.url == url) {
          EXPECT_EQ(params->process_id, 0);
          EXPECT_FALSE(seen);
          seen = true;
        }
        return false;
      }));
  Test();
  EXPECT_TRUE(seen);
}

IN_PROC_BROWSER_TEST_F(URLLoaderInterceptorTest, InterceptFrame) {
  GURL url = GetPageURL();
  URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](URLLoaderInterceptor::RequestParams* params) {
        EXPECT_EQ(params->url_request.url, url);
        EXPECT_EQ(params->process_id, 0);
        network::URLLoaderCompletionStatus status;
        status.error_code = net::ERR_FAILED;
        params->client->OnComplete(status);
        return true;
      }));
  EXPECT_FALSE(NavigateToURL(shell(), GetPageURL()));
}

IN_PROC_BROWSER_TEST_F(URLLoaderInterceptorTest, MonitorSubresource) {
  bool seen = false;
  GURL url = GetImageURL();
  URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](URLLoaderInterceptor::RequestParams* params) {
        if (params->url_request.url == url) {
          EXPECT_NE(params->process_id, 0);
          EXPECT_FALSE(seen);
          seen = true;
        }
        return false;
      }));
  Test();
  EXPECT_TRUE(seen);
  EXPECT_TRUE(DidImageLoad());
}

IN_PROC_BROWSER_TEST_F(URLLoaderInterceptorTest, InterceptSubresource) {
  GURL url = GetImageURL();
  URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](URLLoaderInterceptor::RequestParams* params) {
        if (params->url_request.url == url) {
          network::URLLoaderCompletionStatus status;
          status.error_code = net::ERR_FAILED;
          params->client->OnComplete(status);
          return true;
        }
        return false;
      }));
  Test();
  EXPECT_FALSE(DidImageLoad());
}

IN_PROC_BROWSER_TEST_F(URLLoaderInterceptorTest, InterceptBrowser) {
  GURL url = GetImageURL();
  network::mojom::URLLoaderPtr loader;
  network::TestURLLoaderClient client;
  network::ResourceRequest request;
  request.url = url;

  URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](URLLoaderInterceptor::RequestParams* params) {
        EXPECT_EQ(params->url_request.url, url);
        network::URLLoaderCompletionStatus status;
        status.error_code = net::ERR_FAILED;
        params->client->OnComplete(status);
        return true;
      }));
  auto* factory = BrowserContext::GetDefaultStoragePartition(
                      shell()->web_contents()->GetBrowserContext())
                      ->GetURLLoaderFactoryForBrowserProcess()
                      .get();
  factory->CreateLoaderAndStart(
      mojo::MakeRequest(&loader), 0, 0, 0, request, client.CreateInterfacePtr(),
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  client.RunUntilComplete();
  EXPECT_EQ(net::ERR_FAILED, client.completion_status().error_code);
}

}  // namespace
}  // namespace content
