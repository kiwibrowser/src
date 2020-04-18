// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/bookmarks/bookmark_suggestions_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/ntp_snippets/bookmarks/bookmark_last_visit_utils.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/mock_content_suggestions_provider_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_snippets {

namespace {

using ::testing::StrictMock;
using ::testing::_;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Property;
using ::testing::UnorderedElementsAre;

class BookmarkSuggestionsProviderTest : public ::testing::Test {
 public:
  BookmarkSuggestionsProviderTest()
      : model_(bookmarks::TestBookmarkClient::CreateModel()) {
    EXPECT_CALL(observer_,
                OnNewSuggestions(
                    _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
                    IsEmpty()))
        .RetiresOnSaturation();
    EXPECT_CALL(observer_,
                OnCategoryStatusChanged(
                    _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
                    CategoryStatus::AVAILABLE_LOADING))
        .RetiresOnSaturation();
    EXPECT_CALL(observer_,
                OnCategoryStatusChanged(
                    _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
                    CategoryStatus::AVAILABLE))
        .RetiresOnSaturation();
    provider_ =
        std::make_unique<BookmarkSuggestionsProvider>(&observer_, model_.get());
    scoped_task_environment_.RunUntilIdle();
  }

 protected:
  std::unique_ptr<bookmarks::BookmarkModel> model_;
  StrictMock<MockContentSuggestionsProviderObserver> observer_;
  std::unique_ptr<BookmarkSuggestionsProvider> provider_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(BookmarkSuggestionsProviderTest, ShouldProvideBookmarkSuggestions) {
  GURL url("http://my-new-bookmarked.url");
  // This update to the model does not trigger OnNewSuggestions() on the
  // observer as the provider realizes no new nodes were added.
  model_->AddURL(model_->bookmark_bar_node(), 0,
                 base::ASCIIToUTF16("cool page's title"), url);

  // Once we provided the last-visited meta information, an update with the
  // suggestion containing the bookmark should follow.
  EXPECT_CALL(
      observer_,
      OnNewSuggestions(
          _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
          UnorderedElementsAre(Property(&ContentSuggestion::url, GURL(url)))));
  UpdateBookmarkOnURLVisitedInMainFrame(model_.get(), url,
                                        /*is_mobile_platform=*/true);
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(BookmarkSuggestionsProviderTest,
       ShouldProvideBookmarkSuggestionsOnlyOnceForDuplicates) {
  GURL url("http://my-new-bookmarked.url");
  // This update to the model does not trigger OnNewSuggestions() on the
  // observer as the provider realizes no new nodes were added.
  model_->AddURL(model_->bookmark_bar_node(), 0,
                 base::ASCIIToUTF16("first cool page title"), url);
  model_->AddURL(model_->bookmark_bar_node(), 1,
                 base::ASCIIToUTF16("second cool page title"), url);

  // Once we provided the last-visited meta information, _only one_ update with
  // the suggestion containing the bookmark should follow.
  EXPECT_CALL(
      observer_,
      OnNewSuggestions(
          _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
          UnorderedElementsAre(Property(&ContentSuggestion::url, GURL(url)))))
      .Times(1);
  UpdateBookmarkOnURLVisitedInMainFrame(model_.get(), url,
                                        /*is_mobile_platform=*/true);
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(BookmarkSuggestionsProviderTest,
       ShouldEnsureToBeClearedBookmarksDontAppearAfterClear) {
  // Set up the provider with 2 entries: one dismissed and one active.

  // Add one bookmark (the one to be not dismissed) -- this will trigger a
  // notification.
  GURL active_bookmark("http://my-active-bookmarked.url");
  EXPECT_CALL(observer_,
              OnNewSuggestions(
                  _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
                  UnorderedElementsAre(Property(&ContentSuggestion::url,
                                                GURL(active_bookmark)))));
  model_->AddURL(model_->bookmark_bar_node(), 0,
                 base::ASCIIToUTF16("cool page's title"), active_bookmark);
  UpdateBookmarkOnURLVisitedInMainFrame(model_.get(), active_bookmark,
                                        /*is_mobile_platform=*/true);
  scoped_task_environment_.RunUntilIdle();

  // Add the other bookmark -- this will trigger another notification. Then
  // marks it was dismissed.
  GURL dismissed_bookmark("http://my-dismissed-bookmark.url");
  EXPECT_CALL(
      observer_,
      OnNewSuggestions(
          _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
          UnorderedElementsAre(
              Property(&ContentSuggestion::url, GURL(active_bookmark)),
              Property(&ContentSuggestion::url, GURL(dismissed_bookmark)))));
  const bookmarks::BookmarkNode* dismissed_node = model_->AddURL(
      model_->bookmark_bar_node(), 1, base::ASCIIToUTF16("cool page's title"),
      dismissed_bookmark);
  UpdateBookmarkOnURLVisitedInMainFrame(model_.get(), dismissed_bookmark,
                                        /*is_mobile_platform=*/true);
  scoped_task_environment_.RunUntilIdle();

  // According to the ContentSugestionsProvider contract, solely dismissing an
  // item should not result in another OnNewSuggestions() call.
  static_cast<ContentSuggestionsProvider*>(provider_.get())
      ->DismissSuggestion(ContentSuggestion::ID(
          Category::FromKnownCategory(KnownCategories::BOOKMARKS),
          dismissed_bookmark.spec()));
  EXPECT_THAT(IsDismissedFromNTPForBookmark(*dismissed_node), Eq(true));

  // Clear history and make sure the suggestions actually get removed.
  EXPECT_CALL(observer_,
              OnNewSuggestions(
                  _, Category::FromKnownCategory(KnownCategories::BOOKMARKS),
                  IsEmpty()));
  static_cast<ContentSuggestionsProvider*>(provider_.get())
      ->ClearHistory(base::Time(), base::Time::Max(),
                     base::Bind([](const GURL& url) { return true; }));
  scoped_task_environment_.RunUntilIdle();

  // Verify the dismissed marker is gone.
  EXPECT_THAT(IsDismissedFromNTPForBookmark(*dismissed_node), Eq(false));
}

// TODO(tschumann): There are plenty of test cases missing. Most importantly:
// -- Remove a bookmark from the model
// -- verifying handling of threshold time
// -- dealing with fetches before the model is loaded.

}  // namespace
}  // namespace ntp_snippets
