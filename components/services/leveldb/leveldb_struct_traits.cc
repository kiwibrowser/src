// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/leveldb/leveldb_struct_traits.h"

#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"

namespace mojo {

bool StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    create_if_missing(const leveldb_env::Options& options) {
  return options.create_if_missing;
}

bool StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    error_if_exists(const leveldb_env::Options& options) {
  return options.error_if_exists;
}

bool StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    paranoid_checks(const leveldb_env::Options& options) {
  return options.paranoid_checks;
}

uint64_t
StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    write_buffer_size(const leveldb_env::Options& options) {
  return options.write_buffer_size;
}

int32_t StructTraits<
    leveldb::mojom::OpenOptionsDataView,
    leveldb_env::Options>::max_open_files(const leveldb_env::Options& options) {
  return options.max_open_files;
}

leveldb::mojom::SharedReadCache
StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    shared_block_read_cache(const leveldb_env::Options& options) {
  // The Mojo wrapper for leveldb only supports using one of two different
  // shared caches. Chrome's Mojo wrapper does not currently support custom
  // caches, nor NULL to have leveldb create the block read cache.
  if (!options.block_cache) {
    // Specify either Default or Web.
    NOTREACHED();
    return leveldb::mojom::SharedReadCache::Default;
  }
  if (options.block_cache == leveldb_chrome::GetSharedWebBlockCache())
    return leveldb::mojom::SharedReadCache::Web;

  leveldb_env::Options default_options;
  // If failing see comment above.
  DCHECK_EQ(default_options.block_cache, options.block_cache);

  return leveldb::mojom::SharedReadCache::Default;
}

bool StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options>::
    Read(leveldb::mojom::OpenOptionsDataView data, leveldb_env::Options* out) {
  out->create_if_missing = data.create_if_missing();
  out->error_if_exists = data.error_if_exists();
  out->paranoid_checks = data.paranoid_checks();
  out->write_buffer_size = data.write_buffer_size();
  out->max_open_files = data.max_open_files();
  switch (data.shared_block_read_cache()) {
    case leveldb::mojom::SharedReadCache::Default: {
      leveldb_env::Options options;
      out->block_cache = options.block_cache;
    } break;
    case leveldb::mojom::SharedReadCache::Web:
      out->block_cache = leveldb_chrome::GetSharedWebBlockCache();
      break;
  }

  return true;
}

}  // namespace mojo
