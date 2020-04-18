// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/bookmarks/bookmark_last_visit_utils.h"

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/ntp_snippets/time_serialization.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

using testing::Eq;
using testing::IsEmpty;
using testing::SizeIs;

namespace ntp_snippets {

namespace {

const char kBookmarkLastVisitDateOnMobileKey[] = "last_visited";
const char kBookmarkLastVisitDateOnDesktopKey[] = "last_visited_desktop";

void AddBookmarks(BookmarkModel* model,
                  int num,
                  const std::string& meta_key,
                  const std::string& meta_value) {
  for (int index = 0; index < num; ++index) {
    base::string16 title = base::ASCIIToUTF16(
        base::StringPrintf("title%s%d", meta_key.c_str(), index));
    GURL url(base::StringPrintf("http://url%s%d.com", meta_key.c_str(), index));
    const BookmarkNode* node =
        model->AddURL(model->bookmark_bar_node(), 0, title, url);

    if (!meta_key.empty()) {
      model->SetNodeMetaInfo(node, meta_key, meta_value);
    }
  }
}

void AddBookmarksRecentOnMobile(BookmarkModel* model,
                                int num,
                                const base::Time& visit_time) {
  AddBookmarks(model, num, kBookmarkLastVisitDateOnMobileKey,
               base::Int64ToString(SerializeTime(visit_time)));
}

void AddBookmarksRecentOnDesktop(BookmarkModel* model,
                                 int num,
                                 const base::Time& visit_time) {
  AddBookmarks(model, num, kBookmarkLastVisitDateOnDesktopKey,
               base::Int64ToString(SerializeTime(visit_time)));
}

void AddBookmarksNonVisited(BookmarkModel* model, int num) {
  AddBookmarks(model, num, std::string(), std::string());
}

const BookmarkNode* AddSingleBookmark(BookmarkModel* model,
                                      const std::string& url,
                                      const std::string& last_visit_key,
                                      const base::Time& visit_time) {
  base::string16 title =
      base::ASCIIToUTF16(base::StringPrintf("title-%s", url.c_str()));
  const BookmarkNode* node =
      model->AddURL(model->bookmark_bar_node(), 0, title, GURL(url));
  model->SetNodeMetaInfo(node, last_visit_key,
                         base::Int64ToString(SerializeTime(visit_time)));
  return node;
}

}  // namespace

class GetRecentlyVisitedBookmarksTest : public testing::Test {
 public:
  GetRecentlyVisitedBookmarksTest() {
    base::TimeDelta week = base::TimeDelta::FromDays(7);
    threshold_time_ = base::Time::UnixEpoch() + 52 * week;
  }

  const base::Time& threshold_time() const { return threshold_time_; }

  base::Time GetRecentTime() const {
    return threshold_time_ + base::TimeDelta::FromDays(7);
  }

 private:
  base::Time threshold_time_;

  DISALLOW_COPY_AND_ASSIGN(GetRecentlyVisitedBookmarksTest);
};

TEST_F(GetRecentlyVisitedBookmarksTest, ShouldNotReturnMissing) {
  const int number_of_bookmarks = 3;
  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddBookmarksNonVisited(model.get(), number_of_bookmarks);

  std::vector<const bookmarks::BookmarkNode*> result =
      GetRecentlyVisitedBookmarks(model.get(), number_of_bookmarks,
                                  threshold_time(),
                                  /*consider_visits_from_desktop=*/false);
  EXPECT_THAT(result, IsEmpty());
}

TEST_F(GetRecentlyVisitedBookmarksTest, ShouldNotConsiderDesktopVisits) {
  const int number_of_bookmarks = 3;
  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddBookmarksRecentOnDesktop(model.get(), number_of_bookmarks,
                              GetRecentTime());

  std::vector<const bookmarks::BookmarkNode*> result =
      GetRecentlyVisitedBookmarks(model.get(), number_of_bookmarks,
                                  threshold_time(),
                                  /*consider_visits_from_desktop=*/false);
  EXPECT_THAT(result, IsEmpty());
}

TEST_F(GetRecentlyVisitedBookmarksTest, ShouldConsiderDesktopVisits) {
  const int number_of_bookmarks = 3;
  const int number_of_recent_desktop = 2;
  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddBookmarksRecentOnDesktop(model.get(), number_of_recent_desktop,
                              GetRecentTime());
  AddBookmarksNonVisited(model.get(),
                         number_of_bookmarks - number_of_recent_desktop);

  std::vector<const bookmarks::BookmarkNode*> result =
      GetRecentlyVisitedBookmarks(model.get(), number_of_bookmarks,
                                  threshold_time(),
                                  /*consider_visits_from_desktop=*/true);
  EXPECT_THAT(result, SizeIs(number_of_recent_desktop));
}

TEST_F(GetRecentlyVisitedBookmarksTest, ShouldReturnNotMoreThanMaxCount) {
  const int number_of_bookmarks = 3;
  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddBookmarksRecentOnMobile(model.get(), number_of_bookmarks, GetRecentTime());

  const int max_count = number_of_bookmarks - 1;
  std::vector<const bookmarks::BookmarkNode*> result =
      GetRecentlyVisitedBookmarks(model.get(), max_count, threshold_time(),
                                  /*consider_visits_from_desktop=*/false);
  EXPECT_THAT(result, SizeIs(max_count));
}

namespace {

base::Callback<bool(const GURL& url)> DeleteAllFilter() {
  return base::Bind([](const GURL& url) { return true; });
}

base::Callback<bool(const GURL& url)> DeleteOneURLFilter(
    const GURL& to_delete) {
  return base::Bind(
      [](const GURL& to_delete, const GURL& url) { return url == to_delete; },
      to_delete);
}

}  // namespace

TEST(RemoveLastVisitedDatesBetween, ShouldRemoveTimestampsWithinTimeRange) {
  const base::Time delete_begin =
      base::Time::Now() - base::TimeDelta::FromDays(2);
  const base::Time delete_end = base::Time::Max();

  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddSingleBookmark(model.get(), "http://url-1.com",
                    kBookmarkLastVisitDateOnMobileKey,
                    delete_begin + base::TimeDelta::FromSeconds(1));
  AddSingleBookmark(model.get(), "http://url-1.com",
                    kBookmarkLastVisitDateOnDesktopKey,
                    delete_begin + base::TimeDelta::FromSeconds(1));
  ASSERT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      SizeIs(1));

