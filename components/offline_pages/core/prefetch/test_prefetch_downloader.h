// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_

#include <map>
#include <string>

#include "components/offline_pages/core/prefetch/prefetch_downloader.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

// Mock implementation of prefetch downloader that is suitable for testing.
class TestPrefetchDownloader : public PrefetchDownloader {
 public:
  TestPrefetchDownloader();
  ~TestPrefetchDownloader() override;

  void SetPrefetchService(PrefetchService* service) override;
  bool IsDownloadServiceUnavailable() const override;
  void CleanupDownloadsWhenReady() override;
  void StartDownload(const std::string& download_id,
                     const std::string& download_location) override;
  void OnDownloadServiceReady(
      const std::set<std::string>& outstanding_download_ids,
      const std::map<std::string, std::pair<base::FilePath, int64_t>>&
          success_downloads) override;
  void OnDownloadServiceUnavailable() override;
  void OnDownloadSucceeded(const std::string& download_id,
                           const base::FilePath& file_path,
                           int64_t file_size) override;
  void OnDownloadFailed(const std::string& download_id) override;

  void Reset();

  const std::map<std::string, std::string>& requested_downloads() const {
    return requested_downloads_;
  }

 private:
  std::map<std::string, std::string> requested_downloads_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_
