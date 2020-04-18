// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_MOCK_LEVELDB_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_MOCK_LEVELDB_FACTORY_H_

#include "base/files/file_path.h"
#include "content/browser/indexed_db/leveldb/leveldb_factory.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

class MockLevelDBFactory : public LevelDBFactory {
 public:
  MockLevelDBFactory();
  ~MockLevelDBFactory() override;
  MOCK_METHOD4(OpenLevelDB,
               leveldb::Status(const base::FilePath& file_name,
                               const LevelDBComparator* comparator,
                               std::unique_ptr<LevelDBDatabase>* db,
                               bool* is_disk_full));
  MOCK_METHOD1(DestroyLevelDB,
               leveldb::Status(const base::FilePath& file_name));
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_MOCK_LEVELDB_FACTORY_H_
