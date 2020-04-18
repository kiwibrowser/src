// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_metrics_logger.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/resource_coordinator/tab_ranker/tab_features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_activity_simulator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::WebContentsTester;

// Sanity checks for functions in TabMetricsLogger.
// See TabActivityWatcherTest for more thorough tab usage UKM tests.
using TabMetricsLoggerTest = ChromeRenderViewHostTestHarness;

// Tests creating a flat TabFeatures structure for logging a tab and its
// TabMetrics state.
TEST_F(TabMetricsLoggerTest, TabFeatures) {
  TabActivitySimulator tab_activity_simulator;
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);
  TabStripModel* tab_strip_model = browser->tab_strip_model();

  // Add a foreground tab.
  tab_activity_simulator.AddWebContentsAndNavigate(tab_strip_model,
                                                   GURL("about://blank"));
  tab_strip_model->ActivateTabAt(0, false);

  // Add a background tab to test.
  content::WebContents* bg_contents =
      tab_activity_simulator.AddWebContentsAndNavigate(
          tab_strip_model, GURL("http://example.com/test.html"));
  WebContentsTester::For(bg_contents)->TestSetIsLoading(false);

  {
    TabMetricsLogger::TabMetrics bg_metrics;
    bg_metrics.web_contents = bg_contents;
    bg_metrics.page_transition = ui::PAGE_TRANSITION_FORM_SUBMIT;

    base::TimeDelta inactive_duration = base::TimeDelta::FromSeconds(10);

    tab_ranker::TabFeatures bg_features = TabMetricsLogger::GetTabFeatures(
        browser.get(), bg_metrics, inactive_duration);
    EXPECT_EQ(bg_features.has_before_unload_handler, false);
    EXPECT_EQ(bg_features.has_form_entry, false);
    EXPECT_EQ(bg_features.host, "example.com");
    EXPECT_EQ(bg_features.is_pinned, false);
    EXPECT_EQ(bg_features.key_event_count, 0);
    EXPECT_EQ(bg_features.mouse_event_count, 0);
    EXPECT_EQ(bg_features.navigation_entry_count, 1);
    EXPECT_EQ(bg_features.num_reactivations, 0);
    ASSERT_TRUE(bg_features.page_transition_core_type.has_value());
    EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
        ui::PAGE_TRANSITION_FORM_SUBMIT,
        bg_features.page_transition_core_type.value()));
    EXPECT_EQ(bg_features.page_transition_from_address_bar, false);
    EXPECT_EQ(bg_features.page_transition_is_redirect, false);
    ASSERT_TRUE(bg_features.site_engagement_score.has_value());
    EXPECT_EQ(bg_features.site_engagement_score.value(), 0);
    EXPECT_EQ(bg_features.time_from_backgrounded,
              inactive_duration.InMilliseconds());
    EXPECT_EQ(bg_features.touch_event_count, 0);
    EXPECT_EQ(bg_features.was_recently_audible, false);
  }

  // Update tab features.
  ui::PageTransition page_transition = static_cast<ui::PageTransition>(
      ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);
  tab_activity_simulator.Navigate(bg_contents, GURL("https://www.chromium.org"),
                                  page_transition);
  tab_strip_model->SetTabPinned(1, true);
  SiteEngagementService::Get(profile())->ResetBaseScoreForURL(
      GURL("https://www.chromium.org"), 91);

  {
    TabMetricsLogger::TabMetrics bg_metrics;
    bg_metrics.web_contents = bg_contents;
    bg_metrics.page_transition = page_transition;
    bg_metrics.page_metrics.key_event_count = 3;
    bg_metrics.page_metrics.mouse_event_count = 42;
    bg_metrics.page_metrics.num_reactivations = 5;
    bg_metrics.page_metrics.touch_event_count = 10;

    base::TimeDelta inactive_duration = base::TimeDelta::FromSeconds(5);

    tab_ranker::TabFeatures bg_features = TabMetricsLogger::GetTabFeatures(
        browser.get(), bg_metrics, inactive_duration);
    EXPECT_EQ(bg_features.has_before_unload_handler, false);
    EXPECT_EQ(bg_features.has_form_entry, false);
    EXPECT_EQ(bg_features.host, "www.chromium.org");
    EXPECT_EQ(bg_features.is_pinned, true);
    EXPECT_EQ(bg_features.key_event_count, 3);
    EXPECT_EQ(bg_features.mouse_event_count, 42);
    EXPECT_EQ(bg_features.navigation_entry_count, 2);
    EXPECT_EQ(bg_features.num_reactivations, 5);
    ASSERT_TRUE(bg_features.page_transition_core_type.has_value());
    EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
        ui::PAGE_TRANSITION_LINK,
        bg_features.page_transition_core_type.value()));
    EXPECT_EQ(bg_features.page_transition_from_address_bar, true);
    EXPECT_EQ(bg_features.page_transition_is_redirect, false);
    ASSERT_TRUE(bg_features.site_engagement_score.has_value());
    // Site engagement score should round down to the nearest 10.
    EXPECT_EQ(bg_features.site_engagement_score.value(), 90);
    EXPECT_EQ(bg_features.time_from_backgrounded,
              inactive_duration.InMilliseconds());
    EXPECT_EQ(bg_features.touch_event_count, 10);
    EXPECT_EQ(bg_features.was_recently_audible, false);
  }

  tab_strip_model->CloseAllTabs();
}
