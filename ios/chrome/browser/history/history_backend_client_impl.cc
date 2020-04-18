// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/history/history_backend_client_impl.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/url_and_title.h"
#include "url/gurl.h"

HistoryBackendClientImpl::HistoryBackendClientImpl(
    bookmarks::BookmarkModel* bookmark_model)
    : bookmark_model_(bookmark_model) {
}

HistoryBackendClientImpl::~HistoryBackendClientImpl() {
}

bool HistoryBackendClientImpl::IsBookmarked(const GURL& url) {
  if (!bookmark_model_)
    return false;

  // HistoryBackendClient is used to determine if an URL is bookmarked. The data
  // is loaded on a separate thread and may not be done when this method is
  // called, therefore blocks until the bookmarks have finished loading.
  bookmark_model_->BlockTillLoaded();
  return bookmark_model_->IsBookmarked(url);
}

void HistoryBackendClientImpl::GetBookmarks(
    std::vector<history::URLAndTitle>* bookmarks) {
  if (!bookmark_model_)
    return;

  // HistoryBackendClient is used to determine the set of bookmarked URLs. The
  // data is loaded on a separate thread and may not be done when this method is
  // called, therefore blocks until the bookmarks have finished loading.
  std::vector<bookmarks::UrlAndTitle> url_and_titles;
  bookmark_model_->BlockTillLoaded();
  bookmark_model_->GetBookmarks(&url_and_titles);

  bookmarks->reserve(bookmarks->size() + url_and_titles.size());
  for (const auto& url_and_title : url_and_titles) {
    history::URLAndTitle value = {url_and_title.url, url_and_title.title};
    bookmarks->push_back(value);
  }
}

bool HistoryBackendClientImpl::ShouldReportDatabaseError() {
  return false;
}

bool HistoryBackendClientImpl::IsWebSafe(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS();
}
