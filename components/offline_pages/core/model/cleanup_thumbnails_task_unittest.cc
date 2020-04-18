// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/cleanup_thumbnails_task.h"

#include <memory>

#include "base/test/bind_test_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "components/offline_pages/core/model/get_thumbnail_task.h"
#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/model/store_thumbnail_task.h"
#include "components/offline_pages/core/offline_store_utils.h"

namespace offline_pages {
namespace {

class CleanupThumbnailsTaskTest : public ModelTaskTestBase {
 public:
  ~CleanupThumbnailsTaskTest() override {}

  std::unique_ptr<OfflinePageThumbnail> ReadThumbnail(int64_t offline_id) {
    std::unique_ptr<OfflinePageThumbnail> thumb;
    auto callback = [&](std::unique_ptr<OfflinePageThumbnail> result) {
      thumb = std::move(result);
    };
    RunTask(std::make_unique<GetThumbnailTask>(
        store(), offline_id, base::BindLambdaForTesting(callback)));
    return thumb;
  }

  OfflinePageThumbnail MustReadThumbnail(int64_t offline_id) {
    std::unique_ptr<OfflinePageThumbnail> thumb = ReadThumbnail(offline_id);
    CHECK(thumb);
    return *thumb;
  }
};

TEST_F(CleanupThumbnailsTaskTest, CleanupNoThumbnails) {
  base::MockCallback<StoreThumbnailTask::CompleteCallback> callback;
  EXPECT_CALL(callback, Run(true)).Times(1);

  base::HistogramTester histogram_tester;
  RunTask(std::make_unique<CleanupThumbnailsTask>(
      store(), store_utils::FromDatabaseTime(1000), callback.Get()));

  histogram_tester.ExpectUniqueSample("OfflinePages.CleanupThumbnails.Count", 0,
                                      1);
}

TEST_F(CleanupThumbnailsTaskTest, CleanupAllCombinations) {
  // Two conditions contribute to thumbnail cleanup: does a corresponding
  // OfflinePageItem exist, and is the thumbnail expired. All four combinations
  // of these states are tested.
  const base::Time kTimeLive = store_utils::FromDatabaseTime(1000);
  const base::Time kTimeExpired = store_utils::FromDatabaseTime(999);

  // 1. Has item, not expired.
  OfflinePageItem item1 = generator()->CreateItem();
  store_test_util()->InsertItem(item1);
  OfflinePageThumbnail thumb1(item1.offline_id, kTimeLive, "thumb1");
  RunTask(
      std::make_unique<StoreThumbnailTask>(store(), thumb1, base::DoNothing()));

  // 2. Has item, expired.
  OfflinePageItem item2 = generator()->CreateItem();
  store_test_util()->InsertItem(item2);

  OfflinePageThumbnail thumb2(item2.offline_id, kTimeLive, "thumb2");
  RunTask(
      std::make_unique<StoreThumbnailTask>(store(), thumb2, base::DoNothing()));

  // 3. No item, not expired.
  OfflinePageThumbnail thumb3(store_utils::GenerateOfflineId(), kTimeLive,
                              "thumb3");
  RunTask(
      std::make_unique<StoreThumbnailTask>(store(), thumb3, base::DoNothing()));

  // 4. No item, expired. This one gets removed.
  OfflinePageThumbnail thumb4(store_utils::GenerateOfflineId(), kTimeExpired,
                              "thumb4");
  RunTask(
      std::make_unique<StoreThumbnailTask>(store(), thumb4, base::DoNothing()));

  base::MockCallback<StoreThumbnailTask::CompleteCallback> callback;
  EXPECT_CALL(callback, Run(true)).Times(1);

  base::HistogramTester histogram_tester;
  RunTask(std::make_unique<CleanupThumbnailsTask>(store(), kTimeLive,
                                                  callback.Get()));
  EXPECT_EQ(thumb1, MustReadThumbnail(thumb1.offline_id));
  EXPECT_EQ(thumb2, MustReadThumbnail(thumb2.offline_id));
  EXPECT_EQ(thumb3, MustReadThumbnail(thumb3.offline_id));
  EXPECT_EQ(nullptr, ReadThumbnail(thumb4.offline_id).get());

  histogram_tester.ExpectUniqueSample("OfflinePages.CleanupThumbnails.Count", 1,
                                      1);
}

}  // namespace
}  // namespace offline_pages
