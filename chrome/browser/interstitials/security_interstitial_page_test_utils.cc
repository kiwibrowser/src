// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/core/controller_client.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chrome_browser_interstitials {

bool IsInterstitialDisplayingText(content::RenderFrameHost* interstitial_frame,
                                  const std::string& text) {
  // It's valid for |text| to contain "\'", but simply look for "'" instead
  // since this function is used for searching host names and a predefined
  // string.
  DCHECK(text.find("\'") == std::string::npos);
  std::string command = base::StringPrintf(
      "var hasText = document.body.textContent.indexOf('%s') >= 0;"
      "window.domAutomationController.send(hasText ? %d : %d);",
      text.c_str(), security_interstitials::CMD_TEXT_FOUND,
      security_interstitials::CMD_TEXT_NOT_FOUND);
  int result = 0;
  EXPECT_TRUE(content::ExecuteScriptAndExtractInt(interstitial_frame, command,
                                                  &result));
  return result == security_interstitials::CMD_TEXT_FOUND;
}

void SecurityInterstitialIDNTest::SetUpOnMainThread() {
  // Clear AcceptLanguages to force punycode decoding.
  browser()->profile()->GetPrefs()->SetString(prefs::kAcceptLanguages,
                                              std::string());
}

testing::AssertionResult SecurityInterstitialIDNTest::VerifyIDNDecoded() const {
  const char kHostname[] = "xn--d1abbgf6aiiy.xn--p1ai";
  const char kHostnameJSUnicode[] =
      "\\u043f\\u0440\\u0435\\u0437\\u0438\\u0434\\u0435\\u043d\\u0442."
      "\\u0440\\u0444";
  std::string request_url_spec = base::StringPrintf("https://%s/", kHostname);
  GURL request_url(request_url_spec);

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  DCHECK(contents);
  security_interstitials::SecurityInterstitialPage* blocking_page =
      CreateInterstitial(contents, request_url);
  blocking_page->Show();

  WaitForInterstitialAttach(contents);
  if (!WaitForRenderFrameReady(contents->GetInterstitialPage()->GetMainFrame()))
    return testing::AssertionFailure() << "Render frame not ready";

  if (IsInterstitialDisplayingText(
          contents->GetInterstitialPage()->GetMainFrame(),
          kHostnameJSUnicode)) {
    return testing::AssertionSuccess();
  }
  return testing::AssertionFailure() << "Interstitial not displaying text";
}
}  // namespace chrome_browser_interstitials
