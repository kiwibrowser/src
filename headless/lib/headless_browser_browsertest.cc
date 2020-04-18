// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "headless/lib/headless_macros.h"
#include "headless/public/devtools/domains/inspector.h"
#include "headless/public/devtools/domains/network.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/tracing.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_devtools_target.h"
#include "headless/public/headless_web_contents.h"
#include "headless/test/headless_browser_test.h"
#include "headless/test/test_protocol_handler.h"
#include "headless/test/test_url_request_job.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_util.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/url_request/url_request_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/geometry/size.h"

using testing::UnorderedElementsAre;

namespace headless {

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, CreateAndDestroyBrowserContext) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));

  browser_context->Close();

  EXPECT_TRUE(browser()->GetAllBrowserContexts().empty());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest,
                       CreateAndDoNotDestroyBrowserContext) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));

  // We check that HeadlessBrowser correctly handles non-closed BrowserContexts.
  // We can rely on Chromium DCHECKs to capture this.
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, CreateAndDestroyWebContents) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(web_contents);

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));
  EXPECT_THAT(browser_context->GetAllWebContents(),
              UnorderedElementsAre(web_contents));

  // TODO(skyostil): Verify viewport dimensions once we can.

  web_contents->Close();

  EXPECT_TRUE(browser_context->GetAllWebContents().empty());

  browser_context->Close();

  EXPECT_TRUE(browser()->GetAllBrowserContexts().empty());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest,
                       WebContentsAreDestroyedWithContext) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(web_contents);

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));
  EXPECT_THAT(browser_context->GetAllWebContents(),
              UnorderedElementsAre(web_contents));

  browser_context->Close();

  EXPECT_TRUE(browser()->GetAllBrowserContexts().empty());

  // If WebContents are not destroyed, Chromium DCHECKs will capture this.
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, CreateAndDoNotDestroyWebContents) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(web_contents);

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));
  EXPECT_THAT(browser_context->GetAllWebContents(),
              UnorderedElementsAre(web_contents));

  // If WebContents are not destroyed, Chromium DCHECKs will capture this.
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, DestroyAndCreateTwoWebContents) {
  HeadlessBrowserContext* browser_context1 =
      browser()->CreateBrowserContextBuilder().Build();
  EXPECT_TRUE(browser_context1);
  HeadlessWebContents* web_contents1 =
      browser_context1->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(web_contents1);

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context1));
  EXPECT_THAT(browser_context1->GetAllWebContents(),
              UnorderedElementsAre(web_contents1));

  HeadlessBrowserContext* browser_context2 =
      browser()->CreateBrowserContextBuilder().Build();
  EXPECT_TRUE(browser_context2);
  HeadlessWebContents* web_contents2 =
      browser_context2->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(web_contents2);

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context1, browser_context2));
  EXPECT_THAT(browser_context1->GetAllWebContents(),
              UnorderedElementsAre(web_contents1));
  EXPECT_THAT(browser_context2->GetAllWebContents(),
              UnorderedElementsAre(web_contents2));

  browser_context1->Close();

  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context2));
  EXPECT_THAT(browser_context2->GetAllWebContents(),
              UnorderedElementsAre(web_contents2));

  browser_context2->Close();

  EXPECT_TRUE(browser()->GetAllBrowserContexts().empty());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, CreateWithBadURL) {
  GURL bad_url("not_valid");

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(bad_url)
          .Build();

  EXPECT_FALSE(web_contents);
  EXPECT_TRUE(browser_context->GetAllWebContents().empty());
}

class HeadlessBrowserTestWithProxy : public HeadlessBrowserTest {
 public:
  HeadlessBrowserTestWithProxy()
      : proxy_server_(net::SpawnedTestServer::TYPE_HTTP,
                      base::FilePath(FILE_PATH_LITERAL("headless/test/data"))) {
  }

  void SetUp() override {
    ASSERT_TRUE(proxy_server_.Start());
    HeadlessBrowserTest::SetUp();
  }

  void TearDown() override {
    proxy_server_.Stop();
    HeadlessBrowserTest::TearDown();
  }

