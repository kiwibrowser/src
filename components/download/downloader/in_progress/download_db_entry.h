// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_DB_ENTRY_H_
#define COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_DB_ENTRY_H_

#include <string>

#include "base/optional.h"
#include "components/download/downloader/in_progress/download_info.h"

namespace download {

// Representing one entry in the DownloadDB.
struct DownloadDBEntry {
 public:
  DownloadDBEntry();
  DownloadDBEntry(const DownloadDBEntry& other);
  ~DownloadDBEntry();

  bool operator==(const DownloadDBEntry& other) const;

  // ID of the entry, this should be namespace + GUID of the download.
  std::string id;

  // Information about a regular download.
  base::Optional<DownloadInfo> download_info;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_DB_ENTRY_H_
