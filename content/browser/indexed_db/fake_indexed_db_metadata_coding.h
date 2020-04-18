// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_CODING_H_
#define CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_CODING_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/browser/indexed_db/indexed_db_metadata_coding.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace content {
class LevelDBDatabase;
class LevelDBTransaction;
struct IndexedDBDatabaseMetadata;
struct IndexedDBObjectStoreMetadata;
struct IndexedDBIndexMetadata;

// A fake implementation of IndexedDBMetadataCoding, for testing.
class FakeIndexedDBMetadataCoding : public IndexedDBMetadataCoding {
 public:
  FakeIndexedDBMetadataCoding();
  ~FakeIndexedDBMetadataCoding() override;

  leveldb::Status ReadDatabaseNames(
      LevelDBDatabase* db,
      const std::string& origin_identifier,
      std::vector<base::string16>* names) override;

  leveldb::Status ReadMetadataForDatabaseName(
      LevelDBDatabase* db,
      const std::string& origin_identifier,
      const base::string16& name,
      IndexedDBDatabaseMetadata* metadata,
      bool* found) override;
  leveldb::Status CreateDatabase(LevelDBDatabase* database,
                                 const std::string& origin_identifier,
                                 const base::string16& name,
                                 int64_t version,
                                 IndexedDBDatabaseMetadata* metadata) override;

  void SetDatabaseVersion(LevelDBTransaction* transaction,
                          int64_t row_id,
                          int64_t version,
                          IndexedDBDatabaseMetadata* metadata) override;

  leveldb::Status FindDatabaseId(LevelDBDatabase* db,
                                 const std::string& origin_identifier,
                                 const base::string16& name,
                                 int64_t* id,
                                 bool* found) override;

  leveldb::Status CreateObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      base::string16 name,
      IndexedDBKeyPath key_path,
      bool auto_increment,
      IndexedDBObjectStoreMetadata* metadata) override;

  leveldb::Status RenameObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      base::string16 new_name,
      base::string16* old_name,
      IndexedDBObjectStoreMetadata* metadata) override;

  leveldb::Status DeleteObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      const IndexedDBObjectStoreMetadata& object_store) override;

  leveldb::Status CreateIndex(LevelDBTransaction* transaction,
                              int64_t database_id,
                              int64_t object_store_id,
                              int64_t index_id,
                              base::string16 name,
                              IndexedDBKeyPath key_path,
                              bool is_unique,
                              bool is_multi_entry,
                              IndexedDBIndexMetadata* metadata) override;

  leveldb::Status RenameIndex(LevelDBTransaction* transaction,
                              int64_t database_id,
                              int64_t object_store_id,
                              base::string16 new_name,
                              base::string16* old_name,
                              IndexedDBIndexMetadata* metadata) override;

  leveldb::Status DeleteIndex(LevelDBTransaction* transaction,
                              int64_t database_id,
                              int64_t object_store_id,
                              const IndexedDBIndexMetadata& metadata) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeIndexedDBMetadataCoding);
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_CODING_H_
