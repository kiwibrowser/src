// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

class ChromeDoNotTrackTest : public InProcessBrowserTest {
 protected:
  void TestDoNoTrack(bool enabled) {
    ASSERT_TRUE(embedded_test_server()->Start());

    PrefService* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(prefs::kEnableDoNotTrack, enabled);

    GURL url = embedded_test_server()->GetURL("/echo");
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(enabled, browser()
                           ->tab_strip_model()
                           ->GetActiveWebContents()
                           ->GetMutableRendererPrefs()
                           ->enable_do_not_track);
  }
};

// Checks that the DNT preference is set on the WebContents when navigating.

IN_PROC_BROWSER_TEST_F(ChromeDoNotTrackTest, NotEnabled) {
  TestDoNoTrack(/*enabled=*/false);
}

IN_PROC_BROWSER_TEST_F(ChromeDoNotTrackTest, Enabled) {
  TestDoNoTrack(/*enabled=*/true);
}

}  // namespace
