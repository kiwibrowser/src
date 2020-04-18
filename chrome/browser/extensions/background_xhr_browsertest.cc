// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/browsertest_util.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "extensions/test/test_extension_dir.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/ssl/client_cert_store.h"
#include "net/ssl/ssl_server_config.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace extensions {

namespace {

constexpr const char kWebstoreDomain[] = "cws.com";

std::unique_ptr<net::ClientCertStore> CreateNullCertStore() {
  return nullptr;
}

void InstallNullCertStoreFactoryOnIOThread(
    content::ResourceContext* resource_context) {
  ProfileIOData::FromResourceContext(resource_context)
      ->set_client_cert_store_factory_for_testing(
          base::Bind(&CreateNullCertStore));
}

}  // namespace

class BackgroundXhrTest : public ExtensionBrowserTest {
 protected:
  void RunTest(const std::string& path, const GURL& url) {
    const Extension* extension =
        LoadExtension(test_data_dir_.AppendASCII("background_xhr"));
    ASSERT_TRUE(extension);

    ResultCatcher catcher;
    GURL test_url = net::AppendQueryParameter(extension->GetResourceURL(path),
                                              "url", url.spec());
    ui_test_utils::NavigateToURL(browser(), test_url);
    ASSERT_TRUE(catcher.GetNextResult());
  }
};

// Test that fetching a URL using TLS client auth doesn't crash, hang, or
// prompt.
IN_PROC_BROWSER_TEST_F(BackgroundXhrTest, TlsClientAuth) {
  // Install a null ClientCertStore so the client auth prompt isn't bypassed due
  // to the system certificate store returning no certificates.
  base::RunLoop loop;
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&InstallNullCertStoreFactoryOnIOThread,
                     browser()->profile()->GetResourceContext()),
      loop.QuitClosure());
  loop.Run();

  // Launch HTTPS server.
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  net::SSLServerConfig ssl_config;
  ssl_config.client_cert_type =
      net::SSLServerConfig::ClientCertType::REQUIRE_CLIENT_CERT;
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK, ssl_config);
  https_server.ServeFilesFromSourceDirectory("content/test/data");
  ASSERT_TRUE(https_server.Start());

  ASSERT_NO_FATAL_FAILURE(
      RunTest("test_tls_client_auth.html", https_server.GetURL("/")));
}

// Test that fetching a URL using HTTP auth doesn't crash, hang, or prompt.
IN_PROC_BROWSER_TEST_F(BackgroundXhrTest, HttpAuth) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_NO_FATAL_FAILURE(RunTest(
      "test_http_auth.html", embedded_test_server()->GetURL("/auth-basic")));
}

class BackgroundXhrWebstoreTest : public ExtensionApiTest {
 public:
  BackgroundXhrWebstoreTest() = default;
  ~BackgroundXhrWebstoreTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    // TODO(devlin): For some reason, trying to fetch an HTTPS url in this test
    // fails (even when using an HTTPS EmbeddedTestServer). For this reason, we
    // need to fake the webstore URLs as http versions.
    command_line->AppendSwitchASCII(
        ::switches::kAppsGalleryURL,
        base::StringPrintf("http://%s", kWebstoreDomain));
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BackgroundXhrWebstoreTest);
};

// Extensions should not be able to XHR to the webstore.
IN_PROC_BROWSER_TEST_F(BackgroundXhrWebstoreTest, XHRToWebstore) {
  TestExtensionDir test_dir;
  test_dir.WriteManifest(R"(
    {
      "name": "XHR Test",
      "manifest_version": 2,
      "version": "0.1",
      "background": {"scripts": ["background.js"]},
      "permissions": ["<all_urls>"]
    })");
  constexpr char kBackgroundScriptFile[] =
      R"(function canFetch(url) {
           console.warn('Fetching: ' + url);
           fetch(url).then((response) => {
             domAutomationController.send('true');
           }).catch((e) => {
             domAutomationController.send('false');
           });
         }
         chrome.test.sendMessage('ready');)";

  test_dir.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundScriptFile);

  ExtensionTestMessageListener listener("ready", false);
  const Extension* extension = LoadExtension(test_dir.UnpackedPath());
  ASSERT_TRUE(extension);
  EXPECT_TRUE(listener.WaitUntilSatisfied());

  content::BrowserContext* browser_context = profile();
  auto can_fetch = [extension, browser_context](const GURL& url) {
    std::string result = browsertest_util::ExecuteScriptInBackgroundPage(
        browser_context, extension->id(),
        base::StringPrintf("canFetch('%s');", url.spec().c_str()));
    EXPECT_TRUE(result == "true" || result == "false")
        << "Unexpected result: " << result;
    return result == "true";
  };

  GURL webstore_launch_url = extension_urls::GetWebstoreLaunchURL();
  GURL webstore_url_to_fetch = embedded_test_server()->GetURL(
      webstore_launch_url.host(), "/simple.html");

  EXPECT_FALSE(can_fetch(webstore_url_to_fetch));

  // Sanity check: the extension should be able to fetch google.com.
  GURL google_url =
      embedded_test_server()->GetURL("google.com", "/simple.html");
  EXPECT_TRUE(can_fetch(google_url));
}

}  // namespace extensions