  net::SpawnedTestServer* proxy_server() { return &proxy_server_; }

 private:
  net::SpawnedTestServer proxy_server_;
};

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTestWithProxy, SetProxyConfig) {
  std::unique_ptr<net::ProxyConfig> proxy_config(new net::ProxyConfig);
  proxy_config->proxy_rules().ParseFromString(
      proxy_server()->host_port_pair().ToString());
  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetProxyConfig(std::move(proxy_config))
          .Build();

  // Load a page which doesn't actually exist, but for which the our proxy
  // returns valid content anyway.
  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("http://not-an-actual-domain.tld/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));
  EXPECT_THAT(browser()->GetAllBrowserContexts(),
              UnorderedElementsAre(browser_context));
  EXPECT_THAT(browser_context->GetAllWebContents(),
              UnorderedElementsAre(web_contents));
  web_contents->Close();
  EXPECT_TRUE(browser_context->GetAllWebContents().empty());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, SetHostResolverRules) {
  EXPECT_TRUE(embedded_test_server()->Start());

  std::string host_resolver_rules =
      base::StringPrintf("MAP not-an-actual-domain.tld 127.0.0.1:%d",
                         embedded_test_server()->host_port_pair().port());

  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetHostResolverRules(host_resolver_rules)
          .Build();

  // Load a page which doesn't actually exist, but which is turned into a valid
  // address by our host resolver rules.
  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("http://not-an-actual-domain.tld/hello.html"))
          .Build();
  EXPECT_TRUE(web_contents);

  EXPECT_TRUE(WaitForLoad(web_contents));
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, HttpProtocolHandler) {
  const std::string kResponseBody = "<p>HTTP response body</p>";
  ProtocolHandlerMap protocol_handlers;
  protocol_handlers[url::kHttpScheme] =
      std::make_unique<TestProtocolHandler>(kResponseBody);

  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetProtocolHandlers(std::move(protocol_handlers))
          .Build();

  // Load a page which doesn't actually exist, but which is fetched by our
  // custom protocol handler.
  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("http://not-an-actual-domain.tld/hello.html"))
          .Build();
  EXPECT_TRUE(web_contents);
  EXPECT_TRUE(WaitForLoad(web_contents));

  EXPECT_EQ(kResponseBody,
            EvaluateScript(web_contents, "document.body.innerHTML")
                ->GetResult()
                ->GetValue()
                ->GetString());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, HttpsProtocolHandler) {
  const std::string kResponseBody = "<p>HTTPS response body</p>";
  ProtocolHandlerMap protocol_handlers;
  protocol_handlers[url::kHttpsScheme] =
      std::make_unique<TestProtocolHandler>(kResponseBody);

  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetProtocolHandlers(std::move(protocol_handlers))
          .Build();

  // Load a page which doesn't actually exist, but which is fetched by our
  // custom protocol handler.
  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("https://not-an-actual-domain.tld/hello.html"))
          .Build();
  EXPECT_TRUE(web_contents);
  EXPECT_TRUE(WaitForLoad(web_contents));

  std::string inner_html;
  EXPECT_EQ(kResponseBody,
            EvaluateScript(web_contents, "document.body.innerHTML")
                ->GetResult()
                ->GetValue()
                ->GetString());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, WebGLSupported) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder().Build();

  EXPECT_TRUE(
      EvaluateScript(web_contents,
                     "(document.createElement('canvas').getContext('webgl')"
                     "    instanceof WebGLRenderingContext)")
          ->GetResult()
          ->GetValue()
          ->GetBool());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, ClipboardCopyPasteText) {
  // Tests copy-pasting text with the clipboard in headless mode.
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ASSERT_TRUE(clipboard);
  base::string16 paste_text = base::ASCIIToUTF16("Clippy!");
  for (ui::ClipboardType type :
       {ui::CLIPBOARD_TYPE_COPY_PASTE, ui::CLIPBOARD_TYPE_SELECTION,
        ui::CLIPBOARD_TYPE_DRAG}) {
    if (!ui::Clipboard::IsSupportedClipboardType(type))
      continue;
    {
      ui::ScopedClipboardWriter writer(type);
      writer.WriteText(paste_text);
    }
    base::string16 copy_text;
    clipboard->ReadText(type, &copy_text);
    EXPECT_EQ(paste_text, copy_text);
  }
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, DefaultSizes) {
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder().Build();

  HeadlessBrowser::Options::Builder builder;
  const HeadlessBrowser::Options kDefaultOptions = builder.Build();

#if !defined(OS_MACOSX)
  // On Mac headless does not override the screen dimensions, so they are
  // left with the actual screen values.
  EXPECT_EQ(kDefaultOptions.window_size.width(),
            EvaluateScript(web_contents, "screen.width")
                ->GetResult()
                ->GetValue()
                ->GetInt());
  EXPECT_EQ(kDefaultOptions.window_size.height(),
            EvaluateScript(web_contents, "screen.height")
                ->GetResult()
                ->GetValue()
                ->GetInt());
#endif  // !defined(OS_MACOSX)
  EXPECT_EQ(kDefaultOptions.window_size.width(),
            EvaluateScript(web_contents, "window.innerWidth")
                ->GetResult()
                ->GetValue()
                ->GetInt());
  EXPECT_EQ(kDefaultOptions.window_size.height(),
            EvaluateScript(web_contents, "window.innerHeight")
                ->GetResult()
                ->GetValue()
                ->GetInt());
}

