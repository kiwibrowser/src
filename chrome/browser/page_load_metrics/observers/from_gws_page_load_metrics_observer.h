// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_FROM_GWS_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_FROM_GWS_PAGE_LOAD_METRICS_OBSERVER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "components/ukm/ukm_source.h"
#include "url/gurl.h"

namespace internal {
// Exposed for tests.
extern const char kHistogramFromGWSDomContentLoaded[];
extern const char kHistogramFromGWSLoad[];
extern const char kHistogramFromGWSFirstPaint[];
extern const char kHistogramFromGWSFirstTextPaint[];
extern const char kHistogramFromGWSFirstImagePaint[];
extern const char kHistogramFromGWSFirstContentfulPaint[];
extern const char kHistogramFromGWSParseStartToFirstContentfulPaint[];
extern const char kHistogramFromGWSParseDuration[];
extern const char kHistogramFromGWSParseStart[];
extern const char kHistogramFromGWSAbortStopBeforePaint[];
extern const char kHistogramFromGWSAbortStopBeforeInteraction[];
extern const char kHistogramFromGWSAbortStopBeforeCommit[];
extern const char kHistogramFromGWSAbortCloseBeforePaint[];
extern const char kHistogramFromGWSAbortCloseBeforeInteraction[];
extern const char kHistogramFromGWSAbortCloseBeforeCommit[];
extern const char kHistogramFromGWSAbortNewNavigationBeforeCommit[];
extern const char kHistogramFromGWSAbortNewNavigationBeforePaint[];
extern const char kHistogramFromGWSAbortNewNavigationBeforeInteraction[];
extern const char kHistogramFromGWSAbortReloadBeforeInteraction[];
extern const char kHistogramFromGWSForegroundDuration[];
extern const char kHistogramFromGWSForegroundDurationAfterPaint[];
extern const char kHistogramFromGWSForegroundDurationNoCommit[];

}  // namespace internal

// FromGWSPageLoadMetricsLogger is a peer class to
// FromGWSPageLoadMetricsObserver. FromGWSPageLoadMetricsLogger is responsible
// for tracking state needed to decide if metrics should be logged, and to log
// metrics in cases where metrics should be logged. FromGWSPageLoadMetricsLogger
// exists to decouple the logging policy implementation from other Chromium
// classes such as NavigationHandle and related infrastructure, in order to make
// the code more unit testable.
class FromGWSPageLoadMetricsLogger {
 public:
  FromGWSPageLoadMetricsLogger();

  void SetPreviouslyCommittedUrl(const GURL& url);
  void SetProvisionalUrl(const GURL& url);

  void set_navigation_initiated_via_link(bool navigation_initiated_via_link) {
    navigation_initiated_via_link_ = navigation_initiated_via_link;
  }

  void SetNavigationStart(const base::TimeTicks navigation_start) {
    // Should be invoked at most once
    DCHECK(navigation_start_.is_null());
    navigation_start_ = navigation_start;
  }

  // Invoked when metrics for the given page are complete.
  void OnCommit(content::NavigationHandle* navigation_handle,
                ukm::SourceId source_id);
  void OnComplete(const page_load_metrics::mojom::PageLoadTiming& timing,
                  const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnFailedProvisionalLoad(
      const page_load_metrics::FailedProvisionalLoadInfo& failed_load_info,
      const page_load_metrics::PageLoadExtraInfo& extra_info);

  void OnDomContentLoadedEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnLoadEventStart(const page_load_metrics::mojom::PageLoadTiming& timing,
                        const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnFirstPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnFirstTextPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnFirstImagePaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnParseStart(const page_load_metrics::mojom::PageLoadTiming& timing,
                    const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnParseStop(const page_load_metrics::mojom::PageLoadTiming& timing,
                   const page_load_metrics::PageLoadExtraInfo& extra_info);
  void OnUserInput(const blink::WebInputEvent& event);
  void FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info);

  // The methods below are public only for testing.
  bool ShouldLogFailedProvisionalLoadMetrics();
  bool ShouldLogPostCommitMetrics(const GURL& url);
  bool ShouldLogForegroundEventAfterCommit(
      const base::Optional<base::TimeDelta>& event,
      const page_load_metrics::PageLoadExtraInfo& info);

 private:
  bool previously_committed_url_is_search_results_ = false;
  bool previously_committed_url_is_search_redirector_ = false;
  bool navigation_initiated_via_link_ = false;
  bool provisional_url_has_search_hostname_ = false;

  // The state of if first paint is triggered.
  bool first_paint_triggered_ = false;

  base::TimeTicks navigation_start_;

  // The time of first user interaction after paint from navigation start.
  base::Optional<base::TimeDelta> first_user_interaction_after_paint_;

  DISALLOW_COPY_AND_ASSIGN(FromGWSPageLoadMetricsLogger);
};

class FromGWSPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  FromGWSPageLoadMetricsObserver();

  // page_load_metrics::PageLoadMetricsObserver implementation:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                         const GURL& currently_committed_url,
                         bool started_in_foreground) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;

  ObservePolicy FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;

  void OnDomContentLoadedEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnLoadEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFirstPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFirstTextPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFirstImagePaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnParseStart(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnParseStop(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;

  void OnComplete(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  void OnFailedProvisionalLoad(
      const page_load_metrics::FailedProvisionalLoadInfo& failed_load_info,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;

  void OnUserInput(const blink::WebInputEvent& event) override;

 private:
  FromGWSPageLoadMetricsLogger logger_;

  DISALLOW_COPY_AND_ASSIGN(FromGWSPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_FROM_GWS_PAGE_LOAD_METRICS_OBSERVER_H_
