// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NTP_SNIPPETS_BOOKMARK_LAST_VISIT_UPDATER_H_
#define CHROME_BROWSER_NTP_SNIPPETS_BOOKMARK_LAST_VISIT_UPDATER_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

// A bridge class between platform-specific content::WebContentsObserver and the
// generic function ntp_snippets::UpdateBookmarkOnURLVisitedInMainFrame().
class BookmarkLastVisitUpdater
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BookmarkLastVisitUpdater>,
      public bookmarks::BookmarkModelObserver {
 public:
  ~BookmarkLastVisitUpdater() override;

  static void MaybeCreateForWebContentsWithBookmarkModel(
      content::WebContents* web_contents,
      bookmarks::BookmarkModel* bookmark_model);

 private:
  friend class content::WebContentsUserData<BookmarkLastVisitUpdater>;

  BookmarkLastVisitUpdater(content::WebContents* web_contents,
                           bookmarks::BookmarkModel* bookmark_model);

  // Overridden from BookmarkModelObserver:
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override {}
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override {}
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* parent,
      int old_index,
      const bookmarks::BookmarkNode* node,
      const std::set<GURL>& no_longer_bookmarked) override {}
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override {}
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override {}
  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override {}
  void BookmarkNodeFaviconChanged(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override {}

  // Overridden from content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;

  void NewURLVisited(content::NavigationHandle* navigation_handle);

  bookmarks::BookmarkModel* bookmark_model_;
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkLastVisitUpdater);
};

#endif  // CHROME_BROWSER_NTP_SNIPPETS_BOOKMARK_LAST_VISIT_UPDATER_H_
