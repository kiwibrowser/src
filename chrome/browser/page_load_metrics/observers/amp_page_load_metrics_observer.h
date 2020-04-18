// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_AMP_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_AMP_PAGE_LOAD_METRICS_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "components/ukm/ukm_source.h"

namespace content {
class NavigationHandle;
}

// Observer responsible for recording page load metrics relevant to page served
// from the AMP cache. When AMP pages are served in a same page navigation, UMA
// is not recorded; this is typical for AMP pages navigated to from google.com.
// Navigations to AMP pages from the google search app or directly to the amp
// cache page will be tracked. Refreshing an AMP page served from google.com
// will be tracked.
class AMPPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  // If you add elements to this enum, make sure you update the enum value in
  // enums.xml. Only add elements to the end to prevent inconsistencies between
  // versions.
  enum class AMPViewType {
    NONE,
    AMP_CACHE,
    GOOGLE_SEARCH_AMP_VIEWER,
    GOOGLE_NEWS_AMP_VIEWER,

    // New values should be added before this final entry.
    AMP_VIEW_TYPE_LAST
  };

  static AMPViewType GetAMPViewType(const GURL& url);

  AMPPageLoadMetricsObserver();
  ~AMPPageLoadMetricsObserver() override;

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  void OnCommitSameDocumentNavigation(
      content::NavigationHandle* navigation_handle) override;
  void OnDomContentLoadedEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnFirstLayout(const page_load_metrics::mojom::PageLoadTiming& timing,
                     const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnParseStart(const page_load_metrics::mojom::PageLoadTiming& timing,
                    const page_load_metrics::PageLoadExtraInfo& info) override;

 private:
  GURL current_url_;
  AMPViewType view_type_ = AMPViewType::NONE;

  DISALLOW_COPY_AND_ASSIGN(AMPPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_AMP_PAGE_LOAD_METRICS_OBSERVER_H_