  RemoveLastVisitedDatesBetween(delete_begin, delete_end, DeleteAllFilter(),
                                model.get());

  EXPECT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      IsEmpty());
  // Verify that the bookmark model nodes themselve still exist.
  std::vector<const BookmarkNode*> remaining_nodes;
  model->GetNodesByURL(GURL("http://url-1.com"), &remaining_nodes);
  EXPECT_THAT(remaining_nodes, SizeIs(2));
}

TEST(RemoveLastVisitedDatesBetween,
     ShouldHandleMetadataFromOtherDeviceTypesSeparately) {
  const base::Time delete_begin =
      base::Time::Now() - base::TimeDelta::FromDays(2);
  const base::Time delete_end = base::Time::Max();

  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  // Create a bookmark with last visited times from both, mobile and desktop.
  // The mobile one is within the deletion interval, the desktop one outside.
  // Only the mobile one should get deleted.
  const BookmarkNode* node = AddSingleBookmark(
      model.get(), "http://url-1.com", kBookmarkLastVisitDateOnMobileKey,
      delete_begin + base::TimeDelta::FromSeconds(1));
  model->SetNodeMetaInfo(node, kBookmarkLastVisitDateOnDesktopKey,
                         base::Int64ToString(SerializeTime(
                             delete_begin - base::TimeDelta::FromSeconds(1))));
  ASSERT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      SizeIs(1));

  RemoveLastVisitedDatesBetween(delete_begin, delete_end, DeleteAllFilter(),
                                model.get());

  EXPECT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/false),
      IsEmpty());
  EXPECT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      SizeIs(1));
}

TEST(RemoveLastVisitedDatesBetween, ShouldNotRemoveTimestampsOutsideTimeRange) {
  const base::Time delete_begin =
      base::Time::Now() - base::TimeDelta::FromDays(2);
  const base::Time delete_end = delete_begin + base::TimeDelta::FromDays(5);

  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddSingleBookmark(model.get(), "http://url-1.com",
                    kBookmarkLastVisitDateOnMobileKey,
                    delete_begin - base::TimeDelta::FromSeconds(1));
  AddSingleBookmark(model.get(), "http://url-2.com",
                    kBookmarkLastVisitDateOnDesktopKey,
                    delete_end + base::TimeDelta::FromSeconds(1));
  ASSERT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      SizeIs(2));

  RemoveLastVisitedDatesBetween(delete_begin, delete_end, DeleteAllFilter(),
                                model.get());

  EXPECT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/true),
      SizeIs(2));
}

TEST(RemoveLastVisitedDatesBetween, ShouldOnlyRemoveURLsWithinFilter) {
  const base::Time delete_begin =
      base::Time::Now() - base::TimeDelta::FromDays(2);
  const base::Time delete_end = base::Time::Max();

  std::unique_ptr<BookmarkModel> model =
      bookmarks::TestBookmarkClient::CreateModel();
  AddSingleBookmark(model.get(), "http://url-1.com",
                    kBookmarkLastVisitDateOnMobileKey,
                    delete_begin + base::TimeDelta::FromSeconds(1));
  AddSingleBookmark(model.get(), "http://url-2.com",
                    kBookmarkLastVisitDateOnMobileKey,
                    delete_begin + base::TimeDelta::FromSeconds(1));
  ASSERT_THAT(
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/false),
      SizeIs(2));

  RemoveLastVisitedDatesBetween(delete_begin, delete_end,
                                DeleteOneURLFilter(GURL("http://url-2.com")),
                                model.get());

  std::vector<const bookmarks::BookmarkNode*> remaining_nodes =
      GetRecentlyVisitedBookmarks(model.get(), 20, base::Time(),
                                  /*consider_visits_from_desktop=*/false);
  EXPECT_THAT(remaining_nodes, SizeIs(1));
  EXPECT_THAT(remaining_nodes[0]->url(), Eq(GURL("http://url-1.com")));
}

}  // namespace ntp_snippets
