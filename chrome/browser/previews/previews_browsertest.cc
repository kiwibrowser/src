// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/optimization_guide/optimization_guide_service.h"
#include "components/optimization_guide/test_component_creator.h"
#include "components/previews/core/previews_features.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

class PreviewsBrowserTest : public InProcessBrowserTest {
 public:
  PreviewsBrowserTest()
      : noscript_css_requested_(false), noscript_js_requested_(false) {}

  ~PreviewsBrowserTest() override {}

  void SetUpOnMainThread() override {
    noscript_css_requested_ = false;
    noscript_js_requested_ = false;

    // Set up https server with resource monitor.
    https_server_.reset(
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->ServeFilesFromSourceDirectory("chrome/test/data/previews");
    https_server_->RegisterRequestMonitor(base::BindRepeating(
        &PreviewsBrowserTest::MonitorResourceRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_->Start());

    https_url_ = https_server_->GetURL("/noscript_test.html");
    ASSERT_TRUE(https_url_.SchemeIs(url::kHttpsScheme));

    https_no_transform_url_ =
        https_server_->GetURL("/noscript_test_with_no_transform_header.html");
    ASSERT_TRUE(https_no_transform_url_.SchemeIs(url::kHttpsScheme));

    // Set up http server with resource monitor and redirect handler.
    http_server_.reset(
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTP));
    http_server_->ServeFilesFromSourceDirectory("chrome/test/data/previews");
    http_server_->RegisterRequestMonitor(base::BindRepeating(
        &PreviewsBrowserTest::MonitorResourceRequest, base::Unretained(this)));
    http_server_->RegisterRequestHandler(base::BindRepeating(
        &PreviewsBrowserTest::HandleRedirectRequest, base::Unretained(this)));
    ASSERT_TRUE(http_server_->Start());

    http_url_ = http_server_->GetURL("/noscript_test.html");
    ASSERT_TRUE(http_url_.SchemeIs(url::kHttpScheme));

    redirect_url_ = http_server_->GetURL("/redirect.html");
    ASSERT_TRUE(redirect_url_.SchemeIs(url::kHttpScheme));
  }

  void SetUpCommandLine(base::CommandLine* cmd) override {
    cmd->AppendSwitch("enable-spdy-proxy-auth");
    cmd->AppendSwitchASCII("force-effective-connection-type", "Slow-2G");
  }

  const GURL& https_url() const { return https_url_; }
  const GURL& https_no_transform_url() const { return https_no_transform_url_; }
  const GURL& http_url() const { return http_url_; }
  const GURL& redirect_url() const { return redirect_url_; }
  bool noscript_css_requested() const { return noscript_css_requested_; }
  bool noscript_js_requested() const { return noscript_js_requested_; }

 private:
  // Called by |https_server_|.
  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    if (request.GetURL().spec().find("noscript_test.css") !=
        std::string::npos) {
      noscript_css_requested_ = true;
    }
    if (request.GetURL().spec().find("noscript_test.js") != std::string::npos) {
      noscript_js_requested_ = true;
    }
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRedirectRequest(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> response;
    if (request.GetURL().spec().find("redirect") != std::string::npos) {
      response.reset(new net::test_server::BasicHttpResponse);
      response->set_code(net::HTTP_FOUND);
      response->AddCustomHeader("Location", https_url().spec());
    }
    return std::move(response);
  }

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<net::EmbeddedTestServer> http_server_;
  GURL https_url_;
  GURL https_no_transform_url_;
  GURL http_url_;
  GURL redirect_url_;
  bool noscript_css_requested_;
  bool noscript_js_requested_;
};

// Loads a webpage that has both script and noscript tags and also requests
// a script resource. Verifies that the noscript tag is not evaluated and the
// script resource is loaded.
IN_PROC_BROWSER_TEST_F(PreviewsBrowserTest, NoScriptPreviewsDisabled) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), https_url());

  // Verify loaded js resource but not css triggered by noscript tag.
  EXPECT_TRUE(noscript_js_requested());
  EXPECT_FALSE(noscript_css_requested());

  // Verify info bar not presented via histogram check.
  histogram_tester.ExpectTotalCount("Previews.InfoBarAction.NoScript", 0);
}

// This test class enables NoScriptPreviews but without OptimizationHints.
class PreviewsNoScriptBrowserTest : public PreviewsBrowserTest {
 public:
  PreviewsNoScriptBrowserTest() {}

  ~PreviewsNoScriptBrowserTest() override {}

  void SetUp() override {
    // Explicitly disable server hints.
    scoped_feature_list_.InitWithFeatures(
        {previews::features::kPreviews, previews::features::kNoScriptPreviews},
        {previews::features::kOptimizationHints});
    PreviewsBrowserTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Previews InfoBar (which these tests triggers) does not work on Mac.
// See crbug.com/782322 for detail.
// Also occasional flakes on win7 (crbug.com/789542) and Ubuntu 16.04
// (crbug.com/831838)
#if defined(OS_ANDROID)
#define MAYBE_NoScriptPreviewsEnabled NoScriptPreviewsEnabled
#define MAYBE_NoScriptPreviewsEnabledHttpRedirectToHttps \
  NoScriptPreviewsEnabledHttpRedirectToHttps
#else
#define MAYBE_NoScriptPreviewsEnabled DISABLED_NoScriptPreviewsEnabled
#define MAYBE_NoScriptPreviewsEnabledHttpRedirectToHttps \
  DISABLED_NoScriptPreviewsEnabledHttpRedirectToHttps
#endif

// Loads a webpage that has both script and noscript tags and also requests
// a script resource. Verifies that the noscript tag is evaluated and the
// script resource is not loaded.
IN_PROC_BROWSER_TEST_F(PreviewsNoScriptBrowserTest,
                       MAYBE_NoScriptPreviewsEnabled) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), https_url());

  // Verify loaded noscript tag triggered css resource but not js one.
  EXPECT_TRUE(noscript_css_requested());
  EXPECT_FALSE(noscript_js_requested());

  // Verify info bar presented via histogram check.
  histogram_tester.ExpectUniqueSample("Previews.InfoBarAction.NoScript", 0, 1);
}

