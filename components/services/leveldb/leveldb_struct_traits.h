// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_LEVELDB_LEVELDB_STRUCT_TRAITS_H_
#define COMPONENTS_SERVICES_LEVELDB_LEVELDB_STRUCT_TRAITS_H_

#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"

namespace mojo {

template <>
struct StructTraits<leveldb::mojom::OpenOptionsDataView, leveldb_env::Options> {
  static bool create_if_missing(const leveldb_env::Options& options);
  static bool error_if_exists(const leveldb_env::Options& options);
  static bool paranoid_checks(const leveldb_env::Options& options);
  static uint64_t write_buffer_size(const leveldb_env::Options& options);
  static int32_t max_open_files(const leveldb_env::Options& options);
  static ::leveldb::mojom::SharedReadCache shared_block_read_cache(
      const leveldb_env::Options& options);
  static bool Read(::leveldb::mojom::OpenOptionsDataView data,
                   leveldb_env::Options* out);
};

}  // namespace mojo

#endif  // COMPONENTS_SERVICES_LEVELDB_LEVELDB_STRUCT_TRAITS_H_
