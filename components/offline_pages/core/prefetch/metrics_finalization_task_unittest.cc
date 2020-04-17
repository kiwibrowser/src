// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/metrics_finalization_task.h"

#include <memory>
#include <set>

#include "base/test/metrics/histogram_tester.h"
#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_task_test_base.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class MetricsFinalizationTaskTest : public PrefetchTaskTestBase {
 public:
  MetricsFinalizationTaskTest() = default;
  ~MetricsFinalizationTaskTest() override = default;

  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<MetricsFinalizationTask> metrics_finalization_task_;
};

void MetricsFinalizationTaskTest::SetUp() {
  PrefetchTaskTestBase::SetUp();
  metrics_finalization_task_ =
      std::make_unique<MetricsFinalizationTask>(store());
}

void MetricsFinalizationTaskTest::TearDown() {
  metrics_finalization_task_.reset();
  PrefetchTaskTestBase::TearDown();
}

TEST_F(MetricsFinalizationTaskTest, StoreFailure) {
  store_util()->SimulateInitializationError();

  // Execute the metrics task.
  RunTask(metrics_finalization_task_.get());
}

// Tests that the task works correctly with an empty database.
TEST_F(MetricsFinalizationTaskTest, EmptyRun) {
  EXPECT_EQ(0, store_util()->CountPrefetchItems());

  // Execute the metrics task.
  RunTask(metrics_finalization_task_.get());
  EXPECT_EQ(0, store_util()->CountPrefetchItems());
}

TEST_F(MetricsFinalizationTaskTest, LeavesOtherStatesAlone) {
  std::vector<PrefetchItemState> all_states_but_finished =
      GetAllStatesExcept({PrefetchItemState::FINISHED});

  for (auto& state : all_states_but_finished) {
    PrefetchItem item = item_generator()->CreateItem(state);
    EXPECT_TRUE(store_util()->InsertPrefetchItem(item))
        << "Failed inserting item with state " << static_cast<int>(state);
  }

  std::set<PrefetchItem> all_inserted_items;
  EXPECT_EQ(10U, store_util()->GetAllItems(&all_inserted_items));

  // Execute the task.
  RunTask(metrics_finalization_task_.get());

  std::set<PrefetchItem> all_items_after_task;
  EXPECT_EQ(10U, store_util()->GetAllItems(&all_items_after_task));
  EXPECT_EQ(all_inserted_items, all_items_after_task);
}

TEST_F(MetricsFinalizationTaskTest, FinalizesMultipleItems) {
  std::set<PrefetchItem> finished_items = {
      item_generator()->CreateItem(PrefetchItemState::FINISHED),
      item_generator()->CreateItem(PrefetchItemState::FINISHED),
      item_generator()->CreateItem(PrefetchItemState::FINISHED)};

  PrefetchItem unfinished_item =
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST);

  for (auto& item : finished_items) {
    ASSERT_TRUE(store_util()->InsertPrefetchItem(item));
  }
  ASSERT_TRUE(store_util()->InsertPrefetchItem(unfinished_item));

  // Execute the metrics task.
  RunTask(metrics_finalization_task_.get());

  std::set<PrefetchItem> all_items;
  // The finished ones should be zombies and the new request should be
  // untouched.
  EXPECT_EQ(4U, store_util()->GetAllItems(&all_items));
  EXPECT_EQ(0U, FilterByState(all_items, PrefetchItemState::FINISHED).size());
  EXPECT_EQ(3U, FilterByState(all_items, PrefetchItemState::ZOMBIE).size());

  std::set<PrefetchItem> items_in_new_request_state =
      FilterByState(all_items, PrefetchItemState::NEW_REQUEST);
  EXPECT_EQ(1U, items_in_new_request_state.count(unfinished_item));
}

