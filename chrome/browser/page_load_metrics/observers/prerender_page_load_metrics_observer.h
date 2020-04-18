// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PRERENDER_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PRERENDER_PAGE_LOAD_METRICS_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "components/ukm/ukm_source.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace prerender {
class PrerenderManager;
}

// Observer responsible for recording First Contentful Paing metrics related to
// Prerender.
//
// To record FCP metrics for non-Prerender loads, the
// |NoStatePrefetchPageLoadMetricsObserver| is used.
class PrerenderPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  // Returns a PrerenderPageLoadMetricsObserver, or nullptr if it is not needed.
  static std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
  CreateIfNeeded(content::WebContents* web_contents);

  // Public for testing. Normally one should use CreateIfNeeded. Both the
  // manager and the web_contents must outlive this observer.
  PrerenderPageLoadMetricsObserver(prerender::PrerenderManager* manager,
                                   content::WebContents* web_contents);

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;
  ObservePolicy OnHidden(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& extra_info) override;

  void SetNavigationStartTicksForTesting(base::TimeTicks ticks);

 private:
  prerender::PrerenderManager* const prerender_manager_;
  content::WebContents* web_contents_;
  base::TimeTicks start_ticks_;
  bool is_no_store_;
  bool was_hidden_;

  DISALLOW_COPY_AND_ASSIGN(PrerenderPageLoadMetricsObserver);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PRERENDER_PAGE_LOAD_METRICS_OBSERVER_H_
