// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

class InterstitialUITest : public InProcessBrowserTest {
 public:
   InterstitialUITest() {}
   ~InterstitialUITest() override {}

 protected:
  void TestInterstitial(GURL url, const std::string& page_title) {
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(
      base::ASCIIToUTF16(page_title),
      browser()->tab_strip_model()->GetActiveWebContents()->GetTitle());

    // Should also be able to open and close devtools.
    DevToolsWindow* window =
        DevToolsWindowTesting::OpenDevToolsWindowSync(browser(), true);
    EXPECT_TRUE(window);
    DevToolsWindowTesting::CloseDevToolsWindowSync(window);
  }
};

IN_PROC_BROWSER_TEST_F(InterstitialUITest, HomePage) {
  TestInterstitial(
      GURL("chrome://interstitials"),
      "Interstitials");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, InvalidURLShouldOpenHomePage) {
  // Invalid path should open the main page:
  TestInterstitial(
      GURL("chrome://interstitials/--invalid--"),
      "Interstitials");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest,
                       InvalidURLMatchingStartOfValidURLShouldBeInvalid) {
  // Path that matches the first characters of another should be invalid
  // (and therefore open the main page).
  TestInterstitial(GURL("chrome://interstitials/ssl--invalid--"),
                   "Interstitials");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, SSLInterstitial) {
  TestInterstitial(
      GURL("chrome://interstitials/ssl"),
      "Privacy error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, SuperfishInterstitial) {
  TestInterstitial(GURL("chrome://interstitials/superfish-ssl"),
                   "Privacy error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, MITMSoftwareInterstitial) {
  TestInterstitial(GURL("chrome://interstitials/mitm-software-ssl"),
                   "Privacy error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, PinnedCertInterstitial) {
  TestInterstitial(GURL("chrome://interstitials/ssl?type=hpkp_failure"),
                   "Privacy error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, CTInterstitial) {
  TestInterstitial(GURL("chrome://interstitials/ssl?type=ct_failure"),
                   "Privacy error");
  bool found_ct_error = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send(document.body.textContent.indexOf('"
      "CERTIFICATE_TRANSPARENCY') != -1);",
      &found_ct_error));
  EXPECT_TRUE(found_ct_error);
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, MalwareInterstitial) {
  TestInterstitial(
      GURL("chrome://interstitials/safebrowsing?type=malware"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, PhishingInterstitial) {
  TestInterstitial(
      GURL("chrome://interstitials/safebrowsing?type=phishing"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, MalwareInterstitialQuiet) {
  TestInterstitial(
      GURL("chrome://interstitials/quietsafebrowsing?type=malware"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, PhishingInterstitialQuiet) {
  TestInterstitial(
      GURL("chrome://interstitials/quietsafebrowsing?type=phishing"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, ClientsideMalwareInterstitial) {
  TestInterstitial(
      GURL("chrome://interstitials/safebrowsing?type=clientside_malware"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, ClientsidePhishingInterstitial) {
  TestInterstitial(
      GURL("chrome://interstitials/safebrowsing?type=clientside_phishing"),
      "Security error");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, CaptivePortalInterstitial) {
  TestInterstitial(GURL("chrome://interstitials/captiveportal"),
                   "Connect to network");
}

IN_PROC_BROWSER_TEST_F(InterstitialUITest, CaptivePortalInterstitialWifi) {
  TestInterstitial(GURL("chrome://interstitials/captiveportal?is_wifi=1"),
                   "Connect to Wi-Fi");
}

// Tests that back button works after opening an interstitial from
// chrome://interstitials.
IN_PROC_BROWSER_TEST_F(InterstitialUITest, InterstitialBackButton) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://interstitials"));
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://interstitials/ssl"));
  content::TestNavigationObserver navigation_observer(web_contents);
  chrome::GoBack(browser(), WindowOpenDisposition::CURRENT_TAB);
  navigation_observer.Wait();
  base::string16 title;
  ui_test_utils::GetCurrentTabTitle(browser(), &title);
  EXPECT_EQ(title, base::ASCIIToUTF16("Interstitials"));
}

// Tests that view-source: works correctly on chrome://interstitials.
IN_PROC_BROWSER_TEST_F(InterstitialUITest, InterstitialViewSource) {
  ui_test_utils::NavigateToURL(browser(),
                               GURL("view-source:chrome://interstitials/"));
  int found;
  base::string16 expected_title =
      base::ASCIIToUTF16("<title>Interstitials</title>");
  found = ui_test_utils::FindInPage(
      browser()->tab_strip_model()->GetActiveWebContents(), expected_title,
      true, /* Forward */
      true, /* case_sensitive */
      nullptr, nullptr);
  EXPECT_EQ(found, 1);
}

// Tests that view-source: works correctly on a subpage of
// chrome://interstitials (using chrome://interstitials/ssl).
IN_PROC_BROWSER_TEST_F(InterstitialUITest, InterstitialWithPathViewSource) {
  ui_test_utils::NavigateToURL(browser(),
                               GURL("view-source:chrome://interstitials/ssl"));
  int found;
  base::string16 expected_title =
      base::ASCIIToUTF16("<title>Privacy error</title");
  found = ui_test_utils::FindInPage(
      browser()->tab_strip_model()->GetActiveWebContents(), expected_title,
      true, /* Forward */
      true, /* case_sensitive */
      nullptr, nullptr);
  EXPECT_EQ(found, 1);
}

// Checks that the interstitial page uses correct web contents. If not, closing
// the tab might result in a freed web contents pointer and cause a crash.
// See https://crbug.com/611706 for details.
IN_PROC_BROWSER_TEST_F(InterstitialUITest, UseCorrectWebContents) {
  int current_tab = browser()->tab_strip_model()->active_index();
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://interstitials/ssl"));
  // Duplicate the tab and close it.
  chrome::DuplicateTab(browser());
  EXPECT_NE(current_tab, browser()->tab_strip_model()->active_index());
  chrome::CloseTab(browser());
  EXPECT_EQ(current_tab, browser()->tab_strip_model()->active_index());

  // Reloading the page shouldn't cause a crash.
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
}
