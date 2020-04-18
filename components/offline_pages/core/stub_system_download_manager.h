// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_STUB_SYSTEM_DOWNLOAD_MANAGER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_STUB_SYSTEM_DOWNLOAD_MANAGER_H_

#include "components/offline_pages/core/system_download_manager.h"

namespace offline_pages {

// Stub replacement for the DownloadManager to be used by unit tests.
class StubSystemDownloadManager : public SystemDownloadManager {
 public:
  StubSystemDownloadManager(int64_t download_id, bool installed);
  ~StubSystemDownloadManager() override;

  bool IsDownloadManagerInstalled() override;

  int64_t AddCompletedDownload(const std::string& title,
                               const std::string& description,
                               const std::string& path,
                               int64_t length,
                               const std::string& uri,
                               const std::string& referer) override;

  int Remove(const std::vector<int64_t>& android_download_manager_ids) override;

  // Accessors for the test to use to check passed parameters.
  std::string title() { return title_; }
  std::string description() { return description_; }
  std::string path() { return path_; }
  std::string uri() { return uri_; }
  std::string referer() { return referer_; }
  long length() { return length_; }
  void set_installed(bool installed) { installed_ = installed; }
  void set_download_id(int64_t download_id) { download_id_ = download_id; }
  int64_t last_removed_id() { return last_removed_id_; }

 private:
  int64_t download_id_;
  int64_t last_removed_id_;
  std::string title_;
  std::string description_;
  std::string path_;
  std::string uri_;
  std::string referer_;
  long length_;
  bool installed_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_STUB_SYSTEM_DOWNLOAD_MANAGER_H_
