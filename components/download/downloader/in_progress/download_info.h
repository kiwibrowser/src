// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_INFO_H_
#define COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_INFO_H_

#include <string>

#include "base/optional.h"
#include "components/download/downloader/in_progress/in_progress_info.h"
#include "components/download/downloader/in_progress/ukm_info.h"

namespace download {

// Contains needed information to reconstruct a download item.
struct DownloadInfo {
 public:
  DownloadInfo();
  DownloadInfo(const DownloadInfo& other);
  ~DownloadInfo();

  bool operator==(const DownloadInfo& other) const;

  // Download GUID.
  std::string guid;

  // UKM information for reporting.
  base::Optional<UkmInfo> ukm_info;

  // In progress information for active download.
  base::Optional<InProgressInfo> in_progress_info;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_INFO_H_
