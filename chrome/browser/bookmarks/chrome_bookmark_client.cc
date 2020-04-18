// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/chrome_bookmark_client.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/bookmarks/managed/managed_bookmark_util.h"
#include "components/favicon/core/favicon_util.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/url_database.h"
#include "components/offline_pages/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/offline_page_bookmark_observer.h"
#endif

ChromeBookmarkClient::ChromeBookmarkClient(
    Profile* profile,
    bookmarks::ManagedBookmarkService* managed_bookmark_service)
    : profile_(profile), managed_bookmark_service_(managed_bookmark_service) {}

ChromeBookmarkClient::~ChromeBookmarkClient() {
}

void ChromeBookmarkClient::Init(bookmarks::BookmarkModel* model) {
  if (managed_bookmark_service_)
    managed_bookmark_service_->BookmarkModelCreated(model);

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  offline_page_observer_ =
      std::make_unique<offline_pages::OfflinePageBookmarkObserver>(profile_);
  model->AddObserver(offline_page_observer_.get());
#endif
}

bool ChromeBookmarkClient::PreferTouchIcon() {
  return false;
}

base::CancelableTaskTracker::TaskId
ChromeBookmarkClient::GetFaviconImageForPageURL(
    const GURL& page_url,
    favicon_base::IconType type,
    const favicon_base::FaviconImageCallback& callback,
    base::CancelableTaskTracker* tracker) {
  return favicon::GetFaviconImageForPageURL(
      FaviconServiceFactory::GetForProfile(profile_,
                                           ServiceAccessType::EXPLICIT_ACCESS),
      page_url, type, callback, tracker);
}

bool ChromeBookmarkClient::SupportsTypedCountForUrls() {
  return true;
}

void ChromeBookmarkClient::GetTypedCountForUrls(
    UrlTypedCountMap* url_typed_count_map) {
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfileIfExists(
          profile_, ServiceAccessType::EXPLICIT_ACCESS);
  history::URLDatabase* url_db =
      history_service ? history_service->InMemoryDatabase() : nullptr;
  for (auto& url_typed_count_pair : *url_typed_count_map) {
    int typed_count = 0;

    // If |url_db| is the InMemoryDatabase, it might not cache all URLRows, but
    // it guarantees to contain those with |typed_count| > 0. Thus, if we cannot
    // fetch the URLRow, it is safe to assume that its |typed_count| is 0.
    history::URLRow url_row;
    const GURL* url = url_typed_count_pair.first;
    if (url_db && url && url_db->GetRowForURL(*url, &url_row))
      typed_count = url_row.typed_count();

    url_typed_count_pair.second = typed_count;
  }
}

bool ChromeBookmarkClient::IsPermanentNodeVisible(
    const bookmarks::BookmarkPermanentNode* node) {
  DCHECK(bookmarks::IsPermanentNode(node, managed_bookmark_service_));
  if (bookmarks::IsManagedNode(node, managed_bookmark_service_))
    return false;
#if defined(OS_ANDROID)
  return node->type() == bookmarks::BookmarkNode::MOBILE;
#else
  return node->type() != bookmarks::BookmarkNode::MOBILE;
#endif
}

void ChromeBookmarkClient::RecordAction(const base::UserMetricsAction& action) {
  base::RecordAction(action);
}

bookmarks::LoadExtraCallback ChromeBookmarkClient::GetLoadExtraNodesCallback() {
  if (!managed_bookmark_service_)
    return bookmarks::LoadExtraCallback();

  return managed_bookmark_service_->GetLoadExtraNodesCallback();
}

bool ChromeBookmarkClient::CanSetPermanentNodeTitle(
    const bookmarks::BookmarkNode* permanent_node) {
  return !managed_bookmark_service_
             ? true
             : managed_bookmark_service_->CanSetPermanentNodeTitle(
                   permanent_node);
}

bool ChromeBookmarkClient::CanSyncNode(const bookmarks::BookmarkNode* node) {
  return !managed_bookmark_service_
             ? true
             : managed_bookmark_service_->CanSyncNode(node);
}

bool ChromeBookmarkClient::CanBeEditedByUser(
    const bookmarks::BookmarkNode* node) {
  return !managed_bookmark_service_
             ? true
             : managed_bookmark_service_->CanBeEditedByUser(node);
}
