// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_thumbnail.h"

#include "components/offline_pages/core/offline_store_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {

const base::Time kTestTime = store_utils::FromDatabaseTime(42);
const char kThumbnailData[] = "abc123";

TEST(OfflinePageThumbnailTest, Construct) {
  OfflinePageThumbnail thumbnail(1, kTestTime, kThumbnailData);
  EXPECT_EQ(1, thumbnail.offline_id);
  EXPECT_EQ(kTestTime, thumbnail.expiration);
  EXPECT_EQ(kThumbnailData, thumbnail.thumbnail);
}

TEST(OfflinePageThumbnailTest, Equal) {
  OfflinePageThumbnail thumbnail(1, kTestTime, kThumbnailData);
  auto copy = thumbnail;

  EXPECT_EQ(1, copy.offline_id);
  EXPECT_EQ(kTestTime, copy.expiration);
  EXPECT_EQ(kThumbnailData, copy.thumbnail);
  EXPECT_EQ(copy, thumbnail);
}

TEST(OfflinePageThumbnailTest, Compare) {
  OfflinePageThumbnail thumbnail_a(1, kTestTime, kThumbnailData);
  OfflinePageThumbnail thumbnail_b(2, kTestTime, kThumbnailData);

  EXPECT_TRUE(thumbnail_a < thumbnail_b);
  EXPECT_FALSE(thumbnail_b < thumbnail_a);
  EXPECT_FALSE(thumbnail_a < thumbnail_a);
}

TEST(OfflinePageThumbnailTest, ToString) {
  OfflinePageThumbnail thumbnail(1, kTestTime, kThumbnailData);

  EXPECT_EQ("OfflinePageThumbnail(1, 42, YWJjMTIz)", thumbnail.ToString());
}

}  // namespace
}  // namespace offline_pages
