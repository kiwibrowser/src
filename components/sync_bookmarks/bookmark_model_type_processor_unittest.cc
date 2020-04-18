// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/bookmark_model_type_processor.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/sync/driver/fake_sync_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using testing::Eq;
using testing::NotNull;

namespace sync_bookmarks {

namespace {

// The parent tag for children of the root entity. Entities with this parent are
// referred to as top level enities.
const char kRootParentTag[] = "0";
const char kBookmarkBarTag[] = "bookmark_bar";
const char kBookmarksRootId[] = "32904_google_chrome_bookmarks";

struct BookmarkInfo {
  std::string server_id;
  std::string title;
  std::string url;  // empty for folders.
  std::string parent_id;
  std::string server_tag;
};

syncer::UpdateResponseData CreateUpdateData(const BookmarkInfo& bookmark_info) {
  syncer::EntityData data;
  data.id = bookmark_info.server_id;
  data.parent_id = bookmark_info.parent_id;
  data.server_defined_unique_tag = bookmark_info.server_tag;

  sync_pb::BookmarkSpecifics* bookmark_specifics =
      data.specifics.mutable_bookmark();
  bookmark_specifics->set_title(bookmark_info.title);
  if (bookmark_info.url.empty()) {
    data.is_folder = true;
  } else {
    bookmark_specifics->set_url(bookmark_info.url);
  }

  syncer::UpdateResponseData response_data;
  response_data.entity = data.PassToPtr();
  // Similar to what's done in the loopback_server.
  response_data.response_version = 0;
  return response_data;
}

void AssertState(const BookmarkModelTypeProcessor* processor,
                 const std::vector<BookmarkInfo>& bookmarks) {
  const SyncedBookmarkTracker* tracker = processor->GetTrackerForTest();

  // Make sure the tracker contains all bookmarks in |bookmarks| + the
  // bookmark bar node.
  ASSERT_THAT(tracker->TrackedEntitiesCountForTest(), Eq(bookmarks.size() + 1));

  for (BookmarkInfo bookmark : bookmarks) {
    const SyncedBookmarkTracker::Entity* entity =
        tracker->GetEntityForSyncId(bookmark.server_id);
    ASSERT_THAT(entity, NotNull());
    const bookmarks::BookmarkNode* node = entity->bookmark_node();
    ASSERT_THAT(node->GetTitle(), Eq(ASCIIToUTF16(bookmark.title)));
    ASSERT_THAT(node->url(), Eq(GURL(bookmark.url)));
    const SyncedBookmarkTracker::Entity* parent_entity =
        tracker->GetEntityForSyncId(bookmark.parent_id);
    ASSERT_THAT(node->parent(), parent_entity->bookmark_node());
  }
}

// TODO(crbug.com/516866): Replace with a simpler implementation (e.g. by
// simulating loading from metadata) instead of receiving remote updates.
// Inititalizes the processor with the bookmarks in |bookmarks|.
void InitWithSyncedBookmarks(const std::vector<BookmarkInfo>& bookmarks,
                             BookmarkModelTypeProcessor* processor) {
  syncer::UpdateResponseDataList updates;
  // Add update for the permanent folder "Bookmarks bar".
  updates.push_back(
      CreateUpdateData({"bookmark_bar", std::string(), std::string(),
                        kBookmarksRootId, kBookmarkBarTag}));
  for (BookmarkInfo bookmark : bookmarks) {
    updates.push_back(CreateUpdateData(bookmark));
  }
  processor->OnUpdateReceived(sync_pb::ModelTypeState(), updates);
  AssertState(processor, bookmarks);
}

syncer::UpdateResponseData CreateTombstone(const std::string& server_id) {
  // EntityData is considered to represent a deletion if its
  // specifics hasn't been set.
  syncer::EntityData data;
  data.id = server_id;

  syncer::UpdateResponseData response_data;
  response_data.entity = data.PassToPtr();
  return response_data;
}

syncer::UpdateResponseData CreateBookmarkRootUpdateData() {
  return CreateUpdateData({syncer::ModelTypeToRootTag(syncer::BOOKMARKS),
                           std::string(), std::string(), kRootParentTag,
                           syncer::ModelTypeToRootTag(syncer::BOOKMARKS)});
}

class TestSyncClient : public syncer::FakeSyncClient {
 public:
  explicit TestSyncClient(bookmarks::BookmarkModel* bookmark_model)
      : bookmark_model_(bookmark_model) {}

  bookmarks::BookmarkModel* GetBookmarkModel() override {
    return bookmark_model_;
  }

