// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BOOKMARKS_BOOKMARK_CLIENT_IMPL_H_
#define IOS_CHROME_BROWSER_BOOKMARKS_BOOKMARK_CLIENT_IMPL_H_

#include <set>
#include <vector>

#include "base/deferred_sequenced_task_runner.h"
#include "base/macros.h"
#include "components/bookmarks/browser/bookmark_client.h"

class GURL;

namespace bookmarks {
class BookmarkNode;
class BookmarkPermanentNode;
}

namespace ios {
class ChromeBrowserState;
}

class BookmarkClientImpl : public bookmarks::BookmarkClient {
 public:
  BookmarkClientImpl(ios::ChromeBrowserState* browser_state);
  ~BookmarkClientImpl() override;

  // bookmarks::BookmarkClient:
  bool PreferTouchIcon() override;
  base::CancelableTaskTracker::TaskId GetFaviconImageForPageURL(
      const GURL& page_url,
      favicon_base::IconType type,
      const favicon_base::FaviconImageCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  bool SupportsTypedCountForUrls() override;
  void GetTypedCountForUrls(UrlTypedCountMap* url_typed_count_map) override;
  bool IsPermanentNodeVisible(
      const bookmarks::BookmarkPermanentNode* node) override;
  void RecordAction(const base::UserMetricsAction& action) override;
  bookmarks::LoadExtraCallback GetLoadExtraNodesCallback() override;
  bool CanSetPermanentNodeTitle(
      const bookmarks::BookmarkNode* permanent_node) override;
  bool CanSyncNode(const bookmarks::BookmarkNode* node) override;
  bool CanBeEditedByUser(const bookmarks::BookmarkNode* node) override;

 private:
  // Pointer to the associated ios::ChromeBrowserState. Must outlive
  // BookmarkClientImpl.
  ios::ChromeBrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkClientImpl);
};

#endif  // IOS_CHROME_BROWSER_BOOKMARKS_BOOKMARK_CLIENT_IMPL_H_
