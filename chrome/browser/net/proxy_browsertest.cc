// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/login/login_handler.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/load_flags.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/simple_connection_listener.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/test/test_data_directory.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace {

// PAC script that sends all requests to an invalid proxy server.
const base::FilePath::CharType kPACScript[] = FILE_PATH_LITERAL(
    "bad_server.pac");

// Verify kPACScript is installed as the PAC script.
void VerifyProxyScript(Browser* browser) {
  ui_test_utils::NavigateToURL(browser, GURL("http://google.com"));

  // Verify we get the ERR_PROXY_CONNECTION_FAILED screen.
  bool result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser->tab_strip_model()->GetActiveWebContents(),
      "var textContent = document.body.textContent;"
      "var hasError = textContent.indexOf('ERR_PROXY_CONNECTION_FAILED') >= 0;"
      "domAutomationController.send(hasError);",
      &result));
  EXPECT_TRUE(result);
}

// This class observes chrome::NOTIFICATION_AUTH_NEEDED and supplies
// the credential which is required by the test proxy server.
// "foo:bar" is the required username and password for our test proxy server.
class LoginPromptObserver : public content::NotificationObserver {
 public:
  LoginPromptObserver() : auth_handled_(false) {}

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    if (type == chrome::NOTIFICATION_AUTH_NEEDED) {
      LoginNotificationDetails* login_details =
          content::Details<LoginNotificationDetails>(details).ptr();
      // |login_details->handler()| is the associated LoginHandler object.
      // SetAuth() will close the login dialog.
      login_details->handler()->SetAuth(base::ASCIIToUTF16("foo"),
                                        base::ASCIIToUTF16("bar"));
      auth_handled_ = true;
    }
  }

  bool auth_handled() const { return auth_handled_; }

 private:
  bool auth_handled_;

  DISALLOW_COPY_AND_ASSIGN(LoginPromptObserver);
};

class ProxyBrowserTest : public InProcessBrowserTest {
 public:
  ProxyBrowserTest()
      : proxy_server_(net::SpawnedTestServer::TYPE_BASIC_AUTH_PROXY,
                      base::FilePath()) {
  }

  void SetUp() override {
    ASSERT_TRUE(proxy_server_.Start());
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kProxyServer,
                                    proxy_server_.host_port_pair().ToString());
  }

 protected:
  net::SpawnedTestServer proxy_server_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProxyBrowserTest);
};

// We bypass manually installed proxy for localhost on chromeos.
#if defined(OS_CHROMEOS)
#define MAYBE_BasicAuthWSConnect DISABLED_BasicAuthWSConnect
#else
#define MAYBE_BasicAuthWSConnect BasicAuthWSConnect
#endif
// Test that the browser can establish a WebSocket connection via a proxy
// that requires basic authentication. This test also checks the headers
// arrive at WebSocket server.
IN_PROC_BROWSER_TEST_F(ProxyBrowserTest, MAYBE_BasicAuthWSConnect) {
  // Launch WebSocket server.
  net::SpawnedTestServer ws_server(net::SpawnedTestServer::TYPE_WS,
                                   net::GetWebSocketTestDataDirectory());
  ASSERT_TRUE(ws_server.Start());

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::NavigationController* controller = &tab->GetController();
  content::NotificationRegistrar registrar;
  // The proxy server will request basic authentication.
  // |observer| supplies the credential.
  LoginPromptObserver observer;
  registrar.Add(&observer, chrome::NOTIFICATION_AUTH_NEEDED,
                content::Source<content::NavigationController>(controller));

  content::TitleWatcher watcher(tab, base::ASCIIToUTF16("PASS"));
  watcher.AlsoWaitForTitle(base::ASCIIToUTF16("FAIL"));

  // Visit a page that tries to establish WebSocket connection. The title
  // of the page will be 'PASS' on success.
  GURL::Replacements replacements;
  replacements.SetSchemeStr("http");
  ui_test_utils::NavigateToURL(browser(),
                               ws_server.GetURL("proxied_request_check.html")
                                   .ReplaceComponents(replacements));

  const base::string16 result = watcher.WaitAndGetTitle();
  EXPECT_TRUE(base::EqualsASCII(result, "PASS"));
  EXPECT_TRUE(observer.auth_handled());
}

