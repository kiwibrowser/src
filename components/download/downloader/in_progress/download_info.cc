// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/download_info.h"

namespace download {

DownloadInfo::DownloadInfo() = default;

DownloadInfo::DownloadInfo(const DownloadInfo& other) = default;

DownloadInfo::~DownloadInfo() = default;

bool DownloadInfo::operator==(const DownloadInfo& other) const {
  return guid == other.guid && ukm_info == other.ukm_info &&
         in_progress_info == other.in_progress_info;
}

}  // namespace download
