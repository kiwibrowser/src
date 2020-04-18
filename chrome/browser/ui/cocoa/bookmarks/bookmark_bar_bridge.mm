// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_bridge.h"

#include "base/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

BookmarkBarBridge::BookmarkBarBridge(Profile* profile,
                                     BookmarkBarController* controller,
                                     BookmarkModel* model)
    : controller_(controller),
      model_(model),
      batch_mode_(false) {
  model_->AddObserver(this);

  // Bookmark loading is async; it may may not have happened yet.
  // We will be notified when that happens with the AddObserver() call.
  if (model->loaded())
    BookmarkModelLoaded(model, false);

  profile_pref_registrar_.Init(profile->GetPrefs());
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
      base::Bind(&BookmarkBarBridge::OnExtraButtonsVisibilityChanged,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowManagedBookmarksInBookmarkBar,
      base::Bind(&BookmarkBarBridge::OnExtraButtonsVisibilityChanged,
                 base::Unretained(this)));

  OnExtraButtonsVisibilityChanged();
}

BookmarkBarBridge::~BookmarkBarBridge() {
  if (model_)
    model_->RemoveObserver(this);
}

void BookmarkBarBridge::BookmarkModelLoaded(BookmarkModel* model,
                                            bool ids_reassigned) {
  [controller_ loaded:model];
}

void BookmarkBarBridge::BookmarkModelBeingDeleted(BookmarkModel* model) {
  model_->RemoveObserver(this);
  model_ = nullptr;
  // The browser may be being torn down; little is safe to do.  As an
  // example, it may not be safe to clear the pasteboard.
  // http://crbug.com/38665
}

void BookmarkBarBridge::BookmarkNodeMoved(BookmarkModel* model,
                                          const BookmarkNode* old_parent,
                                          int old_index,
                                          const BookmarkNode* new_parent,
                                          int new_index) {
  if (!batch_mode_) {
    [controller_ nodeMoved:model
                 oldParent:old_parent oldIndex:old_index
                 newParent:new_parent newIndex:new_index];
  }
}

void BookmarkBarBridge::BookmarkNodeAdded(BookmarkModel* model,
                                          const BookmarkNode* parent,
                                          int index) {
  if (!batch_mode_)
    [controller_ nodeAdded:model parent:parent index:index];
}

void BookmarkBarBridge::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int old_index,
    const BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
  if (!batch_mode_)
    [controller_ nodeRemoved:model parent:parent index:old_index];
}

void BookmarkBarBridge::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  [controller_ loaded:model];
}

void BookmarkBarBridge::BookmarkNodeChanged(BookmarkModel* model,
                                            const BookmarkNode* node) {
  if (!batch_mode_)
    [controller_ nodeChanged:model node:node];
}

void BookmarkBarBridge::BookmarkNodeFaviconChanged(BookmarkModel* model,
                                                   const BookmarkNode* node) {
  if (!batch_mode_)
    [controller_ nodeFaviconLoaded:model node:node];
}

void BookmarkBarBridge::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
  if (!batch_mode_)
    [controller_ nodeChildrenReordered:model node:node];
}

void BookmarkBarBridge::ExtensiveBookmarkChangesBeginning(
    BookmarkModel* model) {
  batch_mode_ = true;
}

void BookmarkBarBridge::ExtensiveBookmarkChangesEnded(BookmarkModel* model) {
  batch_mode_ = false;
  [controller_ loaded:model];
}

void BookmarkBarBridge::OnExtraButtonsVisibilityChanged() {
  [controller_ updateExtraButtonsVisibility];
}
