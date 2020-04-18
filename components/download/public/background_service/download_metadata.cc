// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/background_service/download_metadata.h"

namespace download {

CompletionInfo::CompletionInfo(const base::FilePath& path,
                               uint64_t bytes_downloaded)
    : path(path), bytes_downloaded(bytes_downloaded) {}

CompletionInfo::CompletionInfo(const CompletionInfo& other) = default;

CompletionInfo::~CompletionInfo() = default;

bool CompletionInfo::operator==(const CompletionInfo& other) const {
  // The blob data handle is not compared here.
  return path == other.path && bytes_downloaded == other.bytes_downloaded;
}

DownloadMetaData::DownloadMetaData() : current_size(0u) {}

DownloadMetaData::DownloadMetaData(const DownloadMetaData& other) = default;

bool DownloadMetaData::operator==(const DownloadMetaData& other) const {
  return guid == other.guid && current_size == other.current_size &&
         completion_info == other.completion_info;
}

DownloadMetaData::~DownloadMetaData() = default;

}  // namespace download
