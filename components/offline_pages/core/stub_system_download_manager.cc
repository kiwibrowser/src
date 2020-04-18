// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/stub_system_download_manager.h"

namespace offline_pages {

StubSystemDownloadManager::StubSystemDownloadManager(int64_t id_to_use,
                                                     bool installed)
    : download_id_(id_to_use), last_removed_id_(0), installed_(installed) {}

StubSystemDownloadManager::~StubSystemDownloadManager() {}

bool StubSystemDownloadManager::IsDownloadManagerInstalled() {
  return installed_;
}

int64_t StubSystemDownloadManager::AddCompletedDownload(
    const std::string& title,
    const std::string& description,
    const std::string& path,
    int64_t length,
    const std::string& uri,
    const std::string& referer) {
  title_ = title;
  description_ = description;
  path_ = path;
  length_ = length;
  uri_ = uri;
  referer_ = referer;

  return download_id_;
}

int StubSystemDownloadManager::Remove(
    const std::vector<int64_t>& android_download_manager_ids) {
  int count = static_cast<int>(android_download_manager_ids.size());
  if (count > 0)
    last_removed_id_ = android_download_manager_ids[count - 1];
  return count;
}

}  // namespace offline_pages
