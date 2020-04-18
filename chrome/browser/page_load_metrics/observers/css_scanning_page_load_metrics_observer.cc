// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/css_scanning_page_load_metrics_observer.h"

#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"

CssScanningMetricsObserver::CssScanningMetricsObserver() {}

CssScanningMetricsObserver::~CssScanningMetricsObserver() {}

void CssScanningMetricsObserver::OnLoadingBehaviorObserved(
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (css_preload_found_)
    return;

  css_preload_found_ = page_load_metrics::DidObserveLoadingBehaviorInAnyFrame(
      info, blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorCSSPreloadFound);
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
CssScanningMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  return started_in_foreground ? CONTINUE_OBSERVING : STOP_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
CssScanningMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming&,
    const page_load_metrics::PageLoadExtraInfo&) {
  return STOP_OBSERVING;
}

void CssScanningMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!css_preload_found_)
    return;

  PAGE_LOAD_HISTOGRAM(
      "PageLoad.Clients.CssScanner.PaintTiming."
      "ParseStartToFirstContentfulPaint",
      timing.paint_timing->first_contentful_paint.value() -
          timing.parse_timing->parse_start.value());
}

void CssScanningMetricsObserver::OnFirstMeaningfulPaintInMainFrameDocument(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!css_preload_found_)
    return;

  PAGE_LOAD_HISTOGRAM(
      "PageLoad.Clients.CssScanner.Experimental.PaintTiming."
      "ParseStartToFirstMeaningfulPaint",
      timing.paint_timing->first_meaningful_paint.value() -
          timing.parse_timing->parse_start.value());
}
