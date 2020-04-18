// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"

#include <memory>

#include "base/macros.h"
#include "base/test/simple_test_tick_clock.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_activity_simulator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_ukm_test_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "components/ukm/content/source_url_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/test/web_contents_tester.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/mojom/ukm_interface.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"

using blink::WebInputEvent;
using content::WebContentsTester;
using ukm::builders::TabManager_TabMetrics;
using ForegroundedOrClosed =
    ukm::builders::TabManager_Background_ForegroundedOrClosed;

namespace resource_coordinator {
namespace {

// Test URLs need to be from different origins to test site engagement score.
const GURL kTestUrls[] = {
    GURL("https://test1.example.com"), GURL("https://test3.example.com"),
    GURL("https://test2.example.com"), GURL("https://test4.example.com"),
};

// The default metric values for a tab.
const UkmMetricMap kBasicMetricValues({
    {TabManager_TabMetrics::kHasBeforeUnloadHandlerName, 0},
    {TabManager_TabMetrics::kHasFormEntryName, 0},
    {TabManager_TabMetrics::kIsPinnedName, 0},
    {TabManager_TabMetrics::kKeyEventCountName, 0},
    {TabManager_TabMetrics::kMouseEventCountName, 0},
    {TabManager_TabMetrics::kSiteEngagementScoreName, 0},
    {TabManager_TabMetrics::kTouchEventCountName, 0},
    {TabManager_TabMetrics::kWasRecentlyAudibleName, 0},
});

blink::WebMouseEvent CreateMouseEvent(WebInputEvent::Type event_type) {
  return blink::WebMouseEvent(event_type, WebInputEvent::kNoModifiers,
                              WebInputEvent::GetStaticTimeStampForTests());
}

}  // namespace

// Base class for testing tab UKM (URL-Keyed Metrics) entries logged by
// TabMetricsLogger via TabActivityWatcher.
class TabActivityWatcherTest : public ChromeRenderViewHostTestHarness {
 public:
  TabActivityWatcherTest() {
    TabActivityWatcher::GetInstance()->ResetForTesting();
  }

  ~TabActivityWatcherTest() override = default;

  void TearDown() override {
    TabActivityWatcher::GetInstance()->ResetForTesting();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  UkmEntryChecker ukm_entry_checker_;
  TabActivitySimulator tab_activity_simulator_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabActivityWatcherTest);
};

// Tests TabManager.TabMetrics UKM entries generated when tabs are backgrounded.
class TabMetricsTest : public TabActivityWatcherTest {
 public:
  TabMetricsTest() = default;
  ~TabMetricsTest() override = default;

 protected:
  // Expects that a new TabMetrics event has been logged for |source_url|
  // with the expected metrics and the next available SequenceId.
  void ExpectNewEntry(const GURL& source_url,
                      const UkmMetricMap& expected_metrics) {
    ukm_entry_checker_.ExpectNewEntry(kEntryName, source_url, expected_metrics);

    const size_t num_entries = ukm_entry_checker_.NumEntries(kEntryName);
    const ukm::mojom::UkmEntry* last_entry =
        ukm_entry_checker_.LastUkmEntry(kEntryName);
    ukm::TestUkmRecorder::ExpectEntryMetric(
        last_entry, TabManager_TabMetrics::kSequenceIdName, num_entries);
  }

 protected:
  const char* kEntryName = TabManager_TabMetrics::kEntryName;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabMetricsTest);
};

TEST_F(TabMetricsTest, Basic) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* fg_contents =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);
  WebContentsTester::For(fg_contents)->TestSetIsLoading(false);

  // Adding, loading and activating a foreground tab doesn't trigger logging.
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));

  // The second web contents is added as a background tab, so it logs an entry
  // when it stops loading.
  content::WebContents* bg_contents =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[1]));
  WebContentsTester::For(bg_contents)->TestSetIsLoading(false);
  ExpectNewEntry(kTestUrls[1], kBasicMetricValues);

  // Activating a tab logs the deactivated tab.
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], kBasicMetricValues);
  }

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], kBasicMetricValues);
  }

  // Closing the tabs destroys the WebContentses but should not trigger logging.
  // The TestWebContentsObserver simulates hiding these tabs as they are closed;
  // we verify in TearDown() that no logging occurred.
  tab_strip_model->CloseAllTabs();
}

