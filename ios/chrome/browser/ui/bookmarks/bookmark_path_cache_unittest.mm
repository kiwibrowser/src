// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/bookmarks/bookmark_path_cache.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "ios/chrome/browser/ui/bookmarks/bookmark_ios_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkNode;

namespace {

class BookmarkPathCacheTest : public BookmarkIOSUnitTest {
 protected:
  void SetUp() override {
    BookmarkIOSUnitTest::SetUp();
    [BookmarkPathCache registerBrowserStatePrefs:prefs_.registry()];
  }

  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(BookmarkPathCacheTest, TestPathCache) {
  // Try to store and retrieve a cache.
  const BookmarkNode* mobileNode = _bookmarkModel->mobile_node();
  const BookmarkNode* f1 = AddFolder(mobileNode, @"f1");
  int64_t folderId = f1->id();
  double position = 23;
  [BookmarkPathCache cacheBookmarkUIPositionWithPrefService:&prefs_
                                                   folderId:folderId
                                             scrollPosition:position];

  int64_t resultFolderId;
  double resultPosition;
  [BookmarkPathCache getBookmarkUIPositionCacheWithPrefService:&prefs_
                                                         model:_bookmarkModel
                                                      folderId:&resultFolderId
                                                scrollPosition:&resultPosition];
  EXPECT_EQ(folderId, resultFolderId);
  EXPECT_EQ(position, resultPosition);
}

TEST_F(BookmarkPathCacheTest, TestPathCacheWhenFolderDeleted) {
  // Try to store and retrieve a cache after the cached path is deleted.
  const BookmarkNode* mobileNode = _bookmarkModel->mobile_node();
  const BookmarkNode* f1 = AddFolder(mobileNode, @"f1");
  int64_t folderId = f1->id();
  double position = 23;
  [BookmarkPathCache cacheBookmarkUIPositionWithPrefService:&prefs_
                                                   folderId:folderId
                                             scrollPosition:position];

  // Delete the folder.
  _bookmarkModel->Remove(f1);

  int64_t unusedFolderId;
  double unusedPosition;
  BOOL result = [BookmarkPathCache
      getBookmarkUIPositionCacheWithPrefService:&prefs_
                                          model:_bookmarkModel
                                       folderId:&unusedFolderId
                                 scrollPosition:&unusedPosition];
  ASSERT_FALSE(result);
}

}  // anonymous namespace
