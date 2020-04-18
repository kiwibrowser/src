// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace chromeos {
namespace {

constexpr char kFakeOnlineEulaPath[] = "/intl/en-US/chrome/eula_text.html";
constexpr char kFakeOnlineEula[] = "No obligations at all";

#if defined(GOOGLE_CHROME_BUILD)
// See IDS_ABOUT_TERMS_OF_SERVICE for the complete text.
constexpr char kOfflineEULAWarning[] = "Chrome OS Terms";
#else
// Placeholder text in terms_chromium.html.
constexpr char kOfflineEULAWarning[] =
    "In official builds this space will show the terms of service.";
#endif

// Helper class to wait until the WebCotnents finishes loading.
class WebContentsLoadFinishedWaiter : public content::WebContentsObserver {
 public:
  explicit WebContentsLoadFinishedWaiter(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}
  ~WebContentsLoadFinishedWaiter() override = default;

  void Wait() {
    if (!web_contents()->IsLoading())
      return;

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

 private:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& url) override {
    if (run_loop_)
      run_loop_->Quit();
  }

  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsLoadFinishedWaiter);
};

// Helper invoked by GuestViewManager::ForEachGuest to collect WebContents of
// Webview named as |web_view_name,|.
bool AddNamedWebContentsToSet(std::set<content::WebContents*>* frame_set,
                              const std::string& web_view_name,
                              content::WebContents* web_contents) {
  auto* web_view = extensions::WebViewGuest::FromWebContents(web_contents);
  if (web_view && web_view->name() == web_view_name)
    frame_set->insert(web_contents);
  return false;
}

class EulaTest : public OobeBaseTest {
 public:
  EulaTest() = default;
  ~EulaTest() override = default;

  // OobeBaseTest:
  void RegisterAdditionalRequestHandlers() override {
    embedded_test_server()->RegisterRequestHandler(
        base::Bind(&EulaTest::HandleRequest, base::Unretained(this)));
  }

  void OverrideOnlineEulaUrl() {
    // Override with the embedded test server's base url. Otherwise, the load
    // would not hit the embedded test server.
    const GURL fake_eula_url =
        embedded_test_server()->base_url().Resolve(kFakeOnlineEulaPath);
    JS().Evaluate(
        base::StringPrintf("loadTimeData.overrideValues({eulaOnlineUrl: '%s'});"
                           "Oobe.updateLocalizedContent();",
                           fake_eula_url.spec().c_str()));
  }

  void ShowEulaScreen() {
    LoginDisplayHost::default_host()->StartWizard(OobeScreen::SCREEN_OOBE_EULA);
    OverrideOnlineEulaUrl();
    OobeScreenWaiter(OobeScreen::SCREEN_OOBE_EULA).Wait();
  }

  std::string GetLoadedEulaAsText() {
    // Wait the contents to load.
    WebContentsLoadFinishedWaiter(FindEulaContents()).Wait();

    std::string eula_text;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        FindEulaContents(),
        "window.domAutomationController.send(document.body.textContent);",
        &eula_text));

    return eula_text;
  }

  void set_allow_online_eula(bool allow) { allow_online_eula_ = allow; }

 protected:
  content::WebContents* FindEulaContents() {
    // Tag the Eula webview in use with a unique name.
    constexpr char kUniqueEulaWebviewName[] = "unique-eula-webview-name";
    JS().Evaluate(base::StringPrintf(
        "(function(){"
        "  var isMd = (loadTimeData.getString('newOobeUI') == 'on');"
        "  var eulaWebView = isMd ? $('oobe-eula-md').$.crosEulaFrame : "
        "                           $('cros-eula-frame');"
        "  eulaWebView.name = '%s';"
        "})();",
        kUniqueEulaWebviewName));

    // Find the WebContents tagged with the unique name.
    std::set<content::WebContents*> frame_set;
    auto* const owner_contents = GetLoginUI()->GetWebContents();
    auto* const manager = guest_view::GuestViewManager::FromBrowserContext(
        owner_contents->GetBrowserContext());
    manager->ForEachGuest(
        owner_contents,
        base::BindRepeating(&AddNamedWebContentsToSet, &frame_set,
                            kUniqueEulaWebviewName));
    EXPECT_EQ(1u, frame_set.size());
    return *frame_set.begin();
  }

 private:
  std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
    GURL request_url = GURL("http://localhost").Resolve(request.relative_url);
    const std::string request_path = request_url.path();
    if (!base::EndsWith(request_path, "/eula_text.html",
                        base::CompareCase::SENSITIVE)) {
      return std::unique_ptr<HttpResponse>();
    }

    std::unique_ptr<BasicHttpResponse> http_response =
        std::make_unique<BasicHttpResponse>();

    if (allow_online_eula_) {
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(kFakeOnlineEula);
    } else {
      http_response->set_code(net::HTTP_SERVICE_UNAVAILABLE);
    }

    return std::move(http_response);
  }

  bool allow_online_eula_ = false;

  DISALLOW_COPY_AND_ASSIGN(EulaTest);
};

// Tests that online version is shown when it is accessible.
IN_PROC_BROWSER_TEST_F(EulaTest, LoadOnline) {
  set_allow_online_eula(true);
  ShowEulaScreen();

  EXPECT_TRUE(GetLoadedEulaAsText().find(kFakeOnlineEula) != std::string::npos);
}

// Tests that offline version is shown when the online version is not
// accessible.
IN_PROC_BROWSER_TEST_F(EulaTest, LoadOffline) {
  set_allow_online_eula(false);
  ShowEulaScreen();

  content::WebContents* eula_contents = FindEulaContents();
  ASSERT_TRUE(eula_contents);
  // Wait for the fallback offline page (loaded as data url) to be loaded.
  while (!eula_contents->GetLastCommittedURL().SchemeIs("data")) {
    // Pump messages to avoid busy loop so that renderer could do some work.
    base::RunLoop().RunUntilIdle();
    WebContentsLoadFinishedWaiter(eula_contents).Wait();
  }

  EXPECT_TRUE(GetLoadedEulaAsText().find(kOfflineEULAWarning) !=
              std::string::npos);
}

}  // namespace
}  // namespace chromeos
