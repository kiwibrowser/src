// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_

#include <stdint.h>

#include <vector>

#include "components/offline_pages/core/system_download_manager.h"

namespace offline_pages {
namespace android {

// Bridge between C++ and Java for communicating with the AndroidDownloadManager
// on Android.
class OfflinePagesDownloadManagerBridge : public SystemDownloadManager {
 public:
  bool IsDownloadManagerInstalled() override;

  int64_t AddCompletedDownload(const std::string& title,
                               const std::string& description,
                               const std::string& path,
                               int64_t length,
                               const std::string& uri,
                               const std::string& referer) override;

  int Remove(const std::vector<int64_t>& android_download_manager_ids) override;
};

}  // namespace android
}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGES_DOWNLOAD_MANAGER_BRIDGE_H_
