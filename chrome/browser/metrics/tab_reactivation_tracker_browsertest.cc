// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_reactivation_tracker.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"

namespace metrics {

// Simple test fixture that just counts the notification received by a
// TabReactivationTracker.
class TabReactivationTrackerTest : public InProcessBrowserTest,
                                   public TabReactivationTracker::Delegate {
 public:
  TabReactivationTrackerTest()
      : tab_reactivation_count_(0), tab_deactivation_count_(0) {}

  // TabReactivationTracker::Delegate:
  void OnTabReactivated(content::WebContents* contents) override;
  void OnTabDeactivated(content::WebContents* contents) override;

  int tab_reactivation_count() { return tab_reactivation_count_; }
  int tab_deactivation_count() { return tab_deactivation_count_; }

 private:
  int tab_reactivation_count_;
  int tab_deactivation_count_;
};

void TabReactivationTrackerTest::OnTabReactivated(
    content::WebContents* contents) {
  tab_reactivation_count_++;
}

void TabReactivationTrackerTest::OnTabDeactivated(
    content::WebContents* contents) {
  tab_deactivation_count_++;
}

IN_PROC_BROWSER_TEST_F(TabReactivationTrackerTest, CorrectTracking) {
  content::OpenURLParams open1(
      GURL(chrome::kChromeUIAboutURL), content::Referrer(),
      WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_TYPED, false);

  content::OpenURLParams open2(GURL(chrome::kChromeUIAboutURL),
                               content::Referrer(),
                               WindowOpenDisposition::NEW_FOREGROUND_TAB,
                               ui::PAGE_TRANSITION_TYPED, false);

  TabReactivationTracker tab_reactivation_tracker(this);

  EXPECT_EQ(0, tab_reactivation_count());
  EXPECT_EQ(0, tab_deactivation_count());

  // Open one tab.
  browser()->OpenURL(open1);
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(0, tab_reactivation_count());
  EXPECT_EQ(0, tab_deactivation_count());

  // Open a second tab.
  browser()->OpenURL(open2);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());
  EXPECT_EQ(0, tab_reactivation_count());
  EXPECT_EQ(1, tab_deactivation_count());

  // Reactivate the first tab.
  browser()->tab_strip_model()->ActivateTabAt(0, false);
  EXPECT_EQ(1, tab_reactivation_count());
  EXPECT_EQ(2, tab_deactivation_count());

  // Closing a tab doesn't count as a deactivation.
  browser()->tab_strip_model()->CloseWebContentsAt(1, 0);
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(1, tab_reactivation_count());
  EXPECT_EQ(2, tab_deactivation_count());
}

}  // namespace metrics
