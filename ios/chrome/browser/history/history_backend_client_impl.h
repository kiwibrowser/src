// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_HISTORY_HISTORY_BACKEND_CLIENT_IMPL_H_
#define IOS_CHROME_BROWSER_HISTORY_HISTORY_BACKEND_CLIENT_IMPL_H_

#include <vector>

#include "base/macros.h"
#include "components/history/core/browser/history_backend_client.h"

class GURL;

namespace bookmarks {
class BookmarkModel;
}

class HistoryBackendClientImpl : public history::HistoryBackendClient {
 public:
  explicit HistoryBackendClientImpl(bookmarks::BookmarkModel* bookmark_model);
  ~HistoryBackendClientImpl() override;

 private:
  // history::HistoryBackendClient implementation.
  bool IsBookmarked(const GURL& url) override;
  void GetBookmarks(std::vector<history::URLAndTitle>* bookmarks) override;
  bool ShouldReportDatabaseError() override;
  bool IsWebSafe(const GURL& url) override;

  // BookmarkModel instance providing access to bookmarks. May be null during
  // testing but must outlive HistoryBackendClientImpl if non-null.
  bookmarks::BookmarkModel* bookmark_model_;

  DISALLOW_COPY_AND_ASSIGN(HistoryBackendClientImpl);
};

#endif  // IOS_CHROME_BROWSER_HISTORY_HISTORY_BACKEND_CLIENT_IMPL_H_
