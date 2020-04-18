// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/sync_merge_result.h"

#include "base/location.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using SyncMergeResultTest = testing::Test;

TEST_F(SyncMergeResultTest, Unset) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_FALSE(merge_result.error().IsSet());
  EXPECT_EQ(0, merge_result.num_items_before_association());
  EXPECT_EQ(0, merge_result.num_items_after_association());
  EXPECT_EQ(0, merge_result.num_items_added());
  EXPECT_EQ(0, merge_result.num_items_deleted());
  EXPECT_EQ(0, merge_result.num_items_modified());
}

TEST_F(SyncMergeResultTest, SetError) {
  SyncError error(FROM_HERE, SyncError::DATATYPE_ERROR, "message", BOOKMARKS);
  SyncMergeResult merge_result(BOOKMARKS);

  merge_result.set_error(error);
  EXPECT_TRUE(merge_result.error().IsSet());
  EXPECT_EQ(BOOKMARKS, merge_result.model_type());
}

TEST_F(SyncMergeResultTest, SetNumItemsBeforeAssociation) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_EQ(0, merge_result.num_items_before_association());

  merge_result.set_num_items_before_association(10);
  EXPECT_EQ(10, merge_result.num_items_before_association());
}

TEST_F(SyncMergeResultTest, SetNumItemsAfterAssociation) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_EQ(0, merge_result.num_items_after_association());

  merge_result.set_num_items_after_association(10);
  EXPECT_EQ(10, merge_result.num_items_after_association());
}

TEST_F(SyncMergeResultTest, SetNumItemsAdded) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_EQ(0, merge_result.num_items_added());

  merge_result.set_num_items_added(10);
  EXPECT_EQ(10, merge_result.num_items_added());
}

TEST_F(SyncMergeResultTest, SetNumItemsDeleted) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_EQ(0, merge_result.num_items_deleted());

  merge_result.set_num_items_deleted(10);
  EXPECT_EQ(10, merge_result.num_items_deleted());
}

TEST_F(SyncMergeResultTest, SetNumItemsModified) {
  SyncMergeResult merge_result(BOOKMARKS);
  EXPECT_EQ(0, merge_result.num_items_modified());

  merge_result.set_num_items_modified(10);
  EXPECT_EQ(10, merge_result.num_items_modified());
}

}  // namespace

}  // namespace syncer
