// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/lofi_page_load_metrics_observer.h"

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"

namespace data_reduction_proxy {

namespace {

const char kDefaultTestUrl[] = "https://www.google.com";

class LoFiPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  LoFiPageLoadMetricsObserverTest() {}
  ~LoFiPageLoadMetricsObserverTest() override {}

  void ResetTest() {
    page_load_metrics::InitPageLoadTimingForTest(&timing_);
    // Reset to the default testing state. Does not reset histogram state.
    timing_.navigation_start = base::Time::FromDoubleT(1);
    timing_.response_start = base::TimeDelta::FromSeconds(2);
    timing_.parse_timing->parse_start = base::TimeDelta::FromSeconds(3);
    timing_.paint_timing->first_contentful_paint =
        base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_paint = base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_meaningful_paint =
        base::TimeDelta::FromSeconds(8);
    timing_.paint_timing->first_image_paint = base::TimeDelta::FromSeconds(5);
    timing_.paint_timing->first_text_paint = base::TimeDelta::FromSeconds(6);
    timing_.document_timing->load_event_start = base::TimeDelta::FromSeconds(7);
    timing_.parse_timing->parse_stop = base::TimeDelta::FromSeconds(4);
    timing_.parse_timing->parse_blocked_on_script_load_duration =
        base::TimeDelta::FromSeconds(1);
    PopulateRequiredTimingFields(&timing_);
  }

  void RunTest() {
    NavigateAndCommit(GURL(kDefaultTestUrl));
    SimulateTimingUpdate(timing_);
  }

  void ValidateTimingHistograms(bool lofi_request_sent) {
    ValidateTimingHistogram(lofi_names::kNavigationToLoadEvent,
                            timing_.document_timing->load_event_start,
                            lofi_request_sent);
    ValidateTimingHistogram(lofi_names::kNavigationToFirstContentfulPaint,
                            timing_.paint_timing->first_contentful_paint,
                            lofi_request_sent);
    ValidateTimingHistogram(lofi_names::kNavigationToFirstMeaningfulPaint,
                            timing_.paint_timing->first_meaningful_paint,
                            lofi_request_sent);
    ValidateTimingHistogram(lofi_names::kNavigationToFirstImagePaint,
                            timing_.paint_timing->first_image_paint,
                            lofi_request_sent);
    ValidateTimingHistogram(
        lofi_names::kParseBlockedOnScriptLoad,
        timing_.parse_timing->parse_blocked_on_script_load_duration,
        lofi_request_sent);
    ValidateTimingHistogram(lofi_names::kParseDuration,
                            timing_.parse_timing->parse_stop.value() -
                                timing_.parse_timing->parse_start.value(),
                            lofi_request_sent);
  }

  void ValidateTimingHistogram(const std::string& histogram,
                               const base::Optional<base::TimeDelta>& event,
                               bool lofi_request_sent) {
    histogram_tester().ExpectTotalCount(histogram, lofi_request_sent ? 1 : 0);
    if (!lofi_request_sent)
      return;
    histogram_tester().ExpectUniqueSample(
        histogram,
        static_cast<base::HistogramBase::Sample>(
            event.value().InMilliseconds()),
        1);
  }

  void ValidateDataHistograms(int network_resources,
                              int lofi_resources,
                              int64_t network_bytes,
                              int64_t lofi_bytes) {
    if (lofi_resources > 0) {
      histogram_tester().ExpectUniqueSample(lofi_names::kNumNetworkResources,
                                            network_resources, 1);
      histogram_tester().ExpectUniqueSample(
          lofi_names::kNumNetworkLoFiResources, lofi_resources, 1);
      histogram_tester().ExpectUniqueSample(
          lofi_names::kNetworkBytes, static_cast<int>(network_bytes / 1024), 1);
      histogram_tester().ExpectUniqueSample(lofi_names::kLoFiNetworkBytes,
                                            static_cast<int>(lofi_bytes / 1024),
                                            1);
    } else {
      histogram_tester().ExpectTotalCount(lofi_names::kNumNetworkResources, 0);
      histogram_tester().ExpectTotalCount(lofi_names::kNumNetworkLoFiResources,
                                          0);
      histogram_tester().ExpectTotalCount(lofi_names::kNetworkBytes, 0);
      histogram_tester().ExpectTotalCount(lofi_names::kLoFiNetworkBytes, 0);
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(std::make_unique<LoFiPageLoadMetricsObserver>());
  }

  page_load_metrics::mojom::PageLoadTiming timing_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoFiPageLoadMetricsObserverTest);
};

TEST_F(LoFiPageLoadMetricsObserverTest, LoFiNotSeen) {
  ResetTest();
  RunTest();

  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);

