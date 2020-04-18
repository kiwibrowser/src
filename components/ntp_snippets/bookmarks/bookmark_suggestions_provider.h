// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_SUGGESTIONS_PROVIDER_H_
#define COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_SUGGESTIONS_PROVIDER_H_

#include <set>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_status.h"
#include "components/ntp_snippets/content_suggestions_provider.h"

namespace ntp_snippets {

// Provides content suggestions from the bookmarks model.
class BookmarkSuggestionsProvider : public ContentSuggestionsProvider,
                                    public bookmarks::BookmarkModelObserver {
 public:
  BookmarkSuggestionsProvider(ContentSuggestionsProvider::Observer* observer,
                              bookmarks::BookmarkModel* bookmark_model);
  ~BookmarkSuggestionsProvider() override;

 private:
  // ContentSuggestionsProvider implementation.
  CategoryStatus GetCategoryStatus(Category category) override;
  CategoryInfo GetCategoryInfo(Category category) override;
  void DismissSuggestion(const ContentSuggestion::ID& suggestion_id) override;
  void FetchSuggestionImage(const ContentSuggestion::ID& suggestion_id,
                            ImageFetchedCallback callback) override;
  void FetchSuggestionImageData(const ContentSuggestion::ID& suggestion_id,
                                ImageDataFetchedCallback callback) override;
  void Fetch(const Category& category,
             const std::set<std::string>& known_suggestion_ids,
             FetchDoneCallback callback) override;
  void ClearHistory(
      base::Time begin,
      base::Time end,
      const base::Callback<bool(const GURL& url)>& filter) override;
  void ClearCachedSuggestions() override;
  void GetDismissedSuggestionsForDebugging(
      Category category,
      DismissedSuggestionsCallback callback) override;
  void ClearDismissedSuggestionsForDebugging(Category category) override;

  // bookmarks::BookmarkModelObserver implementation.
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void OnWillChangeBookmarkMetaInfo(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override;

  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override {}
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override {}
  void BookmarkNodeFaviconChanged(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override {}
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override {}
  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override {}

  void ConvertBookmark(const bookmarks::BookmarkNode& bookmark,
                       std::vector<ContentSuggestion>* suggestions);

  // The actual method to fetch bookmarks - follows each call to FetchBookmarks
  // but not sooner than the BookmarkModel gets loaded.
  void FetchBookmarksInternal();

  // Queries the BookmarkModel for recently visited bookmarks and pushes the
  // results to the ContentSuggestionService. The actual fetching does not
  // happen before the Bookmark model gets loaded.
  void FetchBookmarks();

  // Updates the |category_status_| and notifies the |observer_|, if necessary.
  void NotifyStatusChanged(CategoryStatus new_status);

  CategoryStatus category_status_;
  const Category provided_category_;
  bookmarks::BookmarkModel* bookmark_model_;
  bool fetch_requested_;
  bool fetch_in_progress_;

  base::Time node_to_change_last_visit_date_;
  base::Time end_of_list_last_visit_date_;

  // By default, only visits to bookmarks on Android are considered when
  // deciding which bookmarks to suggest. Should we also consider visits on
  // desktop platforms?
  bool consider_bookmark_visits_from_desktop_;

  base::WeakPtrFactory<BookmarkSuggestionsProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkSuggestionsProvider);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_SUGGESTIONS_PROVIDER_H_
