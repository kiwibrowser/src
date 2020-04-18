// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/tab_restore_page_load_metrics_observer.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/test/histogram_tester.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "content/public/browser/restore_type.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace {

const char kDefaultTestUrl[] = "https://google.com";

class TestTabRestorePageLoadMetricsObserver
    : public TabRestorePageLoadMetricsObserver {
 public:
  explicit TestTabRestorePageLoadMetricsObserver(bool is_restore)
      : is_restore_(is_restore) {}
  ~TestTabRestorePageLoadMetricsObserver() override {}

 private:
  bool IsTabRestore(content::NavigationHandle* navigation_handle) override {
    return is_restore_;
  }

  const bool is_restore_;

  DISALLOW_COPY_AND_ASSIGN(TestTabRestorePageLoadMetricsObserver);
};

}  // namespace

class TabRestorePageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  TabRestorePageLoadMetricsObserverTest() {}

  void ResetTest() {
    page_load_metrics::InitPageLoadTimingForTest(&timing_);
    // Reset to the default testing state. Does not reset histogram state.
    timing_.navigation_start = base::Time::FromDoubleT(1);
    timing_.response_start = base::TimeDelta::FromSeconds(2);
    timing_.parse_timing->parse_start = base::TimeDelta::FromSeconds(3);
    timing_.paint_timing->first_contentful_paint =
        base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_image_paint = base::TimeDelta::FromSeconds(5);
    timing_.paint_timing->first_text_paint = base::TimeDelta::FromSeconds(6);
    timing_.document_timing->load_event_start = base::TimeDelta::FromSeconds(7);
    PopulateRequiredTimingFields(&timing_);

    network_bytes_ = 0;
    cache_bytes_ = 0;
  }

  void SimulatePageLoad(bool is_restore, bool simulate_app_background) {
    is_restore_ = is_restore;
    NavigateAndCommit(GURL(kDefaultTestUrl));
    SimulateTimingUpdate(timing_);

    // Prepare 4 resources of varying size and configurations.
    page_load_metrics::ExtraRequestCompleteInfo resources[] = {
        // Cached request.
        {GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
         true /*was_cached*/, 1024 * 40 /* raw_body_bytes */,
         0 /* original_network_content_length */,
         nullptr /* data_reduction_proxy_data */,
         content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
         nullptr /* load_timing_info */},
        // Uncached non-proxied request.
        {GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
         false /*was_cached*/, 1024 * 40 /* raw_body_bytes */,
         1024 * 40 /* original_network_content_length */,
         nullptr /* data_reduction_proxy_data */,
         content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
         nullptr /* load_timing_info */},
        // Uncached proxied request with .1 compression ratio.
        {GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
         false /*was_cached*/, 1024 * 40 /* raw_body_bytes */,
         1024 * 40 /* original_network_content_length */,
         nullptr /* data_reduction_proxy_data */,
         content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
         nullptr /* load_timing_info */},
        // Uncached proxied request with .5 compression ratio.
        {GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
         false /*was_cached*/, 1024 * 40 /* raw_body_bytes */,
         1024 * 40 /* original_network_content_length */,
         nullptr /* data_reduction_proxy_data */,
         content::ResourceType::RESOURCE_TYPE_SCRIPT, 0,
         nullptr /* load_timing_info */},
    };

    for (const auto& request : resources) {
      SimulateLoadedResource(request);
      if (!request.was_cached) {
        network_bytes_ += request.raw_body_bytes;
      } else {
        cache_bytes_ += request.raw_body_bytes;
      }
    }

    if (simulate_app_background) {
      // The histograms should be logged when the app is backgrounded.
      SimulateAppEnterBackground();
    } else {
      NavigateToUntrackedUrl();
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(base::WrapUnique(
        new TestTabRestorePageLoadMetricsObserver(is_restore_.value())));
  }

  // Simulated byte usage since the last time the test was reset.
  int64_t network_bytes_;
  int64_t cache_bytes_;

 private:
  base::Optional<bool> is_restore_;
  page_load_metrics::mojom::PageLoadTiming timing_;

  DISALLOW_COPY_AND_ASSIGN(TabRestorePageLoadMetricsObserverTest);
};

TEST_F(TabRestorePageLoadMetricsObserverTest, NotRestored) {
  ResetTest();
  SimulatePageLoad(false /* is_restore */, false /* simulate_app_background */);
  histogram_tester().ExpectTotalCount(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Network", 0);
  histogram_tester().ExpectTotalCount(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Cache", 0);
  histogram_tester().ExpectTotalCount(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Total", 0);
}

TEST_F(TabRestorePageLoadMetricsObserverTest, Restored) {
  ResetTest();
  SimulatePageLoad(true /* is_restore */, false /* simulate_app_background */);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Network",
      static_cast<int>(network_bytes_ / 1024), 1);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Cache",
      static_cast<int>(cache_bytes_ / 1024), 1);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Total",
      static_cast<int>((network_bytes_ + cache_bytes_) / 1024), 1);
}

TEST_F(TabRestorePageLoadMetricsObserverTest, RestoredAppBackground) {
  ResetTest();
  SimulatePageLoad(true /* is_restore */, true /* simulate_app_background */);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Network",
      static_cast<int>(network_bytes_ / 1024), 1);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Cache",
      static_cast<int>(cache_bytes_ / 1024), 1);
  histogram_tester().ExpectUniqueSample(
      "PageLoad.Clients.TabRestore.Experimental.Bytes.Total",
      static_cast<int>((network_bytes_ + cache_bytes_) / 1024), 1);
}