namespace {

class ProtocolHandlerWithCookies
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  explicit ProtocolHandlerWithCookies(net::CookieList* sent_cookies);
  ~ProtocolHandlerWithCookies() override = default;

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  net::CookieList* sent_cookies_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(ProtocolHandlerWithCookies);
};

class URLRequestJobWithCookies : public TestURLRequestJob {
 public:
  URLRequestJobWithCookies(net::URLRequest* request,
                           net::NetworkDelegate* network_delegate,
                           net::CookieList* sent_cookies);
  ~URLRequestJobWithCookies() override = default;

  // net::URLRequestJob implementation:
  void Start() override;

 private:
  void SaveCookiesAndStart(const net::CookieList& cookie_list);

  net::CookieList* sent_cookies_;  // Not owned.
  base::WeakPtrFactory<URLRequestJobWithCookies> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(URLRequestJobWithCookies);
};

ProtocolHandlerWithCookies::ProtocolHandlerWithCookies(
    net::CookieList* sent_cookies)
    : sent_cookies_(sent_cookies) {}

net::URLRequestJob* ProtocolHandlerWithCookies::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new URLRequestJobWithCookies(request, network_delegate, sent_cookies_);
}

URLRequestJobWithCookies::URLRequestJobWithCookies(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    net::CookieList* sent_cookies)
    // Return an empty response for every request.
    : TestURLRequestJob(request, network_delegate, ""),
      sent_cookies_(sent_cookies),
      weak_factory_(this) {}

void URLRequestJobWithCookies::Start() {
  net::CookieStore* cookie_store = request_->context()->cookie_store();
  net::CookieOptions options;
  options.set_include_httponly();

  // See net::URLRequestHttpJob::AddCookieHeaderAndStart().
  url::Origin requested_origin = url::Origin::Create(request_->url());
  url::Origin site_for_cookies =
      url::Origin::Create(request_->site_for_cookies());

  if (net::registry_controlled_domains::SameDomainOrHost(
          requested_origin, site_for_cookies,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    if (net::registry_controlled_domains::SameDomainOrHost(
            requested_origin, request_->initiator(),
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX);
    } else if (net::HttpUtil::IsMethodSafe(request_->method())) {
      options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::INCLUDE_LAX);
    }
  }
  cookie_store->GetCookieListWithOptionsAsync(
      request_->url(), options,
      base::BindOnce(&URLRequestJobWithCookies::SaveCookiesAndStart,
                     weak_factory_.GetWeakPtr()));
}

