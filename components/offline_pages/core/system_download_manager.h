// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_H_

#include <string>
#include <vector>

namespace offline_pages {

// Interface of a class responsible for interacting with the Android download
// manager
class SystemDownloadManager {
 public:
  SystemDownloadManager() = default;
  virtual ~SystemDownloadManager() = default;

  // Returns true if a system download manager is available on this platform.
  virtual bool IsDownloadManagerInstalled() = 0;

  // Returns the download manager ID of the download, which we will place in the
  // offline pages database as part of the offline page item.
  // TODO(petewil): it might make sense to move all these params into a struct.
  virtual int64_t AddCompletedDownload(const std::string& title,
                                       const std::string& description,
                                       const std::string& path,
                                       int64_t length,
                                       const std::string& uri,
                                       const std::string& referer) = 0;

  // Returns the number of pages removed.
  virtual int Remove(
      const std::vector<int64_t>& android_download_manager_ids) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_SYSTEM_DOWNLOAD_MANAGER_H_