 private:
  bookmarks::BookmarkModel* bookmark_model_;
};

class BookmarkModelTypeProcessorTest : public testing::Test {
 public:
  BookmarkModelTypeProcessorTest()
      : bookmark_model_(bookmarks::TestBookmarkClient::CreateModel()),
        sync_client_(bookmark_model_.get()) {}

  TestSyncClient* sync_client() { return &sync_client_; }
  bookmarks::BookmarkModel* bookmark_model() { return bookmark_model_.get(); }

 private:
  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model_;
  TestSyncClient sync_client_;
};

TEST_F(BookmarkModelTypeProcessorTest, ReorderUpdatesShouldIgnoreRootNodes) {
  syncer::UpdateResponseDataList updates;
  updates.push_back(CreateBookmarkRootUpdateData());
  std::vector<const syncer::UpdateResponseData*> ordered_updates =
      BookmarkModelTypeProcessor::ReorderUpdatesForTest(updates);
  // Root node update should be filtered out.
  EXPECT_THAT(ordered_updates.size(), Eq(0U));
}

// TODO(crbug.com/516866): This should change to cover the general case of
// parents before children for non-deletions, and another test should be added
// for children before parents for deletions.
TEST_F(BookmarkModelTypeProcessorTest,
       ReorderUpdatesShouldPlacePermanentNodesFirstForNonDeletions) {
  const std::string kNode1Id = "node1";
  const std::string kNode2Id = "node2";
  syncer::UpdateResponseDataList updates;
  updates.push_back(CreateUpdateData(
      {kNode1Id, std::string(), std::string(), kNode2Id, std::string()}));
  updates.push_back(CreateUpdateData({kNode2Id, std::string(), std::string(),
                                      kBookmarksRootId, kBookmarkBarTag}));
  std::vector<const syncer::UpdateResponseData*> ordered_updates =
      BookmarkModelTypeProcessor::ReorderUpdatesForTest(updates);

  // No update should be dropped.
  ASSERT_THAT(ordered_updates.size(), Eq(2U));

  // Updates should be ordered such that parent node update comes first.
  EXPECT_THAT(ordered_updates[0]->entity.value().id, Eq(kNode2Id));
  EXPECT_THAT(ordered_updates[1]->entity.value().id, Eq(kNode1Id));
}

TEST_F(BookmarkModelTypeProcessorTest, ShouldUpdateModelAfterRemoteCreation) {
  BookmarkModelTypeProcessor processor(sync_client());

  syncer::UpdateResponseDataList updates;
  // Add update for the permanent folder "Bookmarks bar".
  updates.push_back(
      CreateUpdateData({"bookmark_bar", std::string(), std::string(),
                        kBookmarksRootId, kBookmarkBarTag}));

  // Add update for another node under the bookmarks bar.
  const std::string kNodeId = "node_id";
  const std::string kTitle = "title";
  const std::string kUrl = "http://www.url.com";
  updates.push_back(CreateUpdateData({kNodeId, kTitle, kUrl, kBookmarkBarTag,
                                      /*server_tag=*/std::string()}));

  const bookmarks::BookmarkNode* bookmarkbar =
      bookmark_model()->bookmark_bar_node();
  EXPECT_TRUE(bookmarkbar->empty());

  processor.OnUpdateReceived(sync_pb::ModelTypeState(), updates);

  ASSERT_THAT(bookmarkbar->GetChild(0), NotNull());
  EXPECT_THAT(bookmarkbar->GetChild(0)->GetTitle(), Eq(ASCIIToUTF16(kTitle)));
  EXPECT_THAT(bookmarkbar->GetChild(0)->url(), Eq(GURL(kUrl)));
}

TEST_F(BookmarkModelTypeProcessorTest, ShouldUpdateModelAfterRemoteUpdate) {
  BookmarkModelTypeProcessor processor(sync_client());

  const std::string kNodeId = "node_id";
  const std::string kTitle = "title";
  const std::string kUrl = "http://www.url.com";

  std::vector<BookmarkInfo> bookmarks = {
      {kNodeId, kTitle, kUrl, kBookmarkBarTag, /*server_tag=*/std::string()}};

  InitWithSyncedBookmarks(bookmarks, &processor);

  // Make sure original bookmark exists.
  const bookmarks::BookmarkNode* bookmark_bar =
      bookmark_model()->bookmark_bar_node();
  const bookmarks::BookmarkNode* bookmark_node = bookmark_bar->GetChild(0);
  ASSERT_THAT(bookmark_node, NotNull());
  ASSERT_THAT(bookmark_node->GetTitle(), Eq(ASCIIToUTF16(kTitle)));
  ASSERT_THAT(bookmark_node->url(), Eq(GURL(kUrl)));

  // Process an update for the same bookmark.
  const std::string kNewTitle = "new-title";
  const std::string kNewUrl = "http://www.new-url.com";
  syncer::UpdateResponseDataList updates;
  updates.push_back(
      CreateUpdateData({kNodeId, kNewTitle, kNewUrl, kBookmarkBarTag,
                        /*server_tag=*/std::string()}));
  processor.OnUpdateReceived(sync_pb::ModelTypeState(), updates);

  // Check if the bookmark has been updated properly.
  EXPECT_THAT(bookmark_bar->GetChild(0), Eq(bookmark_node));
  EXPECT_THAT(bookmark_node->GetTitle(), Eq(ASCIIToUTF16(kNewTitle)));
  EXPECT_THAT(bookmark_node->url(), Eq(GURL(kNewUrl)));
}

TEST_F(BookmarkModelTypeProcessorTest, ShouldUpdateModelAfterRemoteDelete) {
  BookmarkModelTypeProcessor processor(sync_client());
  // Build this structure
  // bookmark_bar
  //  |- folder1
  //      |- title1
  //      |- title2
  //  |- folder2
  //      |- title3

  // Then send delete updates in this order
  // folder1 -> title2 -> title1
  // to exercise the code of creating a |foster_parent|.

  const std::string kTitle1 = "title1";
  const std::string kTitle1Id = "title1Id";
  const std::string kTitle2 = "title2";
  const std::string kTitle2Id = "title2Id";
  const std::string kTitle3 = "title3";
  const std::string kTitle3Id = "title3Id";
  const std::string kFolder1 = "folder1";
  const std::string kFolder1Id = "folder1Id";
  const std::string kFolder2 = "folder2";
  const std::string kFolder2Id = "folder2Id";
  const std::string kUrl = "http://www.url.com";

  std::vector<BookmarkInfo> bookmarks = {
      {kFolder1Id, kFolder1, /*url=*/std::string(), kBookmarkBarTag,
       /*server_tag=*/std::string()},
      {kTitle1Id, kTitle1, kUrl, kFolder1Id, /*server_tag=*/std::string()},
      {kTitle2Id, kTitle2, kUrl, kFolder1Id, /*server_tag=*/std::string()},
      {kFolder2Id, kFolder2, /*url=*/std::string(), kBookmarkBarTag,
       /*server_tag=*/std::string()},
      {kTitle3Id, kTitle3, kUrl, kFolder2Id, /*server_tag=*/std::string()},
  };

  InitWithSyncedBookmarks(bookmarks, &processor);

  const bookmarks::BookmarkNode* bookmarkbar =
      bookmark_model()->bookmark_bar_node();

  ASSERT_THAT(bookmarkbar->child_count(), Eq(2));
  ASSERT_THAT(bookmarkbar->GetChild(0)->GetTitle(), Eq(ASCIIToUTF16(kFolder1)));
  ASSERT_THAT(bookmarkbar->GetChild(0)->child_count(), Eq(2));
  ASSERT_THAT(bookmarkbar->GetChild(1)->GetTitle(), Eq(ASCIIToUTF16(kFolder2)));
  ASSERT_THAT(bookmarkbar->GetChild(1)->child_count(), Eq(1));
  ASSERT_THAT(bookmarkbar->GetChild(1)->GetChild(0)->GetTitle(),
              Eq(ASCIIToUTF16(kTitle3)));

  // Process delete updates
  syncer::UpdateResponseDataList updates;
  updates.push_back(CreateTombstone(kTitle2Id));
  updates.push_back(CreateTombstone(kTitle1Id));
  updates.push_back(CreateTombstone(kFolder1Id));

  const sync_pb::ModelTypeState model_type_state;
  processor.OnUpdateReceived(model_type_state, updates);

  // The structure should be
  // bookmark_bar
  //  |- folder2
  //      |- title3
  EXPECT_THAT(bookmarkbar->child_count(), Eq(1));
  EXPECT_THAT(bookmarkbar->GetChild(0)->GetTitle(), Eq(ASCIIToUTF16(kFolder2)));
  EXPECT_THAT(bookmarkbar->GetChild(0)->child_count(), Eq(1));
  EXPECT_THAT(bookmarkbar->GetChild(0)->GetChild(0)->GetTitle(),
              Eq(ASCIIToUTF16(kTitle3)));
}

}  // namespace

}  // namespace sync_bookmarks