IN_PROC_BROWSER_TEST_F(PreviewsNoScriptBrowserTest,
                       NoScriptPreviewsEnabledButHttpRequest) {
  ui_test_utils::NavigateToURL(browser(), http_url());

  // Verify loaded js resource but not css triggered by noscript tag.
  EXPECT_TRUE(noscript_js_requested());
  EXPECT_FALSE(noscript_css_requested());
}

// Flaky in all platforms except Android. See crbug.com/803626 for detail.
#if defined(OS_ANDROID)
#define MAYBE_NoScriptPreviewsEnabledButNoTransformDirective \
  NoScriptPreviewsEnabledButNoTransformDirective
#else
#define MAYBE_NoScriptPreviewsEnabledButNoTransformDirective \
  DISABLED_NoScriptPreviewsEnabledButNoTransformDirective
#endif
IN_PROC_BROWSER_TEST_F(PreviewsNoScriptBrowserTest,
                       MAYBE_NoScriptPreviewsEnabledButNoTransformDirective) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), https_no_transform_url());

  // Verify loaded js resource but not css triggered by noscript tag.
  EXPECT_TRUE(noscript_js_requested());
  EXPECT_FALSE(noscript_css_requested());

  histogram_tester.ExpectUniqueSample(
      "Previews.CacheControlNoTransform.BlockedPreview", 5 /* NoScript */, 1);
}

IN_PROC_BROWSER_TEST_F(PreviewsNoScriptBrowserTest,
                       MAYBE_NoScriptPreviewsEnabledHttpRedirectToHttps) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), redirect_url());

  // Verify loaded noscript tag triggered css resource but not js one.
  EXPECT_TRUE(noscript_css_requested());
  EXPECT_FALSE(noscript_js_requested());

  // Verify info bar presented via histogram check.
  histogram_tester.ExpectUniqueSample("Previews.InfoBarAction.NoScript", 0, 1);
}

// This test class enables NoScriptPreviews with OptimizationHints.
class PreviewsOptimizationGuideBrowserTest : public PreviewsBrowserTest {
 public:
  PreviewsOptimizationGuideBrowserTest() {}

  ~PreviewsOptimizationGuideBrowserTest() override {}

  void SetUp() override {
    scoped_feature_list_.InitWithFeatures(
        {previews::features::kPreviews, previews::features::kOptimizationHints,
         previews::features::kNoScriptPreviews},
        {});
    PreviewsBrowserTest::SetUp();
  }

  void SetNoScriptWhitelist(
      std::vector<std::string> whitelisted_noscript_sites) {
    const optimization_guide::ComponentInfo& component_info =
        test_component_creator_.CreateComponentInfoWithNoScriptWhitelist(
            whitelisted_noscript_sites);
    g_browser_process->optimization_guide_service()->ProcessHints(
        component_info);

    // Wait for hints to be processed by PreviewsOptimizationGuide.
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  optimization_guide::testing::TestComponentCreator test_component_creator_;
};

// Previews InfoBar (which this test triggers) does not work on Mac.
// See crbug.com/782322 for detail.
// Also occasional flakes on win7 (crbug.com/789948) and Ubuntu 16.04
// (crbug.com/831838)
#if defined(OS_ANDROID)
#define MAYBE_NoScriptPreviewsEnabledByWhitelist \
  NoScriptPreviewsEnabledByWhitelist
#else
#define MAYBE_NoScriptPreviewsEnabledByWhitelist \
  DISABLED_NoScriptPreviewsEnabledByWhitelist
#endif

IN_PROC_BROWSER_TEST_F(PreviewsOptimizationGuideBrowserTest,
                       MAYBE_NoScriptPreviewsEnabledByWhitelist) {
  // Whitelist test URL for NoScript.
  SetNoScriptWhitelist({https_url().host()});

  ui_test_utils::NavigateToURL(browser(), https_url());

  // Verify loaded noscript tag triggered css resource but not js one.
  EXPECT_TRUE(noscript_css_requested());
  EXPECT_FALSE(noscript_js_requested());
}

IN_PROC_BROWSER_TEST_F(PreviewsOptimizationGuideBrowserTest,
                       NoScriptPreviewsNotEnabledByWhitelist) {
  // Whitelist random site for NoScript.
  SetNoScriptWhitelist({"foo.com"});

  ui_test_utils::NavigateToURL(browser(), https_url());

  // Verify loaded js resource but not css triggered by noscript tag.
  EXPECT_TRUE(noscript_js_requested());
  EXPECT_FALSE(noscript_css_requested());
}
