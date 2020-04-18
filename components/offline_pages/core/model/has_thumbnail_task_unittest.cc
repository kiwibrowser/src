// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/has_thumbnail_task.h"

#include <memory>

#include "base/bind_helpers.h"
#include "base/test/bind_test_util.h"
#include "base/test/mock_callback.h"
#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/model/store_thumbnail_task.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {

using ThumbnailExistsCallback = HasThumbnailTask::ThumbnailExistsCallback;

class HasThumbnailTaskTest : public ModelTaskTestBase {};

TEST_F(HasThumbnailTaskTest, CorrectlyFindsById) {
  const int64_t valid_offline_id = 1;
  const int64_t invalid_offline_id = 2;
  OfflinePageThumbnail thumb;
  thumb.offline_id = valid_offline_id;
  thumb.expiration = store_utils::FromDatabaseTime(1234);
  thumb.thumbnail = "123abc";
  RunTask(
      std::make_unique<StoreThumbnailTask>(store(), thumb, base::DoNothing()));

  base::MockCallback<ThumbnailExistsCallback> exists_callback;
  EXPECT_CALL(exists_callback, Run(true));
  RunTask(std::make_unique<HasThumbnailTask>(store(), valid_offline_id,
                                             exists_callback.Get()));

  base::MockCallback<ThumbnailExistsCallback> doesnt_exist_callback;
  EXPECT_CALL(doesnt_exist_callback, Run(false));
  RunTask(std::make_unique<HasThumbnailTask>(store(), invalid_offline_id,
                                             doesnt_exist_callback.Get()));
}

}  // namespace
}  // namespace offline_pages
