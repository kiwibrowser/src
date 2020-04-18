// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/lofi_page_load_metrics_observer.h"

#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"

namespace data_reduction_proxy {

namespace lofi_names {

const char kNavigationToLoadEvent[] =
    "PageLoad.Clients.LoFi.DocumentTiming.NavigationToLoadEventFired";
const char kNavigationToFirstContentfulPaint[] =
    "PageLoad.Clients.LoFi.PaintTiming.NavigationToFirstContentfulPaint";
const char kNavigationToFirstMeaningfulPaint[] =
    "PageLoad.Clients.LoFi.Experimental.PaintTiming."
    "NavigationToFirstMeaningfulPaint";
const char kNavigationToFirstImagePaint[] =
    "PageLoad.Clients.LoFi.PaintTiming.NavigationToFirstImagePaint";
const char kParseBlockedOnScriptLoad[] =
    "PageLoad.Clients.LoFi.ParseTiming.ParseBlockedOnScriptLoad";
const char kParseDuration[] = "PageLoad.Clients.LoFi.ParseTiming.ParseDuration";

const char kNumNetworkResources[] =
    "PageLoad.Clients.LoFi.Experimental.CompletedResources.Network";
const char kNumNetworkLoFiResources[] =
    "PageLoad.Clients.LoFi.Experimental.CompletedResources.Network.LoFi";
const char kNetworkBytes[] = "PageLoad.Clients.LoFi.Experimental.Bytes.Network";
const char kLoFiNetworkBytes[] =
    "PageLoad.Clients.LoFi.Experimental.Bytes.Network.LoFi";

}  // namespace lofi_names

LoFiPageLoadMetricsObserver::LoFiPageLoadMetricsObserver()
    : num_network_resources_(0),
      num_network_lofi_resources_(0),
      original_network_bytes_(0),
      network_bytes_(0),
      lofi_network_bytes_(0) {}

LoFiPageLoadMetricsObserver::~LoFiPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LoFiPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  if (!started_in_foreground)
    return STOP_OBSERVING;
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
LoFiPageLoadMetricsObserver::FlushMetricsOnAppEnterBackground(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  // FlushMetricsOnAppEnterBackground is invoked on Android in cases where the
  // app is about to be backgrounded, as part of the Activity.onPause()
  // flow. After this method is invoked, Chrome may be killed without further
  // notification.
  if (num_network_lofi_resources_ > 0 && info.did_commit) {
    RecordPageSizeUMA();
    RecordTimingMetrics(timing, info);
  }
  return STOP_OBSERVING;
}

void LoFiPageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (num_network_lofi_resources_ == 0)
    return;
  RecordPageSizeUMA();
  RecordTimingMetrics(timing, info);
}

void LoFiPageLoadMetricsObserver::RecordPageSizeUMA() const {
  PAGE_RESOURCE_COUNT_HISTOGRAM(lofi_names::kNumNetworkResources,
                                num_network_resources_);
  PAGE_RESOURCE_COUNT_HISTOGRAM(lofi_names::kNumNetworkLoFiResources,
                                num_network_lofi_resources_);
  PAGE_BYTES_HISTOGRAM(lofi_names::kNetworkBytes, network_bytes_);
  PAGE_BYTES_HISTOGRAM(lofi_names::kLoFiNetworkBytes, lofi_network_bytes_);
}

void LoFiPageLoadMetricsObserver::RecordTimingMetrics(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->load_event_start, info)) {
    PAGE_LOAD_HISTOGRAM(lofi_names::kNavigationToLoadEvent,
                        timing.document_timing->load_event_start.value());
  }
  if (WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_contentful_paint, info)) {
    PAGE_LOAD_HISTOGRAM(lofi_names::kNavigationToFirstContentfulPaint,
                        timing.paint_timing->first_contentful_paint.value());
  }
  if (WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_meaningful_paint, info)) {
    PAGE_LOAD_HISTOGRAM(lofi_names::kNavigationToFirstMeaningfulPaint,
                        timing.paint_timing->first_meaningful_paint.value());
  }
  if (WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_image_paint, info)) {
    PAGE_LOAD_HISTOGRAM(lofi_names::kNavigationToFirstImagePaint,
                        timing.paint_timing->first_image_paint.value());
  }
  if (WasStartedInForegroundOptionalEventInForeground(
          timing.parse_timing->parse_stop, info)) {
    PAGE_LOAD_HISTOGRAM(
        lofi_names::kParseBlockedOnScriptLoad,
        timing.parse_timing->parse_blocked_on_script_load_duration.value());
    PAGE_LOAD_HISTOGRAM(lofi_names::kParseDuration,
                        timing.parse_timing->parse_stop.value() -
                            timing.parse_timing->parse_start.value());
  }
}

void LoFiPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo&
        extra_request_complete_info) {
  if (extra_request_complete_info.was_cached)
    return;
  ++num_network_resources_;
  network_bytes_ += extra_request_complete_info.raw_body_bytes;
  original_network_bytes_ +=
      extra_request_complete_info.original_network_content_length;
  if (extra_request_complete_info.data_reduction_proxy_data &&
      (extra_request_complete_info.data_reduction_proxy_data->lofi_received() ||
       extra_request_complete_info.data_reduction_proxy_data
           ->client_lofi_requested())) {
    ++num_network_lofi_resources_;
    lofi_network_bytes_ += extra_request_complete_info.raw_body_bytes;
  }
}

}  // namespace data_reduction_proxy
