// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/service_worker_page_load_metrics_observer.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/observers/from_gws_page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "net/http/http_response_headers.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"

namespace internal {

const char kHistogramServiceWorkerPageTransition[] =
    "PageLoad.Clients.ServiceWorker.PageTransition";

const char kHistogramServiceWorkerParseStart[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart";
const char kHistogramServiceWorkerParseStartForwardBack[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart."
    "LoadType.ForwardBackNavigation";
const char kHistogramServiceWorkerParseStartForwardBackNoStore[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart."
    "LoadType.ForwardBackNavigation.NoStore";
const char kBackgroundHistogramServiceWorkerParseStart[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart."
    "Background";
const char kHistogramServiceWorkerFirstContentfulPaint[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint";
const char kHistogramServiceWorkerFirstContentfulPaintForwardBack[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.LoadType.ForwardBackNavigation";
const char kHistogramServiceWorkerFirstContentfulPaintForwardBackNoStore[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.LoadType.ForwardBackNavigation.NoStore";
const char kBackgroundHistogramServiceWorkerFirstContentfulPaint[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.Background";
const char kHistogramServiceWorkerParseStartToFirstContentfulPaint[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "ParseStartToFirstContentfulPaint";
const char kHistogramServiceWorkerFirstMeaningfulPaint[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "NavigationToFirstMeaningfulPaint";
const char kHistogramServiceWorkerParseStartToFirstMeaningfulPaint[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "ParseStartToFirstMeaningfulPaint";
const char kHistogramServiceWorkerDomContentLoaded[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming."
    "NavigationToDOMContentLoadedEventFired";
const char kHistogramServiceWorkerLoad[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming.NavigationToLoadEventFired";

const char kHistogramServiceWorkerParseStartInbox[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart.inbox";
const char kHistogramServiceWorkerFirstContentfulPaintInbox[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.inbox";
const char kHistogramServiceWorkerParseStartToFirstContentfulPaintInbox[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "ParseStartToFirstContentfulPaint.inbox";
const char kHistogramServiceWorkerFirstMeaningfulPaintInbox[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "NavigationToFirstMeaningfulPaint.inbox";
const char kHistogramServiceWorkerParseStartToFirstMeaningfulPaintInbox[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "ParseStartToFirstMeaningfulPaint.inbox";
const char kHistogramServiceWorkerDomContentLoadedInbox[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming."
    "NavigationToDOMContentLoadedEventFired.inbox";
const char kHistogramServiceWorkerLoadInbox[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming.NavigationToLoadEventFired."
    "inbox";

const char kHistogramServiceWorkerParseStartSearch[] =
    "PageLoad.Clients.ServiceWorker.ParseTiming.NavigationToParseStart.search";
const char kHistogramServiceWorkerFirstContentfulPaintSearch[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.search";
const char kHistogramServiceWorkerParseStartToFirstContentfulPaintSearch[] =
    "PageLoad.Clients.ServiceWorker.PaintTiming."
    "ParseStartToFirstContentfulPaint.search";
const char kHistogramServiceWorkerFirstMeaningfulPaintSearch[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "NavigationToFirstMeaningfulPaint.search";
const char kHistogramServiceWorkerParseStartToFirstMeaningfulPaintSearch[] =
    "PageLoad.Clients.ServiceWorker.Experimental.PaintTiming."
    "ParseStartToFirstMeaningfulPaint.search";
const char kHistogramServiceWorkerDomContentLoadedSearch[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming."
    "NavigationToDOMContentLoadedEventFired.search";
const char kHistogramServiceWorkerLoadSearch[] =
    "PageLoad.Clients.ServiceWorker.DocumentTiming.NavigationToLoadEventFired."
    "search";

const char kHistogramNoServiceWorkerFirstContentfulPaintSearch[] =
    "PageLoad.Clients.NoServiceWorker.PaintTiming."
    "NavigationToFirstContentfulPaint.search";
const char kHistogramNoServiceWorkerParseStartToFirstContentfulPaintSearch[] =
    "PageLoad.Clients.NoServiceWorker.PaintTiming."
    "ParseStartToFirstContentfulPaint.search";
const char kHistogramNoServiceWorkerFirstMeaningfulPaintSearch[] =
    "PageLoad.Clients.NoServiceWorker.Experimental.PaintTiming."
    "NavigationToFirstMeaningfulPaint.search";
const char kHistogramNoServiceWorkerParseStartToFirstMeaningfulPaintSearch[] =
    "PageLoad.Clients.NoServiceWorker.Experimental.PaintTiming."
    "ParseStartToFirstMeaningfulPaint.search";
const char kHistogramNoServiceWorkerDomContentLoadedSearch[] =
    "PageLoad.Clients.NoServiceWorker.DocumentTiming."
    "NavigationToDOMContentLoadedEventFired.search";
const char kHistogramNoServiceWorkerLoadSearch[] =
    "PageLoad.Clients.NoServiceWorker.DocumentTiming."
    "NavigationToLoadEventFired.search";

}  // namespace internal

namespace {

bool IsServiceWorkerControlled(
    const page_load_metrics::PageLoadExtraInfo& info) {
  return (info.main_frame_metadata.behavior_flags &
          blink::WebLoadingBehaviorFlag::
              kWebLoadingBehaviorServiceWorkerControlled) != 0;
}

bool IsInboxSite(const GURL& url) {
  return url.host_piece() == "inbox.google.com";
}

bool IsForwardBackLoad(ui::PageTransition transition) {
  return transition & ui::PAGE_TRANSITION_FORWARD_BACK;
}

}  // namespace

ServiceWorkerPageLoadMetricsObserver::ServiceWorkerPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
ServiceWorkerPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  transition_ = navigation_handle->GetPageTransition();
  const net::HttpResponseHeaders* headers =
      navigation_handle->GetResponseHeaders();
  if (headers) {
    was_no_store_main_resource_ =
        headers->HasHeaderValue("cache-control", "no-store");
  }
  return CONTINUE_OBSERVING;
}

void ServiceWorkerPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!IsServiceWorkerControlled(info)) {
    if (!WasStartedInForegroundOptionalEventInForeground(
            timing.paint_timing->first_contentful_paint, info) ||
        !page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
      return;
    }
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramNoServiceWorkerFirstContentfulPaintSearch,
        timing.paint_timing->first_contentful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::
            kHistogramNoServiceWorkerParseStartToFirstContentfulPaintSearch,
        timing.paint_timing->first_contentful_paint.value() -
            timing.parse_timing->parse_start.value());
    return;
  }
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_contentful_paint, info)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint,
        timing.paint_timing->first_contentful_paint.value());
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerFirstContentfulPaint,
                      timing.paint_timing->first_contentful_paint.value());
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value() -
          timing.parse_timing->parse_start.value());

  if (IsForwardBackLoad(transition_)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerFirstContentfulPaintForwardBack,
        timing.paint_timing->first_contentful_paint.value());
    if (was_no_store_main_resource_) {
      PAGE_LOAD_HISTOGRAM(
          internal::
              kHistogramServiceWorkerFirstContentfulPaintForwardBackNoStore,
          timing.paint_timing->first_contentful_paint.value());
    }
  }

  if (IsInboxSite(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerFirstContentfulPaintInbox,
        timing.paint_timing->first_contentful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintInbox,
        timing.paint_timing->first_contentful_paint.value() -
            timing.parse_timing->parse_start.value());
  } else if (page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerFirstContentfulPaintSearch,
        timing.paint_timing->first_contentful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintSearch,
        timing.paint_timing->first_contentful_paint.value() -
            timing.parse_timing->parse_start.value());
  }
}

void ServiceWorkerPageLoadMetricsObserver::
    OnFirstMeaningfulPaintInMainFrameDocument(
        const page_load_metrics::mojom::PageLoadTiming& timing,
        const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_meaningful_paint, info)) {
    return;
  }
  if (!IsServiceWorkerControlled(info)) {
    if (!page_load_metrics::IsGoogleSearchResultUrl(info.url))
      return;
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramNoServiceWorkerFirstMeaningfulPaintSearch,
        timing.paint_timing->first_meaningful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::
            kHistogramNoServiceWorkerParseStartToFirstMeaningfulPaintSearch,
        timing.paint_timing->first_meaningful_paint.value() -
            timing.parse_timing->parse_start.value());
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerFirstMeaningfulPaint,
                      timing.paint_timing->first_meaningful_paint.value());
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint,
      timing.paint_timing->first_meaningful_paint.value() -
          timing.parse_timing->parse_start.value());

  if (IsInboxSite(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerFirstMeaningfulPaintInbox,
        timing.paint_timing->first_meaningful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintInbox,
        timing.paint_timing->first_meaningful_paint.value() -
            timing.parse_timing->parse_start.value());
  } else if (page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerFirstMeaningfulPaintSearch,
        timing.paint_timing->first_meaningful_paint.value());
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintSearch,
        timing.paint_timing->first_meaningful_paint.value() -
            timing.parse_timing->parse_start.value());
  }
}

void ServiceWorkerPageLoadMetricsObserver::OnDomContentLoadedEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->dom_content_loaded_event_start, info)) {
    return;
  }
  if (!IsServiceWorkerControlled(info)) {
    if (!page_load_metrics::IsGoogleSearchResultUrl(info.url))
      return;
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramNoServiceWorkerDomContentLoadedSearch,
        timing.document_timing->dom_content_loaded_event_start.value());
    return;
  }
  PAGE_LOAD_HISTOGRAM(
      internal::kHistogramServiceWorkerDomContentLoaded,
      timing.document_timing->dom_content_loaded_event_start.value());
  if (IsInboxSite(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerDomContentLoadedInbox,
        timing.document_timing->dom_content_loaded_event_start.value());
  } else if (page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
    PAGE_LOAD_HISTOGRAM(
        internal::kHistogramServiceWorkerDomContentLoadedSearch,
        timing.document_timing->dom_content_loaded_event_start.value());
  }
}

void ServiceWorkerPageLoadMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->load_event_start, info))
    return;
  if (!IsServiceWorkerControlled(info)) {
    if (!page_load_metrics::IsGoogleSearchResultUrl(info.url))
      return;
    PAGE_LOAD_HISTOGRAM(internal::kHistogramNoServiceWorkerLoadSearch,
                        timing.document_timing->load_event_start.value());
    return;
  }
  PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerLoad,
                      timing.document_timing->load_event_start.value());
  if (IsInboxSite(info.url)) {
    PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerLoadInbox,
                        timing.document_timing->load_event_start.value());
  } else if (page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
    PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerLoadSearch,
                        timing.document_timing->load_event_start.value());
  }
}

