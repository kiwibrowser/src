// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INTERSTITIALS_SECURITY_INTERSTITIAL_PAGE_TEST_UTILS_H_
#define CHROME_BROWSER_INTERSTITIALS_SECURITY_INTERSTITIAL_PAGE_TEST_UTILS_H_

#include <string>

#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
class WebContents;
}

class GURL;

namespace security_interstitials {
class SecurityInterstitialPage;
}

namespace chrome_browser_interstitials {

// Looks for text in the |textContent| of |interstitial_frame|'s body and
// returns true if found. This can be used for either transient or committed
// interstitials. For the former, pass
// web_contents->GetInterstitialPage()->GetMainFrame() as the first argument,
// and for the latter, just pass web_contents->GetMainFrame().
bool IsInterstitialDisplayingText(content::RenderFrameHost* interstitial_frame,
                                  const std::string& text);

// This class is used for testing the display of IDN names in security
// interstitials.
class SecurityInterstitialIDNTest : public InProcessBrowserTest {
 public:
  // InProcessBrowserTest implementation
  void SetUpOnMainThread() override;

  // Run a test that creates an interstitial with an IDN request URL
  // and checks that it is properly decoded.
  testing::AssertionResult VerifyIDNDecoded() const;

 protected:
  virtual security_interstitials::SecurityInterstitialPage* CreateInterstitial(
      content::WebContents* contents,
      const GURL& request_url) const = 0;
};

}  // namespace chrome_browser_interstitials

#endif  // CHROME_BROWSER_INTERSTITIALS_SECURITY_INTERSTITIAL_PAGE_TEST_UTILS_H_
