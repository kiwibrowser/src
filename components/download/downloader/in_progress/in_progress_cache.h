// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_H_

#include <string>

#include "base/optional.h"
#include "components/download/downloader/in_progress/download_entry.h"

namespace download {

extern const base::FilePath::CharType kDownloadMetadataStoreFilename[];

// InProgressCache provides a write-through cache that persists
// information related to an in-progress download such as request origin, retry
// count, resumption parameters etc to the disk. The entries are written to disk
// right away for now (might not be in the case in the long run).
class InProgressCache {
 public:
  virtual ~InProgressCache() = default;

  // Initializes the cache.
  virtual void Initialize(base::OnceClosure callback) = 0;

  // Adds or updates an existing entry.
  virtual void AddOrReplaceEntry(const DownloadEntry& entry) = 0;

  // Retrieves an existing entry.
  virtual base::Optional<DownloadEntry> RetrieveEntry(
      const std::string& guid) = 0;

  // Removes an entry.
  virtual void RemoveEntry(const std::string& guid) = 0;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_H_
