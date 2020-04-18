// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/leveldb_proto/proto_database.h"

namespace leveldb_proto {

leveldb_env::Options CreateSimpleOptions() {
  leveldb_env::Options options;
  options.create_if_missing = true;
  options.max_open_files = 0;  // Use minimum.
  return options;
}

}  // namespace leveldb_proto