void URLRequestJobWithCookies::SaveCookiesAndStart(
    const net::CookieList& cookie_list) {
  *sent_cookies_ = cookie_list;
  NotifyHeadersComplete();
}

}  // namespace

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, ReadCookiesInProtocolHandler) {
  net::CookieList sent_cookies;
  ProtocolHandlerMap protocol_handlers;
  protocol_handlers[url::kHttpsScheme] =
      std::make_unique<ProtocolHandlerWithCookies>(&sent_cookies);

  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetProtocolHandlers(std::move(protocol_handlers))
          .Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("https://example.com/cookie.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  // The first load has no cookies.
  EXPECT_EQ(0u, sent_cookies.size());

  // Set a cookie and reload the page.
  EXPECT_FALSE(EvaluateScript(
                   web_contents,
                   "document.cookie = 'shape=oblong', window.location.reload()")
                   ->HasExceptionDetails());
  EXPECT_TRUE(WaitForLoad(web_contents));

  // We should have sent the cookie this time.
  EXPECT_EQ(1u, sent_cookies.size());
  EXPECT_EQ("shape", sent_cookies[0].Name());
  EXPECT_EQ("oblong", sent_cookies[0].Value());
}

namespace {

class CookieSetter {
 public:
  CookieSetter(HeadlessBrowserTest* browser_test,
               HeadlessWebContents* web_contents,
               std::unique_ptr<network::SetCookieParams> set_cookie_params)
      : browser_test_(browser_test),
        web_contents_(web_contents),
        devtools_client_(HeadlessDevToolsClient::Create()) {
    web_contents_->GetDevToolsTarget()->AttachClient(devtools_client_.get());
    devtools_client_->GetNetwork()->GetExperimental()->SetCookie(
        std::move(set_cookie_params),
        base::BindOnce(&CookieSetter::OnSetCookieResult,
                       base::Unretained(this)));
  }

  ~CookieSetter() {
    web_contents_->GetDevToolsTarget()->DetachClient(devtools_client_.get());
  }

  void OnSetCookieResult(std::unique_ptr<network::SetCookieResult> result) {
    result_ = std::move(result);
    browser_test_->FinishAsynchronousTest();
  }

  std::unique_ptr<network::SetCookieResult> TakeResult() {
    return std::move(result_);
  }

 private:
  HeadlessBrowserTest* browser_test_;  // Not owned.
  HeadlessWebContents* web_contents_;  // Not owned.
  std::unique_ptr<HeadlessDevToolsClient> devtools_client_;

  std::unique_ptr<network::SetCookieResult> result_;

  DISALLOW_COPY_AND_ASSIGN(CookieSetter);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, SetCookiesWithDevTools) {
  net::CookieList sent_cookies;
  ProtocolHandlerMap protocol_handlers;
  protocol_handlers[url::kHttpsScheme] =
      base::WrapUnique(new ProtocolHandlerWithCookies(&sent_cookies));

  HeadlessBrowserContext* browser_context =
      browser()
          ->CreateBrowserContextBuilder()
          .SetProtocolHandlers(std::move(protocol_handlers))
          .Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("https://example.com/cookie.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  // The first load has no cookies.
  EXPECT_EQ(0u, sent_cookies.size());

  // Set some cookies.
  {
    std::unique_ptr<network::SetCookieParams> set_cookie_params =
        network::SetCookieParams::Builder()
            .SetUrl("https://example.com")
            .SetName("shape")
            .SetValue("oblong")
            .Build();
    CookieSetter cookie_setter(this, web_contents,
                               std::move(set_cookie_params));
    RunAsynchronousTest();
    std::unique_ptr<network::SetCookieResult> result =
        cookie_setter.TakeResult();
    EXPECT_TRUE(result->GetSuccess());
  }
  {
    // Try setting all the fields so we notice if the protocol for any of them
    // changes.
    std::unique_ptr<network::SetCookieParams> set_cookie_params =
        network::SetCookieParams::Builder()
            .SetUrl("https://other.com")
            .SetName("shape")
            .SetValue("trapezoid")
            .SetDomain("other.com")
            .SetPath("")
            .SetSecure(true)
            .SetHttpOnly(true)
            .SetSameSite(network::CookieSameSite::EXACT)
            .SetExpires(0)
            .Build();
    CookieSetter cookie_setter(this, web_contents,
                               std::move(set_cookie_params));
    RunAsynchronousTest();
    std::unique_ptr<network::SetCookieResult> result =
        cookie_setter.TakeResult();
    EXPECT_TRUE(result->GetSuccess());
  }

  // Reload the page.
  EXPECT_FALSE(EvaluateScript(web_contents, "window.location.reload();")
                   ->HasExceptionDetails());
  EXPECT_TRUE(WaitForLoad(web_contents));

  // We should have sent the matching cookies this time.
  EXPECT_EQ(1u, sent_cookies.size());
  EXPECT_EQ("shape", sent_cookies[0].Name());
  EXPECT_EQ("oblong", sent_cookies[0].Value());
}

// TODO(skyostil): This test currently relies on being able to run a shell
// script.
#if defined(OS_POSIX)
IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, RendererCommandPrefixTest) {
  base::ThreadRestrictions::SetIOAllowed(true);
  base::FilePath launcher_stamp;
  base::CreateTemporaryFile(&launcher_stamp);

  base::FilePath launcher_script;
  FILE* launcher_file = base::CreateAndOpenTemporaryFile(&launcher_script);
  fprintf(launcher_file, "#!/bin/sh\n");
  fprintf(launcher_file, "echo $@ > %s\n", launcher_stamp.value().c_str());
  fprintf(launcher_file, "exec $@\n");
  fclose(launcher_file);
#if !defined(OS_FUCHSIA)
  base::SetPosixFilePermissions(launcher_script,
                                base::FILE_PERMISSION_READ_BY_USER |
                                    base::FILE_PERMISSION_EXECUTE_BY_USER);
#endif  // !defined(OS_FUCHSIA)

  base::CommandLine::ForCurrentProcess()->AppendSwitch("--no-sandbox");
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      "--renderer-cmd-prefix", launcher_script.value());

  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  // Make sure the launcher was invoked when starting the renderer.
  std::string stamp;
  EXPECT_TRUE(base::ReadFileToString(launcher_stamp, &stamp));
  EXPECT_GE(stamp.find("--type=renderer"), 0u);

  base::DeleteFile(launcher_script, false);
  base::DeleteFile(launcher_stamp, false);
}
#endif  // defined(OS_POSIX)

class CrashReporterTest : public HeadlessBrowserTest,
                          public HeadlessWebContents::Observer,
                          inspector::ExperimentalObserver {
 public:
  CrashReporterTest() : devtools_client_(HeadlessDevToolsClient::Create()) {}
  ~CrashReporterTest() override = default;

  void SetUp() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    base::CreateNewTempDirectory(FILE_PATH_LITERAL("CrashReporterTest"),
                                 &crash_dumps_dir_);
    EXPECT_FALSE(options()->enable_crash_reporter);
    options()->enable_crash_reporter = true;
    options()->crash_dumps_dir = crash_dumps_dir_;
    HeadlessBrowserTest::SetUp();
  }

  void TearDown() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    base::DeleteFile(crash_dumps_dir_, /* recursive */ false);
  }

  // HeadlessWebContents::Observer implementation:
  void DevToolsTargetReady() override {
    EXPECT_TRUE(web_contents_->GetDevToolsTarget());
    web_contents_->GetDevToolsTarget()->AttachClient(devtools_client_.get());
    devtools_client_->GetInspector()->GetExperimental()->AddObserver(this);
  }

  // inspector::ExperimentalObserver implementation:
  void OnTargetCrashed(const inspector::TargetCrashedParams&) override {
    FinishAsynchronousTest();
  }

 protected:
  HeadlessBrowserContext* browser_context_ = nullptr;
  HeadlessWebContents* web_contents_ = nullptr;
  std::unique_ptr<HeadlessDevToolsClient> devtools_client_;
  base::FilePath crash_dumps_dir_;
};

