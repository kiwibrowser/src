// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/metrics_collector.h"

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "build/build_config.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/resource_coordinator_clock.h"

namespace resource_coordinator {

const char kResponsivenessMeasurement[] = "ResponsivenessMeasurement";
const char kExpectedQueueingTime[] = "ExpectedTaskQueueingDuration";
const base::TimeDelta kTestMetricsReportDelayTimeout =
    kMetricsReportDelayTimeout + base::TimeDelta::FromSeconds(1);
const base::TimeDelta kTestMaxAudioSlientTimeout =
    kMaxAudioSlientTimeout + base::TimeDelta::FromSeconds(1);

// TODO(crbug.com/759905) Enable on Windows once this bug is fixed.
#if defined(OS_WIN)
#define MAYBE_MetricsCollectorTest DISABLED_MetricsCollectorTest
#else
#define MAYBE_MetricsCollectorTest MetricsCollectorTest
#endif
class MAYBE_MetricsCollectorTest : public CoordinationUnitTestHarness {
 public:
  MAYBE_MetricsCollectorTest() : CoordinationUnitTestHarness() {}

  void SetUp() override {
    MetricsCollector* metrics_collector = new MetricsCollector();
    ResourceCoordinatorClock::SetClockForTesting(&clock_);

    // Sets a valid starting time.
    clock_.SetNowTicks(base::TimeTicks::Now());
    coordination_unit_manager().RegisterObserver(
        base::WrapUnique(metrics_collector));
  }

  void TearDown() override {
    ResourceCoordinatorClock::ResetClockForTesting();
  }

 protected:
  void AdvanceClock(base::TimeDelta delta) { clock_.Advance(delta); }

  base::HistogramTester histogram_tester_;
  base::SimpleTestTickClock clock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MAYBE_MetricsCollectorTest);
};

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstAudioStartsUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->SetAudibility(true);
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(false);
  frame_cu->SetAudibility(true);
  // The page was recently audible, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  AdvanceClock(kTestMaxAudioSlientTimeout);
  page_cu->SetVisibility(true);
  frame_cu->SetAudibility(true);
  // The page was not recently audible but it is not backgrounded, thus no
  // metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(false);
  AdvanceClock(kTestMaxAudioSlientTimeout);
  frame_cu->SetAudibility(true);
  // The page was not recently audible and it is backgrounded, thus metrics
  // recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     1);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  AdvanceClock(kTestMaxAudioSlientTimeout);
  frame_cu->SetAudibility(true);
  // The page becomes visible and then invisible again, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstAudioStartsUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());

  page_cu->AddFrame(frame_cu->id());

  page_cu->SetVisibility(false);
  page_cu->OnMainFrameNavigationCommitted();
  frame_cu->SetAudibility(true);
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->SetAudibility(true);
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstTitleUpdatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  page_cu->OnTitleUpdated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     0);

  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);
  page_cu->OnTitleUpdated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstTitleUpdatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  page_cu->OnTitleUpdated();
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstAlertFiredUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->OnAlertFired();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     0);

  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);
  frame_cu->OnAlertFired();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstAlertFiredUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->OnAlertFired();
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstNonPersistentNotificationCreatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 0);

  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);
  frame_cu->OnNonPersistentNotificationCreated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 2);
}

TEST_F(
    MAYBE_MetricsCollectorTest,
    FromBackgroundedToFirstNonPersistentNotificationCreatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->OnNonPersistentNotificationCreated();
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstFaviconUpdatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  page_cu->OnFaviconUpdated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 0);

  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);
  page_cu->OnFaviconUpdated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstFaviconUpdatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  page_cu->OnFaviconUpdated();
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);
}

// Flaky test: https://crbug.com/833028
TEST_F(MAYBE_MetricsCollectorTest, ResponsivenessMetric) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  auto process_cu = CreateCoordinationUnit<ProcessCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(process_cu.get());

  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());
  process_cu->AddFrame(frame_cu->id());

  ukm::TestUkmRecorder ukm_recorder;
  coordination_unit_manager().set_ukm_recorder(&ukm_recorder);

  ukm::SourceId id = ukm_recorder.GetNewSourceID();
  GURL url = GURL("https://google.com/foobar");
  ukm_recorder.UpdateSourceURL(id, url);
  page_cu->SetUKMSourceId(id);
  page_cu->OnMainFrameNavigationCommitted();

  for (int count = 1; count < kDefaultFrequencyUkmEQTReported; ++count) {
    process_cu->SetExpectedTaskQueueingDuration(
        base::TimeDelta::FromMilliseconds(3));
    EXPECT_EQ(0U, ukm_recorder.entries_count());
    EXPECT_EQ(1U, ukm_recorder.sources_count());
  }
  process_cu->SetExpectedTaskQueueingDuration(
      base::TimeDelta::FromMilliseconds(4));
  EXPECT_EQ(1U, ukm_recorder.sources_count());
  EXPECT_EQ(1U, ukm_recorder.entries_count());
  for (int count = 1; count < kDefaultFrequencyUkmEQTReported; ++count) {
    process_cu->SetExpectedTaskQueueingDuration(
        base::TimeDelta::FromMilliseconds(3));
    EXPECT_EQ(1U, ukm_recorder.entries_count());
    EXPECT_EQ(1U, ukm_recorder.sources_count());
  }
  process_cu->SetExpectedTaskQueueingDuration(
      base::TimeDelta::FromMilliseconds(4));
  EXPECT_EQ(1U, ukm_recorder.sources_count());
  EXPECT_EQ(2U, ukm_recorder.entries_count());

  const auto& entries =
      ukm_recorder.GetEntriesByName(kResponsivenessMeasurement);
  EXPECT_EQ(2U, entries.size());
  for (const auto* entry : entries) {
    ukm_recorder.ExpectEntrySourceHasUrl(entry, url);
    ukm_recorder.ExpectEntryMetric(entry, kExpectedQueueingTime, 4);
  }
}

}  // namespace resource_coordinator