// Tests when tab events like pinning and navigating trigger logging.
TEST_F(TabMetricsTest, TabEvents) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* test_contents_1 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);

  // Opening the background tab triggers logging once the page finishes loading.
  content::WebContents* test_contents_2 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[1]));
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));
  WebContentsTester::For(test_contents_2)->TestSetIsLoading(false);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(GURL(kTestUrls[1]), kBasicMetricValues);
  }

  // Navigating the active tab doesn't trigger logging.
  WebContentsTester::For(test_contents_1)->NavigateAndCommit(kTestUrls[2]);
  WebContentsTester::For(test_contents_1)->TestSetIsLoading(false);
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));

  // Pinning the active tab doesn't trigger logging.
  tab_strip_model->SetTabPinned(0, true);
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));

  // Pinning and unpinning the background tab triggers logging.
  tab_strip_model->SetTabPinned(1, true);
  UkmMetricMap expected_metrics(kBasicMetricValues);
  expected_metrics[TabManager_TabMetrics::kIsPinnedName] = 1;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(GURL(kTestUrls[1]), expected_metrics);
  }
  tab_strip_model->SetTabPinned(1, false);
  expected_metrics[TabManager_TabMetrics::kIsPinnedName] = 0;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(GURL(kTestUrls[1]), kBasicMetricValues);
  }

  // Navigating the background tab triggers logging once the page finishes
  // loading.
  WebContentsTester::For(test_contents_2)->NavigateAndCommit(kTestUrls[0]);
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));
  WebContentsTester::For(test_contents_2)->TestSetIsLoading(false);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(GURL(kTestUrls[0]), kBasicMetricValues);
  }

  tab_strip_model->CloseAllTabs();
}

// Tests setting and changing tab metrics.
TEST_F(TabMetricsTest, TabMetrics) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* test_contents_1 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);

  // Expected metrics for tab event.
  UkmMetricMap expected_metrics(kBasicMetricValues);

  // Load background contents and verify UKM entry.
  content::WebContents* test_contents_2 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[1]));
  WebContentsTester::For(test_contents_2)->TestSetIsLoading(false);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics);
  }

  // Site engagement score should round down to the nearest 10.
  SiteEngagementService::Get(profile())->ResetBaseScoreForURL(kTestUrls[1], 45);
  expected_metrics[TabManager_TabMetrics::kSiteEngagementScoreName] = 40;

  WebContentsTester::For(test_contents_2)->SetWasRecentlyAudible(true);
  expected_metrics[TabManager_TabMetrics::kWasRecentlyAudibleName] = 1;

  // Pin the background tab to log an event. (This moves it to index 0.)
  tab_strip_model->SetTabPinned(1, true);
  expected_metrics[TabManager_TabMetrics::kIsPinnedName] = 1;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics);
  }

  // Unset WasRecentlyAudible and navigate the background tab to a new domain.
  // Site engagement score for the new domain is 0.
  WebContentsTester::For(test_contents_2)->SetWasRecentlyAudible(false);
  expected_metrics[TabManager_TabMetrics::kWasRecentlyAudibleName] = 0;
  WebContentsTester::For(test_contents_2)->NavigateAndCommit(kTestUrls[2]);
  expected_metrics[TabManager_TabMetrics::kSiteEngagementScoreName] = 0;

  WebContentsTester::For(test_contents_2)->TestSetIsLoading(false);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[2], expected_metrics);
  }

  // Navigate the active tab and switch away from it. The entry should reflect
  // the new URL (even when the page hasn't finished loading).
  WebContentsTester::For(test_contents_1)->NavigateAndCommit(kTestUrls[2]);
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    // This tab still has the default metrics.
    ExpectNewEntry(kTestUrls[2], kBasicMetricValues);
  }

  tab_strip_model->CloseAllTabs();
}