// TODO(skyostil): Minidump generation currently is only supported on Linux and
// Mac.
#if defined(HEADLESS_USE_BREAKPAD) || defined(OS_MACOSX)
#define MAYBE_GenerateMinidump GenerateMinidump
#else
#define MAYBE_GenerateMinidump DISABLED_GenerateMinidump
#endif  // defined(HEADLESS_USE_BREAKPAD) || defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(CrashReporterTest, MAYBE_GenerateMinidump) {
  // Navigates a tab to chrome://crash and checks that a minidump is generated.
  // Note that we only test renderer crashes here -- browser crashes need to be
  // tested with a separate harness.
  //
  // The case where crash reporting is disabled is covered by
  // HeadlessCrashObserverTest.
  browser_context_ = browser()->CreateBrowserContextBuilder().Build();

  web_contents_ = browser_context_->CreateWebContentsBuilder()
                      .SetInitialURL(GURL(content::kChromeUICrashURL))
                      .Build();

  web_contents_->AddObserver(this);
  RunAsynchronousTest();

  // The target has crashed and should no longer be there.
  EXPECT_FALSE(web_contents_->GetDevToolsTarget());

  // Check that one minidump got created.
  {
#if defined(OS_MACOSX)
    // Mac outputs dumps in the 'pending' directory.
    crash_dumps_dir_ = crash_dumps_dir_.Append("pending");
#endif
    base::ThreadRestrictions::SetIOAllowed(true);
    base::FileEnumerator it(crash_dumps_dir_, /* recursive */ false,
                            base::FileEnumerator::FILES);
    base::FilePath minidump = it.Next();
    EXPECT_FALSE(minidump.empty());
    EXPECT_EQ(FILE_PATH_LITERAL(".dmp"), minidump.Extension());
    EXPECT_TRUE(it.Next().empty());
  }

  web_contents_->RemoveObserver(this);
  web_contents_->Close();
  web_contents_ = nullptr;

  browser_context_->Close();
  browser_context_ = nullptr;
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, PermissionManagerAlwaysASK) {
  GURL url("https://example.com");

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* headless_web_contents =
      browser_context->CreateWebContentsBuilder().Build();
  EXPECT_TRUE(headless_web_contents);

  HeadlessWebContentsImpl* web_contents =
      HeadlessWebContentsImpl::From(headless_web_contents);

  content::PermissionManager* permission_manager =
      web_contents->browser_context()->GetPermissionManager();
  EXPECT_NE(nullptr, permission_manager);

  // Check that the permission manager returns ASK for a given permission type.
  EXPECT_EQ(blink::mojom::PermissionStatus::ASK,
            permission_manager->GetPermissionStatus(
                content::PermissionType::NOTIFICATIONS, url, url));
}

