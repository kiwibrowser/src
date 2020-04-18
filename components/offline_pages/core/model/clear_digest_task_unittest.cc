// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/clear_digest_task.h"

#include "components/offline_pages/core/model/model_task_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char kTestDigest[] = "ktesTDIgest==";
}  // namespace

class ClearDigestTaskTest : public ModelTaskTestBase {};

TEST_F(ClearDigestTaskTest, ClearDigest) {
  OfflinePageItem page = generator()->CreateItem();
  page.digest = kTestDigest;
  store_test_util()->InsertItem(page);

  auto task = std::make_unique<ClearDigestTask>(store(), page.offline_id);
  RunTask(std::move(task));

  // Check the digest of the page is cleared.
  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  EXPECT_TRUE(offline_page);
  EXPECT_TRUE(offline_page->digest.empty());
}

}  // namespace offline_pages