// Tests counting input events. TODO(michaelpg): Currently only tests mouse
// events.
TEST_F(TabMetricsTest, InputEvents) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* test_contents_1 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  content::WebContents* test_contents_2 =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[1]));

  // RunUntilIdle is needed because the widget input handler is initialized
  // asynchronously via mojo (see SetupWidgetInputHandler).
  base::RunLoop().RunUntilIdle();
  tab_strip_model->ActivateTabAt(0, false);

  UkmMetricMap expected_metrics_1(kBasicMetricValues);
  UkmMetricMap expected_metrics_2(kBasicMetricValues);

  // Fake some input events.
  content::RenderWidgetHost* widget_1 =
      test_contents_1->GetRenderViewHost()->GetWidget();
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseDown));
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseUp));
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseMove));
  expected_metrics_1[TabManager_TabMetrics::kMouseEventCountName] = 3;

  // Switch to the background tab. The current tab is deactivated and logged.
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], expected_metrics_1);
  }

  // The second tab's counts are independent of the other's.
  content::RenderWidgetHost* widget_2 =
      test_contents_2->GetRenderViewHost()->GetWidget();
  widget_2->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseMove));
  expected_metrics_2[TabManager_TabMetrics::kMouseEventCountName] = 1;

  // Switch back to the first tab to log the second tab.
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics_2);
  }

  // New events are added to the first tab's existing counts.
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseMove));
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseMove));
  expected_metrics_1[TabManager_TabMetrics::kMouseEventCountName] = 5;
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], expected_metrics_1);
  }
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics_2);
  }

  // After a navigation, test that the counts are reset.
  WebContentsTester::For(test_contents_1)->NavigateAndCommit(kTestUrls[2]);
  // The widget may have been invalidated by the navigation.
  widget_1 = test_contents_1->GetRenderViewHost()->GetWidget();
  widget_1->ForwardMouseEvent(CreateMouseEvent(WebInputEvent::kMouseMove));
  expected_metrics_1[TabManager_TabMetrics::kMouseEventCountName] = 1;
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[2], expected_metrics_1);
  }

  tab_strip_model->CloseAllTabs();
}

// Tests that logging doesn't occur when the WebContents is hidden while still
// the active tab, e.g. when the browser window hides before closing.
TEST_F(TabMetricsTest, HideWebContents) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* test_contents =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);

  // Hiding the window doesn't trigger a log entry, unless the window was
  // minimized.
  // TODO(michaelpg): Test again with the window minimized using the
  // FakeBrowserWindow from window_activity_watcher_unittest.cc.
  test_contents->WasHidden();
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));
  test_contents->WasShown();
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));

  tab_strip_model->CloseAllTabs();
}

// Tests navigation-related metrics.
TEST_F(TabMetricsTest, Navigations) {
  Browser::CreateParams params(profile(), true);
  auto browser = CreateBrowserWithTestWindowForParams(&params);
  TabStripModel* tab_strip_model = browser->tab_strip_model();

  // Set up first tab.
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);

  // Expected metrics for tab event.
  UkmMetricMap expected_metrics(kBasicMetricValues);

  // Load background contents and verify UKM entry.
  content::WebContents* test_contents =
      tab_activity_simulator_.AddWebContentsAndNavigate(
          tab_strip_model, GURL(kTestUrls[1]),
          ui::PageTransitionFromInt(ui::PAGE_TRANSITION_TYPED |
                                    ui::PAGE_TRANSITION_FROM_ADDRESS_BAR));
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      base::nullopt;
  expected_metrics[TabManager_TabMetrics::kPageTransitionFromAddressBarName] =
      true;
  expected_metrics[TabManager_TabMetrics::kPageTransitionIsRedirectName] =
      false;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName] = 1;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics);
  }

  // Navigate background tab (not all transition types make sense in the
  // background, but this is simpler than juggling two tabs to trigger logging).
  tab_activity_simulator_.Navigate(test_contents, kTestUrls[2],
                                   ui::PAGE_TRANSITION_LINK);
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      ui::PAGE_TRANSITION_LINK;
  expected_metrics[TabManager_TabMetrics::kPageTransitionFromAddressBarName] =
      false;
  expected_metrics[TabManager_TabMetrics::kPageTransitionIsRedirectName] =
      false;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[2], expected_metrics);
  }

  tab_activity_simulator_.Navigate(
      test_contents, kTestUrls[0],
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_LINK |
                                ui::PAGE_TRANSITION_SERVER_REDIRECT));
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      ui::PAGE_TRANSITION_LINK;
  expected_metrics[TabManager_TabMetrics::kPageTransitionFromAddressBarName] =
      false;
  expected_metrics[TabManager_TabMetrics::kPageTransitionIsRedirectName] = true;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], expected_metrics);
  }

  tab_activity_simulator_.Navigate(test_contents, kTestUrls[0],
                                   ui::PAGE_TRANSITION_RELOAD);
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      ui::PAGE_TRANSITION_RELOAD;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  expected_metrics[TabManager_TabMetrics::kPageTransitionFromAddressBarName] =
      false;
  expected_metrics[TabManager_TabMetrics::kPageTransitionIsRedirectName] =
      false;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], expected_metrics);
  }

  tab_activity_simulator_.Navigate(test_contents, kTestUrls[1],
                                   ui::PAGE_TRANSITION_AUTO_BOOKMARK);
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      ui::PAGE_TRANSITION_AUTO_BOOKMARK;
  // FromAddressBar and IsRedirect should still be false, no need to update
  // their values in |expected_metrics|.
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics);
  }

  tab_activity_simulator_.Navigate(test_contents, kTestUrls[1],
                                   ui::PAGE_TRANSITION_FORM_SUBMIT);
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      ui::PAGE_TRANSITION_FORM_SUBMIT;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], expected_metrics);
  }

  // Test non-reportable core type.
  tab_activity_simulator_.Navigate(
      test_contents, kTestUrls[0],
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_KEYWORD |
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR));
  WebContentsTester::For(test_contents)->TestSetIsLoading(false);
  expected_metrics[TabManager_TabMetrics::kPageTransitionCoreTypeName] =
      base::nullopt;
  expected_metrics[TabManager_TabMetrics::kPageTransitionFromAddressBarName] =
      true;
  expected_metrics[TabManager_TabMetrics::kNavigationEntryCountName].value()++;
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[0], expected_metrics);
  }

  tab_strip_model->CloseAllTabs();
}