class HeadlessBrowserTestWithNetLog : public HeadlessBrowserTest {
 public:
  HeadlessBrowserTestWithNetLog() = default;

  void SetUp() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    EXPECT_TRUE(base::CreateTemporaryFile(&net_log_));
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        "--log-net-log", net_log_.MaybeAsASCII());
    HeadlessBrowserTest::SetUp();
  }

  void TearDown() override {
    HeadlessBrowserTest::TearDown();
    base::DeleteFile(net_log_, false);
  }

 protected:
  base::FilePath net_log_;
};

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTestWithNetLog, WriteNetLog) {
  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));
  browser()->Shutdown();

  base::ThreadRestrictions::SetIOAllowed(true);
  std::string net_log_data;
  EXPECT_TRUE(base::ReadFileToString(net_log_, &net_log_data));
  EXPECT_GE(net_log_data.find("hello.html"), 0u);
}

namespace {

class TraceHelper : public tracing::ExperimentalObserver {
 public:
  TraceHelper(HeadlessBrowserTest* browser_test, HeadlessDevToolsTarget* target)
      : browser_test_(browser_test),
        target_(target),
        client_(HeadlessDevToolsClient::Create()),
        tracing_data_(std::make_unique<base::ListValue>()) {
    EXPECT_FALSE(target_->IsAttached());
    target_->AttachClient(client_.get());
    EXPECT_TRUE(target_->IsAttached());

    client_->GetTracing()->GetExperimental()->AddObserver(this);

    client_->GetTracing()->GetExperimental()->Start(
        tracing::StartParams::Builder().Build(),
        base::BindOnce(&TraceHelper::OnTracingStarted, base::Unretained(this)));
  }