  // Prepare 4 resources of varying size and configurations, none of which have
  // LoFi set.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */, 0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
       nullptr /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 10 /* original_network_content_length */, data->DeepCopy(),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 5 /* original_network_content_length */, std::move(data),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
  };

  int network_resources = 0;
  int lofi_resources = 0;
  int64_t network_bytes = 0;
  int64_t lofi_bytes = 0;

  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    if (!request.was_cached) {
      network_bytes += request.raw_body_bytes;
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        (request.data_reduction_proxy_data->lofi_received() ||
         request.data_reduction_proxy_data->client_lofi_requested())) {
      lofi_bytes += request.raw_body_bytes;
      ++lofi_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateTimingHistograms(false);
  ValidateDataHistograms(network_resources, lofi_resources, network_bytes,
                         lofi_bytes);
}

TEST_F(LoFiPageLoadMetricsObserverTest, ClientLoFiSeen) {
  ResetTest();
  RunTest();

  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_client_lofi_requested(true);

  // Prepare 4 resources of varying size and configurations, 2 of which have
  // client LoFi set.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */, 0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
       nullptr /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 10 /* original_network_content_length */, data->DeepCopy(),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 5 /* original_network_content_length */, std::move(data),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
  };

  int network_resources = 0;
  int lofi_resources = 0;
  int64_t network_bytes = 0;
  int64_t lofi_bytes = 0;

  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    if (!request.was_cached) {
      network_bytes += request.raw_body_bytes;
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        (request.data_reduction_proxy_data->lofi_received() ||
         request.data_reduction_proxy_data->client_lofi_requested())) {
      lofi_bytes += request.raw_body_bytes;
      ++lofi_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateTimingHistograms(true);
  ValidateDataHistograms(network_resources, lofi_resources, network_bytes,
                         lofi_bytes);
}

TEST_F(LoFiPageLoadMetricsObserverTest, ServerLoFiSeen) {
  ResetTest();
  RunTest();

  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_lofi_received(true);

  // Prepare 4 resources of varying size and configurations, 2 of which have
  // server LoFi set.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */, 0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
       nullptr /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 10 /* original_network_content_length */, data->DeepCopy(),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 5 /* original_network_content_length */, std::move(data),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
  };

  int network_resources = 0;
  int lofi_resources = 0;
  int64_t network_bytes = 0;
  int64_t lofi_bytes = 0;

  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    if (!request.was_cached) {
      network_bytes += request.raw_body_bytes;
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        (request.data_reduction_proxy_data->lofi_received() ||
         request.data_reduction_proxy_data->client_lofi_requested())) {
      lofi_bytes += request.raw_body_bytes;
      ++lofi_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateTimingHistograms(true);
  ValidateDataHistograms(network_resources, lofi_resources, network_bytes,
                         lofi_bytes);
}

TEST_F(LoFiPageLoadMetricsObserverTest, BothLoFiSeen) {
  ResetTest();
  RunTest();

  std::unique_ptr<DataReductionProxyData> data1 =
      std::make_unique<DataReductionProxyData>();
  data1->set_used_data_reduction_proxy(true);
  data1->set_lofi_received(true);

  std::unique_ptr<DataReductionProxyData> data2 =
      std::make_unique<DataReductionProxyData>();
  data2->set_used_data_reduction_proxy(true);
  data2->set_client_lofi_requested(true);

  // Prepare 4 resources of varying size and configurations, 1 has Client LoFi,
  // 1 has Server LoFi.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */, 0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
       nullptr /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 10 /* original_network_content_length */, std::move(data1),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl), net::HostPortPair(), -1, false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 5 /* original_network_content_length */, std::move(data2),
       content::ResourceType::RESOURCE_TYPE_IMAGE, 0,
       nullptr /* load_timing_info */},
  };

  int network_resources = 0;
  int lofi_resources = 0;
  int64_t network_bytes = 0;
  int64_t lofi_bytes = 0;

  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    if (!request.was_cached) {
      network_bytes += request.raw_body_bytes;
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        (request.data_reduction_proxy_data->lofi_received() ||
         request.data_reduction_proxy_data->client_lofi_requested())) {
      lofi_bytes += request.raw_body_bytes;
      ++lofi_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateTimingHistograms(true);
  ValidateDataHistograms(network_resources, lofi_resources, network_bytes,
                         lofi_bytes);
}

}  // namespace

}  //  namespace data_reduction_proxy
