// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_NOSCRIPT_PREVIEW_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_NOSCRIPT_PREVIEW_PAGE_LOAD_METRICS_OBSERVER_H_

#include <stdint.h>

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

namespace content {
class NavigationHandle;
}

namespace previews {

namespace noscript_preview_names {

extern const char kNavigationToLoadEvent[];
extern const char kNavigationToFirstContentfulPaint[];
extern const char kNavigationToFirstMeaningfulPaint[];
extern const char kParseBlockedOnScriptLoad[];
extern const char kParseDuration[];

extern const char kNumNetworkResources[];
extern const char kNetworkBytes[];

}  // namespace noscript_preview_names

// Observer responsible for recording page load metrics when a NoScript Preview
// is loaded in the page.
class NoScriptPreviewPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  NoScriptPreviewPageLoadMetricsObserver();
  ~NoScriptPreviewPageLoadMetricsObserver() override;

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  ObservePolicy FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;
  void OnComplete(const page_load_metrics::mojom::PageLoadTiming& timing,
                  const page_load_metrics::PageLoadExtraInfo& info) override;

 private:
  void RecordTimingMetrics(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info);

  // Records UMA of page size when the observer is about to be deleted.
  void RecordPageSizeUMA() const;

  int64_t num_network_resources_ = 0;
  int64_t network_bytes_ = 0;

  DISALLOW_COPY_AND_ASSIGN(NoScriptPreviewPageLoadMetricsObserver);
};

}  // namespace previews

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_NOSCRIPT_PREVIEW_PAGE_LOAD_METRICS_OBSERVER_H_
