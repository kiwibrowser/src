// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/offline_page_previews_page_load_metrics_observer.h"

#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/offline_page_tab_helper.h"
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

namespace previews {

namespace internal {

const char kHistogramOfflinePreviewsDOMContentLoadedEventFired[] =
    "PageLoad.Clients.Previews.OfflinePages.DocumentTiming."
    "NavigationToDOMContentLoadedEventFired";
const char kHistogramOfflinePreviewsFirstLayout[] =
    "PageLoad.Clients.Previews.OfflinePages.DocumentTiming."
    "NavigationToFirstLayout";
const char kHistogramOfflinePreviewsLoadEventFired[] =
    "PageLoad.Clients.Previews.OfflinePages.DocumentTiming."
    "NavigationToLoadEventFired";
const char kHistogramOfflinePreviewsFirstContentfulPaint[] =
    "PageLoad.Clients.Previews.OfflinePages.PaintTiming."
    "NavigationToFirstContentfulPaint";
const char kHistogramOfflinePreviewsParseStart[] =
    "PageLoad.Clients.Previews.OfflinePages.ParseTiming.NavigationToParseStart";

}  // namespace internal

OfflinePagePreviewsPageLoadMetricsObserver::
    OfflinePagePreviewsPageLoadMetricsObserver() {}

OfflinePagePreviewsPageLoadMetricsObserver::
    ~OfflinePagePreviewsPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OfflinePagePreviewsPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  return IsOfflinePreview(navigation_handle->GetWebContents())
             ? CONTINUE_OBSERVING
             : STOP_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OfflinePagePreviewsPageLoadMetricsObserver::ShouldObserveMimeType(
    const std::string& mime_type) const {
  // On top of base-supported types, support MHTML. Offline previews are served
  // as MHTML (multipart/related).
  return PageLoadMetricsObserver::ShouldObserveMimeType(mime_type) ==
                     CONTINUE_OBSERVING ||
                 mime_type == "multipart/related"
             ? CONTINUE_OBSERVING
             : STOP_OBSERVING;
}

void OfflinePagePreviewsPageLoadMetricsObserver::OnDomContentLoadedEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->dom_content_loaded_event_start, info)) {
    return;
  }
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramOfflinePreviewsDOMContentLoadedEventFired,
      timing.document_timing->dom_content_loaded_event_start.value());
}

void OfflinePagePreviewsPageLoadMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->load_event_start, info)) {
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramOfflinePreviewsLoadEventFired,
                      timing.document_timing->load_event_start.value());
}

void OfflinePagePreviewsPageLoadMetricsObserver::OnFirstLayout(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->first_layout, info)) {
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramOfflinePreviewsFirstLayout,
                      timing.document_timing->first_layout.value());
}

void OfflinePagePreviewsPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_contentful_paint, info)) {
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramOfflinePreviewsFirstContentfulPaint,
                      timing.paint_timing->first_contentful_paint.value());
}

void OfflinePagePreviewsPageLoadMetricsObserver::OnParseStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.parse_timing->parse_start, info)) {
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramOfflinePreviewsParseStart,
                      timing.parse_timing->parse_start.value());
}

bool OfflinePagePreviewsPageLoadMetricsObserver::IsOfflinePreview(
    content::WebContents* web_contents) const {
#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  offline_pages::OfflinePageTabHelper* tab_helper =
      offline_pages::OfflinePageTabHelper::FromWebContents(web_contents);
  return tab_helper && tab_helper->GetOfflinePreviewItem();
#else
  return false;
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)
}

}  // namespace previews
