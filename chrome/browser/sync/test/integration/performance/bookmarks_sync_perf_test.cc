// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/performance/sync_timing_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks_helper::AddURL;
using bookmarks_helper::AllModelsMatch;
using bookmarks_helper::GetBookmarkBarNode;
using bookmarks_helper::IndexedURL;
using bookmarks_helper::IndexedURLTitle;
using bookmarks_helper::Remove;
using bookmarks_helper::SetURL;
using sync_timing_helper::PrintResult;
using sync_timing_helper::TimeMutualSyncCycle;

static const int kNumBookmarks = 150;

class BookmarksSyncPerfTest : public SyncTest {
 public:
  BookmarksSyncPerfTest()
      : SyncTest(TWO_CLIENT),
        url_number_(0),
        url_title_number_(0) {}

  // Adds |num_urls| new unique bookmarks to the bookmark bar for |profile|.
  void AddURLs(int profile, int num_urls);

  // Updates the URL for all bookmarks in the bookmark bar for |profile|.
  void UpdateURLs(int profile);

  // Removes all bookmarks in the bookmark bar for |profile|.
  void RemoveURLs(int profile);

  // Returns the number of bookmarks stored in the bookmark bar for |profile|.
  int GetURLCount(int profile);

 private:
  // Returns a new unique bookmark URL.
  std::string NextIndexedURL();

  // Returns a new unique bookmark title.
  std::string NextIndexedURLTitle();

  int url_number_;
  int url_title_number_;
  DISALLOW_COPY_AND_ASSIGN(BookmarksSyncPerfTest);
};

void BookmarksSyncPerfTest::AddURLs(int profile, int num_urls) {
  for (int i = 0; i < num_urls; ++i) {
    ASSERT_TRUE(AddURL(profile, 0, NextIndexedURLTitle(),
                       GURL(NextIndexedURL())) != nullptr);
  }
}

void BookmarksSyncPerfTest::UpdateURLs(int profile) {
  for (int i = 0;
       i < GetBookmarkBarNode(profile)->child_count();
       ++i) {
    ASSERT_TRUE(SetURL(profile,
                       GetBookmarkBarNode(profile)->GetChild(i),
                       GURL(NextIndexedURL())));
  }
}

void BookmarksSyncPerfTest::RemoveURLs(int profile) {
  while (!GetBookmarkBarNode(profile)->empty()) {
    Remove(profile, GetBookmarkBarNode(profile), 0);
  }
}

int BookmarksSyncPerfTest::GetURLCount(int profile) {
  return GetBookmarkBarNode(profile)->child_count();
}

std::string BookmarksSyncPerfTest::NextIndexedURL() {
  return IndexedURL(url_number_++);
}

std::string BookmarksSyncPerfTest::NextIndexedURLTitle() {
  return IndexedURLTitle(url_title_number_++);
}

IN_PROC_BROWSER_TEST_F(BookmarksSyncPerfTest, P0) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddURLs(0, kNumBookmarks);
  base::TimeDelta dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumBookmarks, GetURLCount(1));
  PrintResult("bookmarks", "add_bookmarks", dt);

  UpdateURLs(0);
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumBookmarks, GetURLCount(1));
  PrintResult("bookmarks", "update_bookmarks", dt);

  RemoveURLs(0);
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(0, GetURLCount(1));
  PrintResult("bookmarks", "delete_bookmarks", dt);
}