  ~TraceHelper() override {
    target_->DetachClient(client_.get());
    EXPECT_FALSE(target_->IsAttached());
  }

  std::unique_ptr<base::ListValue> TakeTracingData() {
    return std::move(tracing_data_);
  }

 private:
  void OnTracingStarted(std::unique_ptr<tracing::StartResult>) {
    // We don't need the callback from End, but the OnTracingComplete event.
    client_->GetTracing()->GetExperimental()->End(
        tracing::EndParams::Builder().Build());
  }

  // tracing::ExperimentalObserver implementation:
  void OnDataCollected(const tracing::DataCollectedParams& params) override {
    for (const auto& value : *params.GetValue()) {
      tracing_data_->Append(value->CreateDeepCopy());
    }
  }

  void OnTracingComplete(
      const tracing::TracingCompleteParams& params) override {
    browser_test_->FinishAsynchronousTest();
  }

  HeadlessBrowserTest* browser_test_;  // Not owned.
  HeadlessDevToolsTarget* target_;     // Not owned.
  std::unique_ptr<HeadlessDevToolsClient> client_;

  std::unique_ptr<base::ListValue> tracing_data_;

  DISALLOW_COPY_AND_ASSIGN(TraceHelper);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, TraceUsingBrowserDevToolsTarget) {
  HeadlessDevToolsTarget* target = browser()->GetDevToolsTarget();
  EXPECT_NE(nullptr, target);

  TraceHelper helper(this, target);
  RunAsynchronousTest();

  std::unique_ptr<base::ListValue> tracing_data = helper.TakeTracingData();
  EXPECT_TRUE(tracing_data);
  EXPECT_LT(0u, tracing_data->GetSize());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, WindowPrint) {
  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));
  EXPECT_FALSE(
      EvaluateScript(web_contents, "window.print()")->HasExceptionDetails());
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, AllowInsecureLocalhostFlag) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
  https_server.ServeFilesFromSourceDirectory("headless/test/data");
  ASSERT_TRUE(https_server.Start());
  GURL test_url = https_server.GetURL("/hello.html");

  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kAllowInsecureLocalhost);

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContentsImpl* web_contents =
      HeadlessWebContentsImpl::From(browser_context->CreateWebContentsBuilder()
                                        .SetInitialURL(test_url)
                                        .Build());

  // If the certificate fails to validate, this should fail.
  EXPECT_TRUE(WaitForLoad(web_contents));
}

class HeadlessBrowserTestAppendCommandLineFlags : public HeadlessBrowserTest {
 public:
  HeadlessBrowserTestAppendCommandLineFlags() {
    options()->append_command_line_flags_callback = base::Bind(
        &HeadlessBrowserTestAppendCommandLineFlags::AppendCommandLineFlags,
        base::Unretained(this));
  }

  void AppendCommandLineFlags(base::CommandLine* command_line,
                              HeadlessBrowserContext* child_browser_context,
                              const std::string& child_process_type,
                              int child_process_id) {
    if (child_process_type != "renderer")
      return;

    callback_was_run_ = true;
    EXPECT_LE(0, child_process_id);
    EXPECT_NE(nullptr, command_line);
    EXPECT_NE(nullptr, child_browser_context);
  }

 protected:
  bool callback_was_run_ = false;
};

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTestAppendCommandLineFlags,
                       AppendChildProcessCommandLineFlags) {
  // Create a new renderer process, and verify that callback was executed.
  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();
  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(GURL("about:blank"))
          .Build();

  EXPECT_TRUE(callback_was_run_);

  // Used only for lifetime.
  (void)web_contents;
}

IN_PROC_BROWSER_TEST_F(HeadlessBrowserTest, ServerWantsClientCertificate) {
  net::SpawnedTestServer::SSLOptions ssl_options;
  ssl_options.request_client_certificate = true;

  net::SpawnedTestServer server(
      net::SpawnedTestServer::TYPE_HTTPS, ssl_options,
      base::FilePath(FILE_PATH_LITERAL("headless/test/data")));
  EXPECT_TRUE(server.Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(server.GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));
}

}  // namespace headless