void ServiceWorkerPageLoadMetricsObserver::OnParseStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!IsServiceWorkerControlled(info))
    return;

  // TODO(falken): It may be cleaner to record page transition in OnCommit() but
  // at that point we don't yet know if the page is controlled by a service
  // worker. It should be possible to plumb the information there since the
  // browser process already sends the controller service worker in the
  // navigation commit IPC.
  UMA_HISTOGRAM_ENUMERATION(
      internal::kHistogramServiceWorkerPageTransition,
      static_cast<int>(ui::PageTransitionStripQualifier(transition_)),
      static_cast<int>(ui::PAGE_TRANSITION_LAST_CORE) + 1);

  if (WasStartedInForegroundOptionalEventInForeground(
          timing.parse_timing->parse_start, info)) {
    PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerParseStart,
                        timing.parse_timing->parse_start.value());

    if (IsInboxSite(info.url)) {
      PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerParseStartInbox,
                          timing.parse_timing->parse_start.value());
    } else if (page_load_metrics::IsGoogleSearchResultUrl(info.url)) {
      PAGE_LOAD_HISTOGRAM(internal::kHistogramServiceWorkerParseStartSearch,
                          timing.parse_timing->parse_start.value());
    }
    if (IsForwardBackLoad(transition_)) {
      PAGE_LOAD_HISTOGRAM(
          internal::kHistogramServiceWorkerParseStartForwardBack,
          timing.parse_timing->parse_start.value());
      if (was_no_store_main_resource_) {
        PAGE_LOAD_HISTOGRAM(
            internal::kHistogramServiceWorkerParseStartForwardBackNoStore,
            timing.parse_timing->parse_start.value());
      }
    }
  } else {
    PAGE_LOAD_HISTOGRAM(internal::kBackgroundHistogramServiceWorkerParseStart,
                        timing.parse_timing->parse_start.value());
  }
}

void ServiceWorkerPageLoadMetricsObserver::OnLoadingBehaviorObserved(
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!IsServiceWorkerControlled(info) || logged_ukm_event_)
    return;
  ukm::builders::PageLoad_ServiceWorkerControlled(info.source_id)
      .Record(ukm::UkmRecorder::Get());
  logged_ukm_event_ = true;
}
