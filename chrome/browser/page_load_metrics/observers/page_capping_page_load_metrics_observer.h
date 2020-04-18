// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_CAPPING_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_CAPPING_PAGE_LOAD_METRICS_OBSERVER_H_

#include <stdint.h>

#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

namespace content {
class WebContents;
}

// A class that tracks the data usage of a page load and triggers an infobar
// when the page load is above a certain threshold. The thresholds are field
// trial controlled and vary based on whether media has played on the page.
class PageCappingPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  PageCappingPageLoadMetricsObserver();
  ~PageCappingPageLoadMetricsObserver() override;

 private:
  // page_load_metrics::PageLoadMetricsObserver:
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  void MediaStartedPlaying(
      const content::WebContentsObserver::MediaPlayerInfo& video_type,
      bool is_in_main_frame) override;

  // Show the page capping infobar if it has not been shown before and the data
  // use is above the threshold.
  void MaybeCreate();

  // The current bytes threshold of the capping page triggering.
  base::Optional<int64_t> page_cap_;

  // The WebContents for this page load. |this| cannot outlive |web_contents|.
  content::WebContents* web_contents_ = nullptr;

  // The cumulative network body bytes used so far.
  int64_t network_bytes_ = 0;

  // Track if the infobar has already been shown from this observer.
  bool displayed_infobar_ = false;

  DISALLOW_COPY_AND_ASSIGN(PageCappingPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_CAPPING_PAGE_LOAD_METRICS_OBSERVER_H_
