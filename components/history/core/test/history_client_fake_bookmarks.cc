// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/test/history_client_fake_bookmarks.h"

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"
#include "components/history/core/browser/history_backend_client.h"
#include "url/gurl.h"

namespace history {

class FakeBookmarkDatabase
    : public base::RefCountedThreadSafe<FakeBookmarkDatabase> {
 public:
  FakeBookmarkDatabase() {}

  void ClearAllBookmarks();
  void AddBookmarkWithTitle(const GURL& url, const base::string16& title);
  void DelBookmark(const GURL& url);

  bool IsBookmarked(const GURL& url);
  void GetBookmarks(std::vector<URLAndTitle>* bookmarks);

 private:
  friend class base::RefCountedThreadSafe<FakeBookmarkDatabase>;

  ~FakeBookmarkDatabase() {}

  base::Lock lock_;
  std::map<GURL, base::string16> bookmarks_;

  DISALLOW_COPY_AND_ASSIGN(FakeBookmarkDatabase);
};

void FakeBookmarkDatabase::ClearAllBookmarks() {
  base::AutoLock with_lock(lock_);
  bookmarks_.clear();
}

void FakeBookmarkDatabase::AddBookmarkWithTitle(const GURL& url,
                                                const base::string16& title) {
  base::AutoLock with_lock(lock_);
  bookmarks_.insert(std::make_pair(url, title));
}

void FakeBookmarkDatabase::DelBookmark(const GURL& url) {
  base::AutoLock with_lock(lock_);
  auto iter = bookmarks_.find(url);
  if (iter != bookmarks_.end())
    bookmarks_.erase(iter);
}

bool FakeBookmarkDatabase::IsBookmarked(const GURL& url) {
  base::AutoLock with_lock(lock_);
  return bookmarks_.find(url) != bookmarks_.end();
}

void FakeBookmarkDatabase::GetBookmarks(std::vector<URLAndTitle>* bookmarks) {
  base::AutoLock with_lock(lock_);
  bookmarks->reserve(bookmarks->size() + bookmarks_.size());
  for (const auto& pair : bookmarks_) {
    URLAndTitle url_and_title = { pair.first, pair.second };
    bookmarks->push_back(url_and_title);
  }
}

namespace {

class HistoryBackendClientFakeBookmarks : public HistoryBackendClient {
 public:
  explicit HistoryBackendClientFakeBookmarks(
      const scoped_refptr<FakeBookmarkDatabase>& bookmarks);
  ~HistoryBackendClientFakeBookmarks() override;

  // HistoryBackendClient implementation.
  bool IsBookmarked(const GURL& url) override;
  void GetBookmarks(std::vector<URLAndTitle>* bookmarks) override;
  bool ShouldReportDatabaseError() override;
  bool IsWebSafe(const GURL& url) override;
#if defined(OS_ANDROID)
  void OnHistoryBackendInitialized(HistoryBackend* history_backend,
                                   HistoryDatabase* history_database,
                                   ThumbnailDatabase* thumbnail_database,
                                   const base::FilePath& history_dir) override;
  void OnHistoryBackendDestroyed(HistoryBackend* history_backend,
                                 const base::FilePath& history_dir) override;
#endif  // defined(OS_ANDROID)

 private:
  scoped_refptr<FakeBookmarkDatabase> bookmarks_;

  DISALLOW_COPY_AND_ASSIGN(HistoryBackendClientFakeBookmarks);
};

HistoryBackendClientFakeBookmarks::HistoryBackendClientFakeBookmarks(
    const scoped_refptr<FakeBookmarkDatabase>& bookmarks)
    : bookmarks_(bookmarks) {
}

HistoryBackendClientFakeBookmarks::~HistoryBackendClientFakeBookmarks() {
}

bool HistoryBackendClientFakeBookmarks::IsBookmarked(const GURL& url) {
  return bookmarks_->IsBookmarked(url);
}

void HistoryBackendClientFakeBookmarks::GetBookmarks(
    std::vector<URLAndTitle>* bookmarks) {
  bookmarks_->GetBookmarks(bookmarks);
}

bool HistoryBackendClientFakeBookmarks::ShouldReportDatabaseError() {
  return false;
}

bool HistoryBackendClientFakeBookmarks::IsWebSafe(const GURL& url) {
  return true;
}

#if defined(OS_ANDROID)
void HistoryBackendClientFakeBookmarks::OnHistoryBackendInitialized(
    HistoryBackend* history_backend,
    HistoryDatabase* history_database,
    ThumbnailDatabase* thumbnail_database,
    const base::FilePath& history_dir) {
}

void HistoryBackendClientFakeBookmarks::OnHistoryBackendDestroyed(
    HistoryBackend* history_backend,
    const base::FilePath& history_dir) {
}
#endif  // defined(OS_ANDROID)

}  // namespace

HistoryClientFakeBookmarks::HistoryClientFakeBookmarks() {
  bookmarks_ = new FakeBookmarkDatabase;
}

HistoryClientFakeBookmarks::~HistoryClientFakeBookmarks() {
}

void HistoryClientFakeBookmarks::ClearAllBookmarks() {
  bookmarks_->ClearAllBookmarks();
}

void HistoryClientFakeBookmarks::AddBookmark(const GURL& url) {
  bookmarks_->AddBookmarkWithTitle(url, base::string16());
}

void HistoryClientFakeBookmarks::AddBookmarkWithTitle(
    const GURL& url,
    const base::string16& title) {
  bookmarks_->AddBookmarkWithTitle(url, title);
}

void HistoryClientFakeBookmarks::DelBookmark(const GURL& url) {
  bookmarks_->DelBookmark(url);
}

bool HistoryClientFakeBookmarks::IsBookmarked(const GURL& url) {
  return bookmarks_->IsBookmarked(url);
}

void HistoryClientFakeBookmarks::OnHistoryServiceCreated(
    HistoryService* history_service) {
}

void HistoryClientFakeBookmarks::Shutdown() {
}

bool HistoryClientFakeBookmarks::CanAddURL(const GURL& url) {
  return url.is_valid();
}

void HistoryClientFakeBookmarks::NotifyProfileError(
    sql::InitStatus init_status,
    const std::string& diagnostics) {}

std::unique_ptr<HistoryBackendClient>
HistoryClientFakeBookmarks::CreateBackendClient() {
  return std::make_unique<HistoryBackendClientFakeBookmarks>(bookmarks_);
}

}  // namespace history
