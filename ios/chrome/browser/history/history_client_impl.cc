// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/history/history_client_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/history/core/browser/history_service.h"
#include "ios/chrome/browser/history/history_backend_client_impl.h"
#include "ios/chrome/browser/history/history_utils.h"
#include "url/gurl.h"

HistoryClientImpl::HistoryClientImpl(bookmarks::BookmarkModel* bookmark_model)
    : bookmark_model_(bookmark_model), is_bookmark_model_observer_(false) {
}

HistoryClientImpl::~HistoryClientImpl() {
}

void HistoryClientImpl::OnHistoryServiceCreated(
    history::HistoryService* history_service) {
  DCHECK(!is_bookmark_model_observer_);
  if (bookmark_model_) {
    on_bookmarks_removed_ =
        base::Bind(&history::HistoryService::URLsNoLongerBookmarked,
                   base::Unretained(history_service));
    favicons_changed_subscription_ =
        history_service->AddFaviconsChangedCallback(
            base::Bind(&bookmarks::BookmarkModel::OnFaviconsChanged,
                       base::Unretained(bookmark_model_)));
    bookmark_model_->AddObserver(this);
    is_bookmark_model_observer_ = true;
  }
}

void HistoryClientImpl::Shutdown() {
  // It's possible that bookmarks haven't loaded and history is waiting for
  // bookmarks to complete loading. In such a situation history can't shutdown
  // (meaning if we invoked HistoryService::Cleanup now, we would deadlock). To
  // break the deadlock we tell BookmarkModel it's about to be deleted so that
  // it can release the signal history is waiting on, allowing history to
  // shutdown (HistoryService::Cleanup to complete). In such a scenario history
  // sees an incorrect view of bookmarks, but it's better than a deadlock.
  if (bookmark_model_) {
    if (is_bookmark_model_observer_) {
      is_bookmark_model_observer_ = false;
      bookmark_model_->RemoveObserver(this);
      favicons_changed_subscription_.reset();
      on_bookmarks_removed_.Reset();
    }
    bookmark_model_->Shutdown();
  }
}

bool HistoryClientImpl::CanAddURL(const GURL& url) {
  return ios::CanAddURLToHistory(url);
}

void HistoryClientImpl::NotifyProfileError(sql::InitStatus init_status,
                                           const std::string& diagnostics) {}

std::unique_ptr<history::HistoryBackendClient>
HistoryClientImpl::CreateBackendClient() {
  return std::make_unique<HistoryBackendClientImpl>(bookmark_model_);
}

void HistoryClientImpl::BookmarkModelChanged() {
}

void HistoryClientImpl::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {
  if (!on_bookmarks_removed_.is_null())
    on_bookmarks_removed_.Run(no_longer_bookmarked);
}

void HistoryClientImpl::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  if (!on_bookmarks_removed_.is_null())
    on_bookmarks_removed_.Run(removed_urls);
}
