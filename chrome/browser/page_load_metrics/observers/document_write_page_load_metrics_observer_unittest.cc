// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/document_write_page_load_metrics_observer.h"

#include <memory>

#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"

class DocumentWritePageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        std::make_unique<DocumentWritePageLoadMetricsObserver>());
  }

  void AssertNoBlockHistogramsLogged() {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 0);
  }
};

TEST_F(DocumentWritePageLoadMetricsObserverTest, NoMetrics) {
  AssertNoBlockHistogramsLogged();
  EXPECT_EQ(0ul, test_ukm_recorder().entries_count());
}

TEST_F(DocumentWritePageLoadMetricsObserverTest, PossibleBlock) {
  base::TimeDelta contentful_paint = base::TimeDelta::FromMilliseconds(1);
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.paint_timing->first_contentful_paint = contentful_paint;
  timing.parse_timing->parse_start = base::TimeDelta::FromMilliseconds(1);
  timing.parse_timing->parse_stop = base::TimeDelta::FromMilliseconds(100);
  timing.parse_timing->parse_blocked_on_script_load_duration =
      base::TimeDelta::FromMilliseconds(5);
  timing.parse_timing
      ->parse_blocked_on_script_load_from_document_write_duration =
      base::TimeDelta::FromMilliseconds(5);
  timing.parse_timing->parse_blocked_on_script_execution_duration =
      base::TimeDelta::FromMilliseconds(3);
  timing.parse_timing
      ->parse_blocked_on_script_execution_from_document_write_duration =
      base::TimeDelta::FromMilliseconds(3);
  PopulateRequiredTimingFields(&timing);

  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorDocumentWriteBlock;
  NavigateAndCommit(GURL("https://www.google.com/"));
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(internal::kHistogramDocWriteBlockCount,
                                      1);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint,
      contentful_paint.InMilliseconds(), 1);

  using Entry = ukm::builders::Intervention_DocumentWrite_ScriptBlock;

  std::map<ukm::SourceId, ukm::mojom::UkmEntryPtr> entries =
      test_ukm_recorder().GetMergedEntriesByName(Entry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto& kv : entries) {
    test_ukm_recorder().ExpectEntrySourceHasUrl(
        kv.second.get(), GURL("https://www.google.com/"));
    test_ukm_recorder().ExpectEntryMetric(
        kv.second.get(),
        Entry::kParseTiming_ParseBlockedOnScriptLoadFromDocumentWriteName, 5);
    test_ukm_recorder().ExpectEntryMetric(
        kv.second.get(),
        Entry::kParseTiming_ParseBlockedOnScriptExecutionFromDocumentWriteName,
        3);
  }

  NavigateAndCommit(GURL("https://www.example.com/"));

  histogram_tester().ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 1);
  histogram_tester().ExpectBucketCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint,
      contentful_paint.InMilliseconds(), 1);
}

TEST_F(DocumentWritePageLoadMetricsObserverTest, PossibleBlockReload) {
  base::TimeDelta contentful_paint = base::TimeDelta::FromMilliseconds(1);
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.paint_timing->first_contentful_paint = contentful_paint;
  timing.parse_timing->parse_start = base::TimeDelta::FromMilliseconds(1);
  PopulateRequiredTimingFields(&timing);

  page_load_metrics::mojom::PageLoadMetadata metadata;
  metadata.behavior_flags |= blink::WebLoadingBehaviorFlag::
      kWebLoadingBehaviorDocumentWriteBlockReload;
  NavigateAndCommit(GURL("https://www.google.com/"));
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramDocWriteBlockReloadCount, 1);

  using Entry = ukm::builders::Intervention_DocumentWrite_ScriptBlock;
  auto entries = test_ukm_recorder().GetEntriesByName(Entry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  const ukm::mojom::UkmEntry* entry1 = nullptr;
  for (const auto* const entry : entries) {
    test_ukm_recorder().ExpectEntrySourceHasUrl(
        entry, GURL("https://www.google.com/"));
    test_ukm_recorder().ExpectEntryMetric(entry, Entry::kDisabled_ReloadName,
                                          true);
    entry1 = entry;
  }

  // Another reload.
  NavigateAndCommit(GURL("https://www.example.com/"));
  SimulateTimingAndMetadataUpdate(timing, metadata);

  histogram_tester().ExpectTotalCount(
      internal::kHistogramDocWriteBlockReloadCount, 2);

  auto entries2 = test_ukm_recorder().GetEntriesByName(Entry::kEntryName);
  EXPECT_EQ(2u, entries2.size());
  for (const auto* const entry : entries2) {
    if (entry != entry1)
      test_ukm_recorder().ExpectEntrySourceHasUrl(
          entry, GURL("https://www.example.com/"));
    test_ukm_recorder().ExpectEntryMetric(entry, Entry::kDisabled_ReloadName,
                                          true);
  }

  // Another metadata update should not increase reload count.
  metadata.behavior_flags |=
      blink::WebLoadingBehaviorFlag::kWebLoadingBehaviorServiceWorkerControlled;
  SimulateTimingAndMetadataUpdate(timing, metadata);
  histogram_tester().ExpectTotalCount(
      internal::kHistogramDocWriteBlockReloadCount, 2);

  histogram_tester().ExpectTotalCount(internal::kHistogramDocWriteBlockCount,
                                      0);
}

TEST_F(DocumentWritePageLoadMetricsObserverTest, NoPossibleBlock) {
  base::TimeDelta contentful_paint = base::TimeDelta::FromMilliseconds(1);
  page_load_metrics::mojom::PageLoadTiming timing;
  page_load_metrics::InitPageLoadTimingForTest(&timing);
  timing.navigation_start = base::Time::FromDoubleT(1);
  timing.paint_timing->first_contentful_paint = contentful_paint;
  PopulateRequiredTimingFields(&timing);

  page_load_metrics::mojom::PageLoadMetadata metadata;
  NavigateAndCommit(GURL("https://www.google.com/"));
  SimulateTimingAndMetadataUpdate(timing, metadata);

  NavigateAndCommit(GURL("https://www.example.com/"));
  AssertNoBlockHistogramsLogged();
}
