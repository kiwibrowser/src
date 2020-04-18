// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_FACTORY_H_

#include <memory>

#include "content/common/content_export.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace base {
class FilePath;
}

namespace content {

class LevelDBComparator;
class LevelDBDatabase;

class CONTENT_EXPORT LevelDBFactory {
 public:
  virtual ~LevelDBFactory() {}
  virtual leveldb::Status OpenLevelDB(const base::FilePath& file_name,
                                      const LevelDBComparator* comparator,
                                      std::unique_ptr<LevelDBDatabase>* db,
                                      bool* is_disk_full) = 0;
  virtual leveldb::Status DestroyLevelDB(const base::FilePath& file_name) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_FACTORY_H_