// Fetch PAC script via an http:// URL.
class HttpProxyScriptBrowserTest : public InProcessBrowserTest {
 public:
  HttpProxyScriptBrowserTest() {
    http_server_.ServeFilesFromSourceDirectory("chrome/test/data");
  }
  ~HttpProxyScriptBrowserTest() override {}

  void SetUp() override {
    ASSERT_TRUE(http_server_.Start());
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath pac_script_path(FILE_PATH_LITERAL("/"));
    command_line->AppendSwitchASCII(switches::kProxyPacUrl, http_server_.GetURL(
        pac_script_path.Append(kPACScript).MaybeAsASCII()).spec());
  }

 private:
  net::EmbeddedTestServer http_server_;

  DISALLOW_COPY_AND_ASSIGN(HttpProxyScriptBrowserTest);
};

IN_PROC_BROWSER_TEST_F(HttpProxyScriptBrowserTest, Verify) {
  VerifyProxyScript(browser());
}

// Fetch PAC script via a hanging http:// URL.
class HangingPacRequestProxyScriptBrowserTest : public InProcessBrowserTest {
 public:
  HangingPacRequestProxyScriptBrowserTest() {}
  ~HangingPacRequestProxyScriptBrowserTest() override {}

  void SetUp() override {
    // Must start listening (And get a port for the proxy) before calling
    // SetUp().
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    // Need to stop this before |connection_listener_| is destroyed.
    EXPECT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    InProcessBrowserTest::TearDown();
  }

  void SetUpOnMainThread() override {
    // This must be created after the main message loop has been set up.
    // Waits for one connection.  Additional connections are fine.
    connection_listener_ =
        std::make_unique<net::test_server::SimpleConnectionListener>(
            1, net::test_server::SimpleConnectionListener::
                   ALLOW_ADDITIONAL_CONNECTIONS);
    embedded_test_server()->SetConnectionListener(connection_listener_.get());
    embedded_test_server()->StartAcceptingConnections();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kProxyPacUrl, embedded_test_server()->GetURL("/hung").spec());
  }

 protected:
  std::unique_ptr<net::test_server::SimpleConnectionListener>
      connection_listener_;

 private:
  DISALLOW_COPY_AND_ASSIGN(HangingPacRequestProxyScriptBrowserTest);
};

// URLFetcherDelegate that expects a request to hang.
class HangingURLFetcherDelegate : public net::URLFetcherDelegate {
 public:
  HangingURLFetcherDelegate() {}
  ~HangingURLFetcherDelegate() override {}

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    ADD_FAILURE() << "This request should never complete.";
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HangingURLFetcherDelegate);
};

// Check that the URLRequest for a PAC that is still alive during shutdown is
// safely cleaned up.  This test relies on AssertNoURLRequests being called on
// the main URLRequestContext.
IN_PROC_BROWSER_TEST_F(HangingPacRequestProxyScriptBrowserTest, Shutdown) {
  // Request that should hang while trying to request the PAC script.
  // Enough requests are created on startup that this probably isn't needed, but
  // best to be safe.
  HangingURLFetcherDelegate hanging_request_delegate;
  std::unique_ptr<net::URLFetcher> hanging_fetcher = net::URLFetcher::Create(
      GURL("http://blah/"), net::URLFetcher::GET, &hanging_request_delegate,
      TRAFFIC_ANNOTATION_FOR_TESTS);
  hanging_fetcher->SetRequestContext(browser()->profile()->GetRequestContext());
  hanging_fetcher->Start();

  connection_listener_->WaitForConnections();
}

}  // namespace
