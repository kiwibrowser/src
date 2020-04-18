// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/remove_stale_cache_files.h"

#include "base/logging.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"

namespace drive {
namespace internal {

void RemoveStaleCacheFiles(FileCache* cache,
                           ResourceMetadata* resource_metadata) {
  std::unique_ptr<ResourceMetadata::Iterator> it =
      resource_metadata->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    const ResourceEntry& entry = it->GetValue();
    const FileCacheEntry& cache_state =
        entry.file_specific_info().cache_state();
    // Stale = not dirty but the MD5 does not match.
    if (!cache_state.is_dirty() &&
        cache_state.md5() != entry.file_specific_info().md5()) {
      FileError error = cache->Remove(it->GetID());
      LOG_IF(WARNING, error != FILE_ERROR_OK)
          << "Failed to remove a stale cache file. resource_id: "
          << it->GetID();
    }
  }
}

}  // namespace internal
}  // namespace drive
