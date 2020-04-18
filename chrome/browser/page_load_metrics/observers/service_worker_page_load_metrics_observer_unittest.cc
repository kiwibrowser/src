// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/service_worker_page_load_metrics_observer.h"

#include <memory>

#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"

namespace {

const char kDefaultTestUrl[] = "https://google.com/";
const char kInboxTestUrl[] = "https://inbox.google.com/test";
const char kSearchTestUrl[] = "https://www.google.com/search?q=test";

}  // namespace

class ServiceWorkerPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        std::make_unique<ServiceWorkerPageLoadMetricsObserver>());
  }

  void SimulateTimingWithoutPaint() {
    page_load_metrics::mojom::PageLoadTiming timing;
    page_load_metrics::InitPageLoadTimingForTest(&timing);
    timing.navigation_start = base::Time::FromDoubleT(1);
    SimulateTimingUpdate(timing);
  }

  void AssertNoServiceWorkerHistogramsLogged() {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstContentfulPaint, 0);
    histogram_tester().ExpectTotalCount(
        internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstContentfulPaintForwardBack, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstContentfulPaintForwardBackNoStore,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstMeaningfulPaint, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerDomContentLoaded, 0);
    histogram_tester().ExpectTotalCount(internal::kHistogramServiceWorkerLoad,
                                        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStart, 0);
    histogram_tester().ExpectTotalCount(
        internal::kBackgroundHistogramServiceWorkerParseStart, 0);
  }

  void AssertNoInboxHistogramsLogged() {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartInbox, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstContentfulPaintInbox, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintInbox,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstMeaningfulPaintInbox, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintInbox,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerDomContentLoadedInbox, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerLoadInbox, 0);
  }

  void AssertNoSearchHistogramsLogged() {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstContentfulPaintSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintSearch,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerFirstMeaningfulPaintSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintSearch,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerDomContentLoadedSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramServiceWorkerLoadSearch, 0);
  }

  void AssertNoSearchNoSWHistogramsLogged() {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramNoServiceWorkerFirstContentfulPaintSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::
            kHistogramNoServiceWorkerParseStartToFirstContentfulPaintSearch,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramNoServiceWorkerFirstMeaningfulPaintSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::
            kHistogramNoServiceWorkerParseStartToFirstMeaningfulPaintSearch,
        0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramNoServiceWorkerDomContentLoadedSearch, 0);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramNoServiceWorkerLoadSearch, 0);
  }

  void InitializeTestPageLoadTiming(
      page_load_metrics::mojom::PageLoadTiming* timing) {
    page_load_metrics::InitPageLoadTimingForTest(timing);
    timing->navigation_start = base::Time::FromDoubleT(1);
    timing->parse_timing->parse_start = base::TimeDelta::FromMilliseconds(100);
    timing->paint_timing->first_contentful_paint =
        base::TimeDelta::FromMilliseconds(300);
    timing->paint_timing->first_meaningful_paint =
        base::TimeDelta::FromMilliseconds(700);
    timing->document_timing->dom_content_loaded_event_start =
        base::TimeDelta::FromMilliseconds(600);
    timing->document_timing->load_event_start =
        base::TimeDelta::FromMilliseconds(1000);
    PopulateRequiredTimingFields(timing);
  }
};

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, NoMetrics) {
  AssertNoServiceWorkerHistogramsLogged();
  AssertNoInboxHistogramsLogged();
  AssertNoSearchHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
  EXPECT_EQ(0ul, test_ukm_recorder().entries_count());
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, NoServiceWorker) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  NavigateAndCommit(GURL(kDefaultTestUrl));
  SimulateTimingUpdate(timing);

  AssertNoServiceWorkerHistogramsLogged();
  AssertNoInboxHistogramsLogged();
  AssertNoSearchHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
  EXPECT_EQ(0ul, test_ukm_recorder().entries_count());
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, WithServiceWorker) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  NavigateAndCommit(GURL(kDefaultTestUrl));
  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint, 0);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoaded, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerDomContentLoaded,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(internal::kHistogramServiceWorkerLoad, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerLoad,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartForwardBack, 0);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartForwardBackNoStore, 0);

  const auto& entries = test_ukm_recorder().GetEntriesByName(
      ukm::builders::PageLoad_ServiceWorkerControlled::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_ukm_recorder().ExpectEntrySourceHasUrl(entry, GURL(kDefaultTestUrl));
  }

  AssertNoInboxHistogramsLogged();
  AssertNoSearchHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, WithServiceWorkerBackground) {
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  PopulateRequiredTimingFields(&timing);

  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;

  NavigateAndCommit(GURL(kDefaultTestUrl));
  SimulateTimingAndMetadataUpdate(timing, metadata);

  // Background the tab, then forground it.
  web_contents()->WasHidden();
  web_contents()->WasShown();

  InitializeTestPageLoadTiming(&timing);
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint, 0);
  histogram_tester().ExpectTotalCount(
      internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint, 0);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaint, 0);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint, 0);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoaded, 0);
  histogram_tester().ExpectTotalCount(internal::kHistogramServiceWorkerLoad, 0);
  // TODO(crbug.com/686590): The following expectation fails on Win7 Tests
  // (dbg)(1) builder, so is disabled for the time being.
  // histogram_tester().ExpectTotalCount(
  //     internal::kBackgroundHistogramServiceWorkerParseStart, 1);

  const auto& entries = test_ukm_recorder().GetEntriesByName(
      ukm::builders::PageLoad_ServiceWorkerControlled::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_ukm_recorder().ExpectEntrySourceHasUrl(entry, GURL(kDefaultTestUrl));
  }

  AssertNoInboxHistogramsLogged();
  AssertNoSearchHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, InboxSite) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  NavigateAndCommit(GURL(kInboxTestUrl));
  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartInbox, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartInbox,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintInbox, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintInbox,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint, 0);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintInbox,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintInbox,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaint,
      timing.paint_timing->first_meaningful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaintInbox, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaintInbox,
      timing.paint_timing->first_meaningful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint,
      (timing.paint_timing->first_meaningful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintInbox,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintInbox,
      (timing.paint_timing->first_meaningful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoaded, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerDomContentLoaded,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoadedInbox, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerDomContentLoadedInbox,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(internal::kHistogramServiceWorkerLoad, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerLoad,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerLoadInbox, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerLoadInbox,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);

  AssertNoSearchHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, SearchSite) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  NavigateAndCommit(GURL(kSearchTestUrl));
  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartSearch,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintSearch,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kBackgroundHistogramServiceWorkerFirstContentfulPaint, 0);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaint,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintSearch,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstContentfulPaintSearch,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaint,
      timing.paint_timing->first_meaningful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaintSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstMeaningfulPaintSearch,
      timing.paint_timing->first_meaningful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaint,
      (timing.paint_timing->first_meaningful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintSearch,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartToFirstMeaningfulPaintSearch,
      (timing.paint_timing->first_meaningful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoaded, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerDomContentLoaded,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerDomContentLoadedSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerDomContentLoadedSearch,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(internal::kHistogramServiceWorkerLoad, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerLoad,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerLoadSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerLoadSearch,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);

  AssertNoInboxHistogramsLogged();
  AssertNoSearchNoSWHistogramsLogged();
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest, SearchNoSWSite) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  NavigateAndCommit(GURL(kSearchTestUrl));
  page_load_metrics::mojom::PageLoadMetadata metadata;
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerFirstContentfulPaintSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerFirstContentfulPaintSearch,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerParseStartToFirstContentfulPaintSearch,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerParseStartToFirstContentfulPaintSearch,
      (timing.paint_timing->first_contentful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerFirstMeaningfulPaintSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerFirstMeaningfulPaintSearch,
      timing.paint_timing->first_meaningful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerParseStartToFirstMeaningfulPaintSearch,
      1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerParseStartToFirstMeaningfulPaintSearch,
      (timing.paint_timing->first_meaningful_paint.value() -
       timing.parse_timing->parse_start.value())
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerDomContentLoadedSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerDomContentLoadedSearch,
      timing.document_timing->dom_content_loaded_event_start.value()
          .InMilliseconds(),
      1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramNoServiceWorkerLoadSearch, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramNoServiceWorkerLoadSearch,
      timing.document_timing->load_event_start.value().InMilliseconds(), 1);

  AssertNoServiceWorkerHistogramsLogged();
  AssertNoInboxHistogramsLogged();
  AssertNoSearchHistogramsLogged();
}

TEST_F(ServiceWorkerPageLoadMetricsObserverTest,
       WithServiceWorker_ForwardBack) {
  page_load_metrics::mojom::PageLoadTiming timing;
  InitializeTestPageLoadTiming(&timing);

  // Back navigations to a page that was reloaded report a main transition type
  // of PAGE_TRANSITION_RELOAD with a PAGE_TRANSITION_FORWARD_BACK
  // modifier. This test verifies that when we encounter such a page, we log it
  // as a forward/back navigation.
  NavigateWithPageTransitionAndCommit(
      GURL(kDefaultTestUrl),
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_RELOAD |
                                ui::PAGE_TRANSITION_FORWARD_BACK));
  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaint,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintForwardBack, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerFirstContentfulPaintForwardBack,
      timing.paint_timing->first_contentful_paint.value().InMilliseconds(), 1);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStart, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStart,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramServiceWorkerParseStartForwardBack, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramServiceWorkerParseStartForwardBack,
      timing.parse_timing->parse_start.value().InMilliseconds(), 1);
}
