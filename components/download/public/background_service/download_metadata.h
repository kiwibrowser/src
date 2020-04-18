// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_METADATA_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_METADATA_H_

#include "base/files/file_path.h"
#include "base/optional.h"
#include "storage/browser/blob/blob_data_handle.h"

namespace download {

// Struct that contains information about successfully completed downloads.
struct CompletionInfo {
  // The file path for the download file. In incognito mode, use |blob_handle_|
  // to retrieve data.
  base::FilePath path;

  // The blob data handle that contains download data.
  // Will be available after the download is completed in incognito mode.
  base::Optional<storage::BlobDataHandle> blob_handle;

  // Download file size in bytes.
  uint64_t bytes_downloaded;

  CompletionInfo(const base::FilePath& path, uint64_t bytes_downloaded);
  CompletionInfo(const CompletionInfo& other);
  ~CompletionInfo();
  bool operator==(const CompletionInfo& other) const;
};

// Struct to describe general download status.
struct DownloadMetaData {
  // The GUID of the download.
  std::string guid;

  // Data that has been fetched. Can be used to get the current size of
  // uncompleted download.
  uint64_t current_size;

  // Info about successfully completed download, or null for in-progress
  // download. Failed download will not be persisted and exposed as meta data.
  base::Optional<CompletionInfo> completion_info;

  DownloadMetaData();
  ~DownloadMetaData();
  DownloadMetaData(const DownloadMetaData& other);
  bool operator==(const DownloadMetaData& other) const;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_DOWNLOAD_METADATA_H_
