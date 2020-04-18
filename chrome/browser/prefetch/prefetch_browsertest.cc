// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/network_change_notifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using chrome_browser_net::NetworkPredictionOptions;
using net::NetworkChangeNotifier;

namespace {

const char kPrefetchPage[] = "/prerender/simple_prefetch.html";

class MockNetworkChangeNotifierWIFI : public NetworkChangeNotifier {
 public:
  ConnectionType GetCurrentConnectionType() const override {
    return NetworkChangeNotifier::CONNECTION_WIFI;
  }
};

class MockNetworkChangeNotifier4G : public NetworkChangeNotifier {
 public:
  ConnectionType GetCurrentConnectionType() const override {
    return NetworkChangeNotifier::CONNECTION_4G;
  }
};

class PrefetchBrowserTest : public InProcessBrowserTest {
 public:
  PrefetchBrowserTest() {}

  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetPreference(NetworkPredictionOptions value) {
    browser()->profile()->GetPrefs()->SetInteger(
        prefs::kNetworkPredictionOptions, value);
  }

  bool RunPrefetchExperiment(bool expect_success, Browser* browser) {
    GURL url = embedded_test_server()->GetURL(kPrefetchPage);

    const base::string16 expected_title =
        expect_success ? base::ASCIIToUTF16("link onload")
                       : base::ASCIIToUTF16("link onerror");
    content::TitleWatcher title_watcher(
        browser->tab_strip_model()->GetActiveWebContents(), expected_title);
    ui_test_utils::NavigateToURL(browser, url);
    return expected_title == title_watcher.WaitAndGetTitle();
  }
};

// When initiated from the renderer, prefetch should be allowed regardless of
// the network type.
IN_PROC_BROWSER_TEST_F(PrefetchBrowserTest, PreferenceWorks) {
  // Set real NetworkChangeNotifier singleton aside.
  std::unique_ptr<NetworkChangeNotifier::DisableForTest> disable_for_test(
      new NetworkChangeNotifier::DisableForTest);

  // Preference defaults to ALWAYS.
  {
    std::unique_ptr<NetworkChangeNotifier> mock(
        new MockNetworkChangeNotifierWIFI);
    EXPECT_TRUE(RunPrefetchExperiment(true, browser()));
  }
  {
    std::unique_ptr<NetworkChangeNotifier> mock(
        new MockNetworkChangeNotifier4G);
    EXPECT_TRUE(RunPrefetchExperiment(true, browser()));
  }

  // Set preference to NEVER: prefetch should be unaffected.
  SetPreference(NetworkPredictionOptions::NETWORK_PREDICTION_NEVER);
  {
    std::unique_ptr<NetworkChangeNotifier> mock(
        new MockNetworkChangeNotifierWIFI);
    EXPECT_TRUE(RunPrefetchExperiment(true, browser()));
  }
  {
    std::unique_ptr<NetworkChangeNotifier> mock(
        new MockNetworkChangeNotifier4G);
    EXPECT_TRUE(RunPrefetchExperiment(true, browser()));
  }
}

// Bug 339909: When in incognito mode the browser crashed due to an
// uninitialized preference member. Verify that it no longer does.
IN_PROC_BROWSER_TEST_F(PrefetchBrowserTest, IncognitoTest) {
  Profile* incognito_profile = browser()->profile()->GetOffTheRecordProfile();
  Browser* incognito_browser =
      new Browser(Browser::CreateParams(incognito_profile, true));

  // Navigate just to have a tab in this window, otherwise there is no
  // WebContents for the incognito browser.
  OpenURLOffTheRecord(browser()->profile(), GURL("about:blank"));

  EXPECT_TRUE(RunPrefetchExperiment(true, incognito_browser));
}

}  // namespace
