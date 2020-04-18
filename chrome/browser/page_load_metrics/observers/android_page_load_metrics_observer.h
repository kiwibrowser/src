// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_

#include <jni.h>

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "net/nqe/network_quality_estimator.h"

namespace content {
class WebContents;
}  // namespace content

class GURL;

/** Forwards page load metrics to the Java side on Android. */
class AndroidPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit AndroidPageLoadMetricsObserver(content::WebContents* web_contents);

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFirstMeaningfulPaintInMainFrameDocument(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnLoadEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;

 protected:
  AndroidPageLoadMetricsObserver(
      content::WebContents* web_contents,
      net::NetworkQualityEstimator::NetworkQualityProvider*
          network_quality_provider)
      : web_contents_(web_contents),
        network_quality_provider_(network_quality_provider) {}

  virtual void ReportNewNavigation();

  virtual void ReportNetworkQualityEstimate(
      net::EffectiveConnectionType connection_type,
      int64_t http_rtt_ms,
      int64_t transport_rtt_ms);

  virtual void ReportFirstContentfulPaint(int64_t navigation_start_tick,
                                          int64_t first_contentful_paint_ms);

  virtual void ReportFirstMeaningfulPaint(int64_t navigation_start_tick,
                                          int64_t first_meaningful_paint_ms);

  virtual void ReportLoadEventStart(int64_t navigation_start_tick,
                                    int64_t load_event_start_ms);

  virtual void ReportLoadedMainResource(int64_t dns_start_ms,
                                        int64_t dns_end_ms,
                                        int64_t connect_start_ms,
                                        int64_t connect_end_ms,
                                        int64_t request_start_ms,
                                        int64_t send_start_ms,
                                        int64_t send_end_ms);

 private:
  content::WebContents* web_contents_;

  bool did_dispatch_on_main_resource_ = false;
  int64_t navigation_id_ = -1;

  net::NetworkQualityEstimator::NetworkQualityProvider*
      network_quality_provider_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AndroidPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ANDROID_PAGE_LOAD_METRICS_OBSERVER_H_