// Tests that replacing a foreground tab doesn't log new tab metrics until the
// new tab is backgrounded.
TEST_F(TabMetricsTest, ReplaceForegroundTab) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* orig_contents =
      tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                        GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);
  WebContentsTester::For(orig_contents)->TestSetIsLoading(false);

  // Build the replacement contents.
  std::unique_ptr<content::WebContents> new_contents =
      tab_activity_simulator_.CreateWebContents(profile());

  // Ensure the test URL gets a UKM source ID upon navigating.
  // Normally this happens when the browser or prerenderer attaches tab helpers.
  ukm::InitializeSourceUrlRecorderForWebContents(new_contents.get());

  tab_activity_simulator_.Navigate(new_contents.get(), GURL(kTestUrls[1]));
  WebContentsTester::For(new_contents.get())->TestSetIsLoading(false);

  // Replace and delete the old contents.
  std::unique_ptr<content::WebContents> old_contents =
      tab_strip_model->ReplaceWebContentsAt(0, std::move(new_contents));
  ASSERT_EQ(old_contents.get(), orig_contents);
  old_contents.reset();
  tab_strip_model->GetWebContentsAt(0)->WasShown();

  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));

  // Add a new tab so the first tab is backgrounded.
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[2]));
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ExpectNewEntry(kTestUrls[1], kBasicMetricValues);
  }

  tab_strip_model->CloseAllTabs();
}

// Tests TabManager.Background.ForegroundedOrClosed UKMs logged by
// TabActivityWatcher.
class ForegroundedOrClosedTest : public TabActivityWatcherTest {
 public:
  ForegroundedOrClosedTest()
      : scoped_set_tick_clock_for_testing_(&test_clock_) {
    // Start at a nonzero time.
    AdvanceClock();
  }
  ~ForegroundedOrClosedTest() override = default;

 protected:
  const char* kEntryName = ForegroundedOrClosed::kEntryName;

  void AdvanceClock() { test_clock_.Advance(base::TimeDelta::FromSeconds(1)); }

 private:
  base::SimpleTestTickClock test_clock_;
  resource_coordinator::ScopedSetTickClockForTesting
      scoped_set_tick_clock_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(ForegroundedOrClosedTest);
};

// TODO(michaelpg): test is flaky on ChromeOS. https://crbug.com/839886
#if defined(OS_CHROMEOS)
#define MAYBE_SingleTab DISABLED_SingleTab
#else
#define MAYBE_SingleTab SingleTab
#endif
// Tests TabManager.Backgrounded.ForegroundedOrClosed UKM logging.
TEST_F(ForegroundedOrClosedTest, MAYBE_SingleTab) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[0]));

  // The tab is in the foreground, so it isn't logged as a background tab.
  tab_strip_model->CloseWebContentsAt(0, TabStripModel::CLOSE_USER_GESTURE);
  EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));
}

