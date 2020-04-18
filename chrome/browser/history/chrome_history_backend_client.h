// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_CHROME_HISTORY_BACKEND_CLIENT_H_
#define CHROME_BROWSER_HISTORY_CHROME_HISTORY_BACKEND_CLIENT_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "components/history/core/browser/history_backend_client.h"

namespace bookmarks {
class BookmarkModel;
}

// ChromeHistoryBackendClient implements history::HistoryBackendClient interface
// to provides access to embedder-specific features.
class ChromeHistoryBackendClient : public history::HistoryBackendClient {
 public:
  explicit ChromeHistoryBackendClient(bookmarks::BookmarkModel* bookmark_model);
  ~ChromeHistoryBackendClient() override;

  // history::HistoryBackendClient implementation.
  bool IsBookmarked(const GURL& url) override;
  void GetBookmarks(std::vector<history::URLAndTitle>* bookmarks) override;
  bool ShouldReportDatabaseError() override;
  bool IsWebSafe(const GURL& url) override;
#if defined(OS_ANDROID)
  void OnHistoryBackendInitialized(
      history::HistoryBackend* history_backend,
      history::HistoryDatabase* history_database,
      history::ThumbnailDatabase* thumbnail_database,
      const base::FilePath& history_dir) override;
  void OnHistoryBackendDestroyed(history::HistoryBackend* history_backend,
                                 const base::FilePath& history_dir) override;
#endif  // defined(OS_ANDROID)

 private:
  // BookmarkModel instance providing access to bookmarks. May be null during
  // testing but must outlive ChromeHistoryBackendClient if non-null.
  bookmarks::BookmarkModel* bookmark_model_;

  DISALLOW_COPY_AND_ASSIGN(ChromeHistoryBackendClient);
};

#endif  // CHROME_BROWSER_HISTORY_CHROME_HISTORY_BACKEND_CLIENT_H_
