// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOFI_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOFI_PAGE_LOAD_METRICS_OBSERVER_H_

#include <stdint.h>

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

namespace content {
class NavigationHandle;
}

namespace data_reduction_proxy {

namespace lofi_names {

extern const char kNavigationToLoadEvent[];
extern const char kNavigationToFirstContentfulPaint[];
extern const char kNavigationToFirstMeaningfulPaint[];
extern const char kNavigationToFirstImagePaint[];
extern const char kParseBlockedOnScriptLoad[];
extern const char kParseDuration[];

extern const char kNumNetworkResources[];
extern const char kNumNetworkLoFiResources[];
extern const char kNetworkBytes[];
extern const char kLoFiNetworkBytes[];

}  // namespace lofi_names

// Observer responsible for recording page load metrics when a LoFi image is
// loaded in the page. LoFi can happen via the data saver proxy or via a client
// optimization that requests a range header.
class LoFiPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  LoFiPageLoadMetricsObserver();
  ~LoFiPageLoadMetricsObserver() override;

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  ObservePolicy FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnComplete(const page_load_metrics::mojom::PageLoadTiming& timing,
                  const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;

 private:
  void RecordTimingMetrics(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info);

  // Records UMA of page size when the observer is about to be deleted.
  void RecordPageSizeUMA() const;

  int64_t num_network_resources_;
  int64_t num_network_lofi_resources_;
  int64_t original_network_bytes_;
  int64_t network_bytes_;
  int64_t lofi_network_bytes_;

  DISALLOW_COPY_AND_ASSIGN(LoFiPageLoadMetricsObserver);
};

}  // namespace data_reduction_proxy

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_LOFI_PAGE_LOAD_METRICS_OBSERVER_H_
