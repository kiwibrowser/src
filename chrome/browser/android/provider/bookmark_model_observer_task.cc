// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/provider/bookmark_model_observer_task.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "content/public/browser/browser_thread.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::BrowserThread;

BookmarkModelTask::BookmarkModelTask(BookmarkModel* model)
    : model_(model) {
  // Ensure the initialization of the native bookmark model.
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(model_);
  model_->BlockTillLoaded();
}

BookmarkModel* BookmarkModelTask::model() const {
  return model_;
}

BookmarkModelObserverTask::BookmarkModelObserverTask(
    BookmarkModel* bookmark_model)
    : BookmarkModelTask(bookmark_model) {
  model()->AddObserver(this);
}

BookmarkModelObserverTask::~BookmarkModelObserverTask() {
  model()->RemoveObserver(this);
}

void BookmarkModelObserverTask::BookmarkModelLoaded(BookmarkModel* model,
                                                    bool ids_reassigned) {}

void BookmarkModelObserverTask::BookmarkNodeMoved(
    BookmarkModel* model,
    const BookmarkNode* old_parent,
    int old_index,
    const BookmarkNode* new_parent,
    int new_index) {
}

void BookmarkModelObserverTask::BookmarkNodeAdded(BookmarkModel* model,
                                                  const BookmarkNode* parent,
                                                  int index) {
}

void BookmarkModelObserverTask::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int old_index,
    const BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
}

void BookmarkModelObserverTask::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
}

void BookmarkModelObserverTask::BookmarkNodeChanged(BookmarkModel* model,
                                                    const BookmarkNode* node) {
}

void BookmarkModelObserverTask::BookmarkNodeFaviconChanged(
    BookmarkModel* model,
    const BookmarkNode* node) {
}

void BookmarkModelObserverTask::BookmarkNodeChildrenReordered(
    BookmarkModel* model,
    const BookmarkNode* node) {
}