TEST_F(MetricsFinalizationTaskTest, MetricsAreReported) {
  PrefetchItem successful_item =
      item_generator()->CreateItem(PrefetchItemState::FINISHED);
  successful_item.generate_bundle_attempts = 1;
  successful_item.get_operation_attempts = 1;
  successful_item.download_initiation_attempts = 1;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(successful_item));

  PrefetchItem failed_item =
      item_generator()->CreateItem(PrefetchItemState::RECEIVED_GCM);
  failed_item.state = PrefetchItemState::FINISHED;
  failed_item.error_code = PrefetchItemErrorCode::ARCHIVING_FAILED;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(failed_item));

  PrefetchItem unfinished_item =
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST);
  ASSERT_TRUE(store_util()->InsertPrefetchItem(unfinished_item));

  // Execute the metrics task.
  base::HistogramTester histogram_tester;
  RunTask(metrics_finalization_task_.get());

  std::set<PrefetchItem> all_items;
  EXPECT_EQ(3U, store_util()->GetAllItems(&all_items));
  EXPECT_EQ(2U, FilterByState(all_items, PrefetchItemState::ZOMBIE).size());
  EXPECT_EQ(1U,
            FilterByState(all_items, PrefetchItemState::NEW_REQUEST).size());

  // One successful and one failed samples.
  histogram_tester.ExpectUniqueSample(
      "OfflinePages.Prefetching.ItemLifetime.Successful", 0, 1);
  histogram_tester.ExpectUniqueSample(
      "OfflinePages.Prefetching.ItemLifetime.Failed", 0, 1);

  // One sample for each_error code value.
  histogram_tester.ExpectTotalCount(
      "OfflinePages.Prefetching.FinishedItemErrorCode", 2);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.FinishedItemErrorCode",
      static_cast<int>(PrefetchItemErrorCode::SUCCESS), 1);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.FinishedItemErrorCode",
      static_cast<int>(PrefetchItemErrorCode::ARCHIVING_FAILED), 1);

  // One sample at the "size matches (100%)" bucket.
  histogram_tester.ExpectUniqueSample(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 11, 1);

  // Attempt values match what was set above (non set values default to 0).
  histogram_tester.ExpectTotalCount(
      "OfflinePages.Prefetching.ActionAttempts.GeneratePageBundle", 2);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.GeneratePageBundle", 0, 1);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.GeneratePageBundle", 1, 1);
  histogram_tester.ExpectTotalCount(
      "OfflinePages.Prefetching.ActionAttempts.GetOperation", 2);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.GetOperation", 0, 1);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.GetOperation", 1, 1);
  histogram_tester.ExpectTotalCount(
      "OfflinePages.Prefetching.ActionAttempts.DownloadInitiation", 2);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.DownloadInitiation", 0, 1);
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.ActionAttempts.DownloadInitiation", 1, 1);
}

TEST_F(MetricsFinalizationTaskTest, FileSizeMetricsAreReportedCorrectly) {
  PrefetchItem zero_body_length =
      item_generator()->CreateItem(PrefetchItemState::RECEIVED_BUNDLE);
  zero_body_length.state = PrefetchItemState::FINISHED;
  zero_body_length.archive_body_length = 0;
  zero_body_length.file_size = -1;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(zero_body_length));

  PrefetchItem smaller_than_expected =
      item_generator()->CreateItem(PrefetchItemState::FINISHED);
  smaller_than_expected.archive_body_length = 1000;
  smaller_than_expected.file_size = 999;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(smaller_than_expected));

  PrefetchItem sizes_match =
      item_generator()->CreateItem(PrefetchItemState::FINISHED);
  sizes_match.archive_body_length = 1000;
  sizes_match.file_size = 1000;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(sizes_match));

  PrefetchItem larger_than_expected =
      item_generator()->CreateItem(PrefetchItemState::FINISHED);
  larger_than_expected.archive_body_length = 1000;
  larger_than_expected.file_size = 1001;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(larger_than_expected));

  PrefetchItem much_larger_than_expected =
      item_generator()->CreateItem(PrefetchItemState::FINISHED);
  much_larger_than_expected.archive_body_length = 1000;
  much_larger_than_expected.file_size = 10000;
  ASSERT_TRUE(store_util()->InsertPrefetchItem(much_larger_than_expected));

  // Execute the metrics task.
  base::HistogramTester histogram_tester;
  RunTask(metrics_finalization_task_.get());

  std::set<PrefetchItem> all_items;
  EXPECT_EQ(5U, store_util()->GetAllItems(&all_items));
  EXPECT_EQ(5U, FilterByState(all_items, PrefetchItemState::ZOMBIE).size());

  histogram_tester.ExpectTotalCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 5);
  // One sample at the "archive_body_length = 0" bucket.
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 0, 1);
  // One sample at the "90% to 100%" bucket.
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 10, 1);
  // One sample at the "size matches (100%)" bucket.
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 11, 1);
  // One sample at the "100% to 110%" bucket.
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 12, 1);
  // One sample at the "above 200%" bucket.
  histogram_tester.ExpectBucketCount(
      "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected", 22, 1);
}

// Verifies that items from all states are counted properly.
TEST_F(MetricsFinalizationTaskTest,
       CountsItemsInEachStateMetricReportedCorectly) {
  // Insert a different number of items for each state.
  for (size_t i = 0; i < kOrderedPrefetchItemStates.size(); ++i) {
    PrefetchItemState state = kOrderedPrefetchItemStates[i];
    for (size_t j = 0; j < i + 1; ++j) {
      PrefetchItem item = item_generator()->CreateItem(state);
      EXPECT_TRUE(store_util()->InsertPrefetchItem(item))
          << "Failed inserting item with state " << static_cast<int>(state);
    }
  }

  // Execute the task.
  base::HistogramTester histogram_tester;
  RunTask(metrics_finalization_task_.get());

  histogram_tester.ExpectTotalCount("OfflinePages.Prefetching.StateCounts", 66);

  // Check that histogram was recorded correctly for items in each state.
  for (size_t i = 0; i < kOrderedPrefetchItemStates.size(); ++i) {
    histogram_tester.ExpectBucketCount(
        "OfflinePages.Prefetching.StateCounts",
        static_cast<int>(kOrderedPrefetchItemStates[i]), i + 1);
  }
}

}  // namespace offline_pages
