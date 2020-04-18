// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted_memory.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/features.h"

namespace content {

namespace {

class RenderProcessKilledObserver : public WebContentsObserver {
 public:
  explicit RenderProcessKilledObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}
  ~RenderProcessKilledObserver() override {}

  bool killed() const { return killed_; }

  void RenderProcessGone(base::TerminationStatus status) override {
    killed_ = true;
    run_loop_.Quit();
  }

  void WaitUntilRenderProcessDied() {
    if (killed_)
      return;
    run_loop_.Run();
  }

 private:
  bool killed_ = false;

  // Used to wait for the render process being killed. Android doesn't
  // immediately kill the render process.
  base::RunLoop run_loop_;
};

class WebUITestWebUIControllerFactory : public WebUIControllerFactory {
 public:
  WebUIController* CreateWebUIControllerForURL(WebUI* web_ui,
                                               const GURL& url) const override {
    std::string foo(url.path());
    if (url.path() == "/nobinding/")
      web_ui->SetBindings(0);
    return HasWebUIScheme(url) ? new WebUIController(web_ui) : nullptr;
  }
  WebUI::TypeID GetWebUIType(BrowserContext* browser_context,
                             const GURL& url) const override {
    return HasWebUIScheme(url) ? reinterpret_cast<WebUI::TypeID>(1) : nullptr;
  }
  bool UseWebUIForURL(BrowserContext* browser_context,
                      const GURL& url) const override {
    return HasWebUIScheme(url);
  }
  bool UseWebUIBindingsForURL(BrowserContext* browser_context,
                              const GURL& url) const override {
    return HasWebUIScheme(url);
  }
};

class WebUIDataSource : public URLDataSource {
 public:
  WebUIDataSource() {}

 private:
  ~WebUIDataSource() override {}

  std::string GetSource() const override { return "webui"; }

  void StartDataRequest(
      const std::string& path,
      const ResourceRequestInfo::WebContentsGetter& wc_getter,
      const URLDataSource::GotDataCallback& callback) override {
    std::string dummy_html = "<html><body>Foo</body></html>";
    scoped_refptr<base::RefCountedString> response =
        base::RefCountedString::TakeString(&dummy_html);
    callback.Run(response.get());
  }

  std::string GetMimeType(const std::string& path) const override {
    return "text/html";
  }

  DISALLOW_COPY_AND_ASSIGN(WebUIDataSource);
};

class NetworkServiceBrowserTest : public ContentBrowserTest {
 public:
  NetworkServiceBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        network::features::kNetworkService);
    EXPECT_TRUE(embedded_test_server()->Start());

    WebUIControllerFactory::RegisterFactory(&factory_);
  }

  bool ExecuteScript(const std::string& script) {
    bool xhr_result = false;
    // The JS call will fail if disallowed because the process will be killed.
    bool execute_result =
        ExecuteScriptAndExtractBool(shell(), script, &xhr_result);
    return xhr_result && execute_result;
  }

  bool FetchResource(const GURL& url) {
    if (!url.is_valid())
      return false;
    std::string script(
        "var xhr = new XMLHttpRequest();"
        "xhr.open('GET', '");
    script += url.spec() +
              "', true);"
              "xhr.onload = function (e) {"
              "  if (xhr.readyState === 4) {"
              "    window.domAutomationController.send(xhr.status === 200);"
              "  }"
              "};"
              "xhr.onerror = function () {"
              "  window.domAutomationController.send(false);"
              "};"
              "xhr.send(null)";
    return ExecuteScript(script);
  }

  bool CheckCanLoadHttp() {
    return FetchResource(embedded_test_server()->GetURL("/echo"));
  }

  void SetUpOnMainThread() override {
    URLDataSource::Add(shell()->web_contents()->GetBrowserContext(),
                       new WebUIDataSource);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Since we assume exploited renderer process, it can bypass the same origin
    // policy at will. Simulate that by passing the disable-web-security flag.
    command_line->AppendSwitch(switches::kDisableWebSecurity);
    IsolateAllSitesForTesting(command_line);
  }

 private:
  WebUITestWebUIControllerFactory factory_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceBrowserTest);
};

// Verifies that WebUI pages with WebUI bindings can't make network requests.
IN_PROC_BROWSER_TEST_F(NetworkServiceBrowserTest, WebUIBindingsNoHttp) {
  GURL test_url("chrome://webui/");
  NavigateToURL(shell(), test_url);
  RenderProcessKilledObserver killed_observer(shell()->web_contents());
  ASSERT_FALSE(CheckCanLoadHttp());
  killed_observer.WaitUntilRenderProcessDied();
  ASSERT_TRUE(killed_observer.killed());
}

// Verifies that WebUI pages without WebUI bindings can make network requests.
IN_PROC_BROWSER_TEST_F(NetworkServiceBrowserTest, NoWebUIBindingsHttp) {
  GURL test_url("chrome://webui/nobinding/");
  NavigateToURL(shell(), test_url);
  ASSERT_TRUE(CheckCanLoadHttp());
}

// Verifies the filesystem URLLoaderFactory's check, using
// ChildProcessSecurityPolicyImpl::CanRequestURL is properly rejected.
IN_PROC_BROWSER_TEST_F(NetworkServiceBrowserTest,
                       FileSystemBindingsCorrectOrigin) {
  GURL test_url("chrome://webui/nobinding/");
  NavigateToURL(shell(), test_url);

  // Note: must be filesystem scheme (obviously).
  //       file: is not a safe web scheme (see IsWebSafeScheme),
  //       and /etc/passwd fails the CanCommitURL check.
  GURL file_url("filesystem:file:///etc/passwd");
  EXPECT_FALSE(FetchResource(file_url));
}

class NetworkServiceInProcessBrowserTest : public ContentBrowserTest {
 public:
  NetworkServiceInProcessBrowserTest() {
    std::vector<base::Feature> features;
    features.push_back(network::features::kNetworkService);
    features.push_back(features::kNetworkServiceInProcess);
    scoped_feature_list_.InitWithFeatures(features,
                                          std::vector<base::Feature>());
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    EXPECT_TRUE(embedded_test_server()->Start());
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceInProcessBrowserTest);
};

// Verifies that in-process network service works.
IN_PROC_BROWSER_TEST_F(NetworkServiceInProcessBrowserTest, Basic) {
  GURL test_url = embedded_test_server()->GetURL("foo.com", "/echo");
  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(
          shell()->web_contents()->GetBrowserContext()));
  NavigateToURL(shell(), test_url);
  ASSERT_EQ(net::OK,
            LoadBasicRequest(partition->GetNetworkContext(), test_url));
}

}  // namespace

}  // namespace content
