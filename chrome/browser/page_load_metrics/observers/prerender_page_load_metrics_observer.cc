// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/prerender_page_load_metrics_observer.h"

#include <memory>

#include "base/logging.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/prerender/prerender_manager_factory.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

// static
std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
PrerenderPageLoadMetricsObserver::CreateIfNeeded(
    content::WebContents* web_contents) {
  prerender::PrerenderManager* manager =
      prerender::PrerenderManagerFactory::GetForBrowserContext(
          web_contents->GetBrowserContext());
  if (!manager)
    return nullptr;

  if (manager->PageLoadMetricsObserverDisabledForTesting())
    return nullptr;

  return std::make_unique<PrerenderPageLoadMetricsObserver>(manager,
                                                            web_contents);
}

PrerenderPageLoadMetricsObserver::PrerenderPageLoadMetricsObserver(
    prerender::PrerenderManager* manager,
    content::WebContents* web_contents)
    : prerender_manager_(manager),
      web_contents_(web_contents),
      is_no_store_(false),
      was_hidden_(false) {
  DCHECK(prerender_manager_);
  DCHECK(web_contents_);
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PrerenderPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  DCHECK(!started_in_foreground);

  start_ticks_ = navigation_handle->NavigationStart();
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PrerenderPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  const net::HttpResponseHeaders* response_headers =
      navigation_handle->GetResponseHeaders();

  is_no_store_ = response_headers &&
                 response_headers->HasHeaderValue("cache-control", "no-store");
  return CONTINUE_OBSERVING;
}

void PrerenderPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  DCHECK(!start_ticks_.is_null());
  prerender_manager_->RecordPrerenderFirstContentfulPaint(
      extra_info.start_url, web_contents_, is_no_store_, was_hidden_,
      start_ticks_ + *timing.paint_timing->first_contentful_paint);
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PrerenderPageLoadMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  was_hidden_ = true;
  return CONTINUE_OBSERVING;
}

void PrerenderPageLoadMetricsObserver::SetNavigationStartTicksForTesting(
    base::TimeTicks ticks) {
  start_ticks_ = ticks;
}
