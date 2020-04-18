// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/store_thumbnail_task.h"

#include <memory>

#include "base/test/bind_test_util.h"
#include "base/test/mock_callback.h"
#include "components/offline_pages/core/model/get_thumbnail_task.h"
#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/offline_store_utils.h"

namespace offline_pages {
namespace {

class StoreThumbnailTaskTest : public ModelTaskTestBase {
 public:
  ~StoreThumbnailTaskTest() override {}

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

TEST_F(StoreThumbnailTaskTest, Success) {
  OfflinePageThumbnail thumb;
  thumb.offline_id = 1;
  thumb.expiration = store_utils::FromDatabaseTime(1234);
  thumb.thumbnail = "123abc";
  base::MockCallback<StoreThumbnailTask::CompleteCallback> callback;
  EXPECT_CALL(callback, Run(true)).Times(1);

  RunTask(std::make_unique<StoreThumbnailTask>(store(), thumb, callback.Get()));
  EXPECT_EQ(thumb, MustReadThumbnail(thumb.offline_id));
}

TEST_F(StoreThumbnailTaskTest, AlreadyExists) {
  // Store the same thumbnail twice. The second operation should overwrite the
  // first.
  OfflinePageThumbnail thumb;
  thumb.offline_id = 1;
  thumb.expiration = store_utils::FromDatabaseTime(1234);
  thumb.thumbnail = "123abc";
  base::MockCallback<StoreThumbnailTask::CompleteCallback> callback;
  EXPECT_CALL(callback, Run(true)).Times(2);

  RunTask(std::make_unique<StoreThumbnailTask>(store(), thumb, callback.Get()));
  EXPECT_EQ(thumb, MustReadThumbnail(thumb.offline_id));

  thumb.thumbnail += "_extradata";
  thumb.expiration = store_utils::FromDatabaseTime(12345);
  RunTask(std::make_unique<StoreThumbnailTask>(store(), thumb, callback.Get()));
  EXPECT_EQ(thumb, MustReadThumbnail(thumb.offline_id));
}

}  // namespace
}  // namespace offline_pages
