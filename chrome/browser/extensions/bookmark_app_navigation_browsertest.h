// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_BROWSERTEST_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_BROWSERTEST_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "extensions/common/extension.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class Browser;

namespace content {
class RenderFrameHost;
class TestNavigationObserver;
class WebContents;
}  // namespace content

namespace extensions {
namespace test {

class BookmarkAppNavigationBrowserTest : public ExtensionBrowserTest {
 public:
  enum class LinkTarget {
    SELF,
    BLANK,
  };

  // Various string constants for in/out-of-scope URLs.
  static const char* GetLaunchingPageHost();
  static const char* GetLaunchingPagePath();
  static const char* GetAppUrlHost();
  static const char* GetOtherAppUrlHost();
  static const char* GetAppScopePath();
  static const char* GetAppUrlPath();
  static const char* GetInScopeUrlPath();
  static const char* GetOutOfScopeUrlPath();
  static const char* GetAppName();

  static std::string CreateServerRedirect(const GURL& target_url);

  static std::unique_ptr<content::TestNavigationObserver>
  GetTestNavigationObserver(const GURL& target_url);

  // Returns a subframe in |web_contents|. |web_contents| is expected to have
  // exactly 2 frames: the main frame and a subframe.
  static content::RenderFrameHost* GetIFrame(
      content::WebContents* web_contents);

 protected:
  // Creates an <a> element, sets its href and target to |link_url| and |target|
  // respectively, adds it to the DOM, and clicks on it with |modifiers|.
  // Returns once |target_url| has loaded. |modifiers| should be based on
  // blink::WebInputEvent::Modifiers.
  static void ClickLinkWithModifiersAndWaitForURL(
      content::WebContents* web_contents,
      const GURL& link_url,
      const GURL& target_url,
      LinkTarget target,
      const std::string& rel,
      int modifiers);

  // Creates an <a> element, sets its href and target to |link_url| and |target|
  // respectively, adds it to the DOM, and clicks on it. Returns once
  // |target_url| has loaded.
  static void ClickLinkAndWaitForURL(content::WebContents* web_contents,
                                     const GURL& link_url,
                                     const GURL& target_url,
                                     LinkTarget target,
                                     const std::string& rel);

  // Creates an <a> element, sets its href and target to |link_url| and |target|
  // respectively, adds it to the DOM, and clicks on it. Returns once the link
  // has loaded.
  static void ClickLinkAndWait(content::WebContents* web_contents,
                               const GURL& link_url,
                               LinkTarget target,
                               const std::string& rel);

  // Creates an <a> element, sets its href and target to |link_url| and |target|
  // respectively, adds it to the DOM, and clicks on it with |modifiers|.
  // Returns once the link has loaded. |modifiers| should be based on
  // blink::WebInputEvent::Modifiers.
  static void ClickLinkWithModifiersAndWait(content::WebContents* web_contents,
                                            const GURL& link_url,
                                            LinkTarget target,
                                            const std::string& rel,
                                            int modifiers);

  BookmarkAppNavigationBrowserTest();
  ~BookmarkAppNavigationBrowserTest() override;

  void SetUp() override;
  void SetUpInProcessBrowserTestFixture() override;
  void TearDownInProcessBrowserTestFixture() override;
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;

  void InstallTestBookmarkApp();
  void InstallOtherTestBookmarkApp();
  const Extension* InstallTestBookmarkApp(const std::string& app_host);

  // Installs a Bookmark App that immediately redirects to a URL with
  // |target_host| and |target_path|.
  const Extension* InstallImmediateRedirectingApp(
      const std::string& target_host,
      const std::string& target_path);

  Browser* OpenTestBookmarkApp();

  // Navigates the active tab in |browser| to the launching page.
  void NavigateToLaunchingPage(Browser* browser);

  // Navigates the active tab to the launching page.
  void NavigateToLaunchingPage();

  // Navigates the browser's current active tab to the test app's URL. It does
  // not open a new app window.
  void NavigateToTestAppURL();

  // Checks that, after running |action|, the initial tab's window doesn't have
  // any new tabs, the initial tab did not navigate, and that no new windows
  // have been opened.
  void TestTabActionDoesNotNavigateOrOpenAppWindow(base::OnceClosure action);

  // Checks that, after running |action|, a new tab is opened with |target_url|,
  // the initial tab is still focused, and that the initial tab didn't
  // navigate.
  void TestTabActionOpensBackgroundTab(const GURL& target_url,
                                       base::OnceClosure action);

  // Checks that a new regular browser window is opened, that the new window
  // is in the foreground, and that the initial tab didn't navigate.
  void TestTabActionOpensForegroundWindow(const GURL& target_url,
                                          base::OnceClosure action);

  // Checks that, after running |action|, the initial tab's window doesn't have
  // any new tabs, the initial tab did not navigate, and that a new app window
  // with |target_url| is opened.
  void TestTabActionOpensAppWindow(const GURL& target_url,
                                   base::OnceClosure action);

  // Same as TestTabActionOpensAppWindow(), but also tests that the newly opened
  // app window has an opener.
  void TestTabActionOpensAppWindowWithOpener(const GURL& target_url,
                                             base::OnceClosure action);

  // Checks that no new windows are opened after running |action| and that the
  // existing |browser| window is still the active one and navigated to
  // |target_url|. Returns true if there were no errors.
  bool TestActionDoesNotOpenAppWindow(Browser* browser,
                                      const GURL& target_url,
                                      base::OnceClosure action);

  // Checks that a new foreground tab is opened in an existing browser, that the
  // new tab's browser is in the foreground, and that |app_browser| didn't
  // navigate.
  void TestAppActionOpensForegroundTab(Browser* app_browser,
                                       const GURL& target_url,
                                       base::OnceClosure action);

  // Checks that a new app window is opened, that the new window is in the
  // foreground, that the |app_browser| didn't navigate and that the new window
  // has an opener.
  void TestAppActionOpensAppWindowWithOpener(Browser* app_browser,
                                             const GURL& target_url,
                                             base::OnceClosure action);

  // Checks that no new windows are opened after running |action| and that the
  // main browser window is still the active one and navigated to |target_url|.
  // Returns true if there were no errors.
  bool TestTabActionDoesNotOpenAppWindow(const GURL& target_url,
                                         base::OnceClosure action);

  // Checks that no new windows are opened after running |action| and that the
  // iframe in the initial tab navigated to |target_url|. Returns true if there
  // were no errors.
  bool TestIFrameActionDoesNotOpenAppWindow(const GURL& target_url,
                                            base::OnceClosure action);

  GURL GetLaunchingPageURL();

  const base::HistogramTester& global_histogram() { return histogram_tester_; }

  const Extension* test_bookmark_app() { return test_bookmark_app_; }

  const net::EmbeddedTestServer& https_server() { return https_server_; }

  CertVerifierBrowserTest::CertVerifier& mock_cert_verifier() {
    return cert_verifier_;
  }

 private:
  net::EmbeddedTestServer https_server_;
  net::MockCertVerifier mock_cert_verifier_;
  // Similar to net::MockCertVerifier, but also updates the CertVerifier
  // used by the NetworkService. This is needed for when tests run with
  // the NetworkService enabled.
  CertVerifierBrowserTest::CertVerifier cert_verifier_;
  const Extension* test_bookmark_app_;
  base::HistogramTester histogram_tester_;
};

}  // namespace test
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_BROWSERTEST_H_
