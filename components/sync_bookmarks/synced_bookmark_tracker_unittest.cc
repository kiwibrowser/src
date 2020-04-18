// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/synced_bookmark_tracker.h"

#include "components/bookmarks/browser/bookmark_node.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Eq;
using testing::IsNull;
using testing::NotNull;

namespace sync_bookmarks {

namespace {

TEST(SyncedBookmarkTrackerTest, ShouldGetAssociatedNodes) {
  SyncedBookmarkTracker tracker;
  const std::string kSyncId = "SYNC_ID";
  const int64_t kId = 1;
  bookmarks::BookmarkNode node(kId, GURL());
  tracker.Associate(kSyncId, &node);
  const SyncedBookmarkTracker::Entity* entity =
      tracker.GetEntityForSyncId(kSyncId);
  ASSERT_THAT(entity, NotNull());
  EXPECT_THAT(entity->bookmark_node(), Eq(&node));
  EXPECT_THAT(tracker.GetEntityForSyncId("unknown id"), IsNull());
}

TEST(SyncedBookmarkTrackerTest, ShouldReturnNullForDisassociatedNodes) {
  SyncedBookmarkTracker tracker;
  const std::string kSyncId = "SYNC_ID";
  const int64_t kId = 1;
  bookmarks::BookmarkNode node(kId, GURL());
  tracker.Associate(kSyncId, &node);
  ASSERT_THAT(tracker.GetEntityForSyncId(kSyncId), NotNull());
  tracker.Disassociate(kSyncId);
  EXPECT_THAT(tracker.GetEntityForSyncId(kSyncId), IsNull());
}

}  // namespace

}  // namespace sync_bookmarks
