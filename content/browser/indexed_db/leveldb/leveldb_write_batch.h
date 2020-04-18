// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_WRITE_BATCH_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_WRITE_BATCH_H_

#include <memory>

#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

namespace leveldb {
class WriteBatch;
}

namespace content {

// Wrapper around leveldb::WriteBatch.
// This class holds a collection of updates to apply atomically to a database.
class CONTENT_EXPORT LevelDBWriteBatch {
 public:
  static std::unique_ptr<LevelDBWriteBatch> Create();
  ~LevelDBWriteBatch();

  void Put(const base::StringPiece& key, const base::StringPiece& value);
  void Remove(const base::StringPiece& key);  // Add remove operation to the
                                              // batch.
  void Clear();

 private:
  friend class LevelDBDatabase;
  LevelDBWriteBatch();

  std::unique_ptr<leveldb::WriteBatch> write_batch_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_WRITE_BATCH_H_
