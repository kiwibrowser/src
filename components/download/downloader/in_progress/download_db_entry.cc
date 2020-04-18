// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/download_db_entry.h"

namespace download {

DownloadDBEntry::DownloadDBEntry() = default;

DownloadDBEntry::DownloadDBEntry(const DownloadDBEntry& other) = default;

DownloadDBEntry::~DownloadDBEntry() = default;

bool DownloadDBEntry::operator==(const DownloadDBEntry& other) const {
  return id == other.id && download_info == other.download_info;
}

}  // namespace download
