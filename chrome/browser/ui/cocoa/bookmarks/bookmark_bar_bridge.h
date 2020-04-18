// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_BRIDGE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;
@class BookmarkBarController;

// C++ bridge class between Chromium and Cocoa to connect the
// Bookmarks (model) with the Bookmark Bar (view).
//
// There is exactly one BookmarkBarBridge per BookmarkBarController /
// BrowserWindowController / Browser.
class BookmarkBarBridge : public bookmarks::BookmarkModelObserver {
 public:
  BookmarkBarBridge(Profile* profile,
                    BookmarkBarController* controller,
                    bookmarks::BookmarkModel* model);
  ~BookmarkBarBridge() override;

  // bookmarks::BookmarkModelObserver:
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& removed_urls) override;
  void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model,
                                   const std::set<GURL>& removed_urls) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;
  void ExtensiveBookmarkChangesBeginning(
      bookmarks::BookmarkModel* model) override;
  void ExtensiveBookmarkChangesEnded(bookmarks::BookmarkModel* model) override;

 private:
  BookmarkBarController* controller_;  // weak; owns me
  bookmarks::BookmarkModel* model_;  // weak; it is owned by a Profile.
  bool batch_mode_;

  // Needed to react to kShowAppsShortcutInBookmarkBar changes.
  PrefChangeRegistrar profile_pref_registrar_;

  // Updates the visibility of the apps shortcut and the managed bookmarks
  // folder based on the pref values.
  void OnExtraButtonsVisibilityChanged();

  DISALLOW_COPY_AND_ASSIGN(BookmarkBarBridge);
};

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_BRIDGE_H_