// Tests TabManager.Backgrounded.ForegroundedOrClosed UKM logging.
TEST_F(ForegroundedOrClosedTest, MultipleTabs) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[1]));
  AdvanceClock();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[2]));
  AdvanceClock();
  // MRU ordering by tab indices:
  // 0 (foreground), 1 (created first), 2 (created last)

  // Foreground a tab to log an event.
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 2);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[2],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 1},
        });
  }
  AdvanceClock();
  // MRU ordering by tab indices:
  // 2 (foreground), 0 (foregrounded earlier), 1 (never foregrounded)

  // Foreground the middle tab to log another event.
  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[1],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 2},
        });
  }
  AdvanceClock();
  // MRU ordering by tab indices:
  // 1 (foreground), 2 (foregrounded earlier), 0 (foregrounded even earlier)

  // Close all tabs. Background tabs are logged as closed.
  tab_strip_model->CloseAllTabs();
  {
    SCOPED_TRACE("");
    // The rightmost tab was in the background and was closed.
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[2],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 0},
            // TODO(michaelpg): The final tab has an MRU of 0 because the
            // remaining tabs were closed first. It would be more accurate to
            // use the MRUIndex this tab had when CloseAllTabs() was called.
            // See https://crbug.com/817174.
            {ForegroundedOrClosed::kMRUIndexName, 0},
        });

    // The leftmost tab was in the background and was closed.
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[0],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 0},
            // TODO(michaelpg): The final tab has an MRU of 0 because the
            // remaining tabs were closed first. It would be more accurate to
            // use the MRUIndex this tab had when CloseAllTabs() was called.
            // See https://crbug.com/817174.
            {ForegroundedOrClosed::kMRUIndexName, 0},
        });

    // No event is logged for the middle tab, which was in the foreground.
    EXPECT_EQ(0, ukm_entry_checker_.NumNewEntriesRecorded(kEntryName));
  }
}

// Tests the MRUIndex value for ForegroundedOrClosed events.
TEST_F(ForegroundedOrClosedTest, MRUIndex) {
  Browser::CreateParams params(profile(), true);
  std::unique_ptr<Browser> browser =
      CreateBrowserWithTestWindowForParams(&params);

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[0]));
  tab_strip_model->ActivateTabAt(0, false);
  AdvanceClock();

  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[1]));
  AdvanceClock();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[2]));
  AdvanceClock();
  tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model,
                                                    GURL(kTestUrls[3]));
  AdvanceClock();

  // Tabs in MRU order: [0, 3, 2, 1]
  // The 0th tab is foregrounded. The other tabs are ordered by most recently
  // created since they haven't ever been foregrounded.

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 2);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[2],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 2},
        });
  }
  AdvanceClock();
  // New MRU order: [2, 0, 3, 1]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[0],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 1},
        });
  }
  AdvanceClock();
  // New MRU order: [0, 2, 3, 1]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[1],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 3},
        });
  }
  AdvanceClock();
  // New MRU order: [1, 0, 2, 3]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 0);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[0],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 1},
        });
  }
  AdvanceClock();
  // New MRU order: [0, 1, 2, 3]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 3);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[3],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 3},
        });
  }
  AdvanceClock();
  // New MRU order: [3, 0, 1, 2]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 1);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[1],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 2},
        });
  }
  AdvanceClock();
  // New MRU order: [1, 3, 0, 2]

  // Close a background tab.
  tab_strip_model->CloseWebContentsAt(3, TabStripModel::CLOSE_NONE);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[3],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 0},
            {ForegroundedOrClosed::kMRUIndexName, 1},
        });
  }
  AdvanceClock();
  // New MRU order: [1, 0, 2]

  tab_activity_simulator_.SwitchToTabAt(tab_strip_model, 2);
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[2],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 2},
        });
  }
  AdvanceClock();
  // New MRU order: [2, 1, 0]

  // Close a foreground tab.
  tab_strip_model->CloseWebContentsAt(2, TabStripModel::CLOSE_NONE);
  // This activates the next tab in the tabstrip. Since this is a
  // TestWebContents, we must manually call WasShown().
  tab_strip_model->GetWebContentsAt(1)->WasShown();
  {
    SCOPED_TRACE("");
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[1],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 1},
            {ForegroundedOrClosed::kMRUIndexName, 0},
        });
  }
  AdvanceClock();
  // New MRU order: [1, 0]

  tab_strip_model->CloseAllTabs();
  {
    SCOPED_TRACE("");
    // The rightmost tab was in the foreground, so only the leftmost tab is
    // logged.
    // TODO(michaelpg): The last tab has an MRU of 0 because the remaining tabs
    // were closed first. It would be more accurate to use the MRUIndex this tab
    // had when CloseAllTabs() was called. See https://crbug.com/817174.
    ukm_entry_checker_.ExpectNewEntry(
        kEntryName, kTestUrls[0],
        {
            {ForegroundedOrClosed::kIsForegroundedName, 0},
            {ForegroundedOrClosed::kMRUIndexName, 0},
        });
  }
}

}  // namespace resource_coordinator
