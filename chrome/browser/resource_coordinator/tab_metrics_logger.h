// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_METRICS_LOGGER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_METRICS_LOGGER_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/tab_metrics_event.pb.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "ui/base/page_transition_types.h"

class Browser;

namespace base {
class TimeDelta;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace tab_ranker {
struct MRUFeatures;
struct TabFeatures;
}  // namespace tab_ranker

// Logs metrics for a tab and its WebContents when requested.
// Must be used on the UI thread.
class TabMetricsLogger {
 public:
  // The state of the page loaded in a tab's main frame, starting since the last
  // navigation.
  struct PageMetrics {
    // Number of key events.
    int key_event_count = 0;
    // Number of mouse events.
    int mouse_event_count = 0;
    // Number of touch events.
    int touch_event_count = 0;
    // Number of times this tab has been reactivated.
    int num_reactivations = 0;
  };

  // The state of a tab.
  struct TabMetrics {
    content::WebContents* web_contents = nullptr;

    // Source of the last committed navigation.
    ui::PageTransition page_transition = ui::PAGE_TRANSITION_FIRST;

    // Per-page metrics of the state of the WebContents. Tracked since the
    // tab's last top-level navigation.
    PageMetrics page_metrics = {};
  };

  TabMetricsLogger();
  ~TabMetricsLogger();

  // Logs metrics for the tab with the given main frame WebContents. Does
  // nothing if |ukm_source_id| is zero.
  void LogBackgroundTab(ukm::SourceId ukm_source_id,
                        const TabMetrics& tab_metrics);

  // Logs TabManager.Background.ForegroundedOrClosed UKM for a tab that was
  // shown after being inactive.
  void LogBackgroundTabShown(ukm::SourceId ukm_source_id,
                             base::TimeDelta inactive_duration,
                             const tab_ranker::MRUFeatures& mru_metrics);

  // Logs TabManager.Background.ForegroundedOrClosed UKM for a tab that was
  // closed after being inactive.
  void LogBackgroundTabClosed(ukm::SourceId ukm_source_id,
                              base::TimeDelta inactive_duration,
                              const tab_ranker::MRUFeatures& mru_metrics);

  // Logs TabManager.TabLifetime UKM for a closed tab.
  void LogTabLifetime(ukm::SourceId ukm_source_id,
                      base::TimeDelta time_since_navigation);

  // Returns the ContentType that matches |mime_type|.
  static metrics::TabMetricsEvent::ContentType GetContentTypeFromMimeType(
      const std::string& mime_type);

  // Returns the site engagement score for the WebContents, rounded down to 10s
  // to limit granularity. Returns -1 if site engagement service is disabled.
  static int GetSiteEngagementScore(const content::WebContents* web_contents);

  // Creates TabFeatures for logging or scoring tabs.
  // A common function for populating these features ensures that the same
  // values are used for logging training examples to UKM and for locally
  // scoring tabs.
  static tab_ranker::TabFeatures GetTabFeatures(
      const Browser* browser,
      const TabMetrics& tab_metrics,
      base::TimeDelta inactive_duration);

 private:
  // A counter to be incremented and logged with each UKM entry, used to
  // indicate the order that events within the same report were logged.
  int sequence_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TabMetricsLogger);
};

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_METRICS_LOGGER_H_
