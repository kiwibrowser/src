// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_LEVELDB_OPERATIONS_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_LEVELDB_OPERATIONS_H_

#include <string>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

// Contains common operations for LevelDBTransactions and/or LevelDBDatabases.

namespace content {
class LevelDBDatabase;
class LevelDBIterator;
class LevelDBTransaction;

namespace indexed_db {

// Was able to use LevelDB to read the data w/o error, but the data read was not
// in the expected format.
leveldb::Status InternalInconsistencyStatus();

leveldb::Status InvalidDBKeyStatus();

leveldb::Status IOErrorStatus();

leveldb::Status CONTENT_EXPORT GetInt(LevelDBDatabase* db,
                                      const base::StringPiece& key,
                                      int64_t* found_int,
                                      bool* found);
leveldb::Status CONTENT_EXPORT GetInt(LevelDBTransaction* txn,
                                      const base::StringPiece& key,
                                      int64_t* found_int,
                                      bool* found);

void PutBool(LevelDBTransaction* transaction,
             const base::StringPiece& key,
             bool value);
void CONTENT_EXPORT PutInt(LevelDBTransaction* transaction,
                           const base::StringPiece& key,
                           int64_t value);

template <typename DBOrTransaction>
WARN_UNUSED_RESULT leveldb::Status GetVarInt(DBOrTransaction* db,
                                             const base::StringPiece& key,
                                             int64_t* found_int,
                                             bool* found);

void PutVarInt(LevelDBTransaction* transaction,
               const base::StringPiece& key,
               int64_t value);

template <typename DBOrTransaction>
WARN_UNUSED_RESULT leveldb::Status GetString(DBOrTransaction* db,
                                             const base::StringPiece& key,
                                             base::string16* found_string,
                                             bool* found);

void PutString(LevelDBTransaction* transaction,
               const base::StringPiece& key,
               const base::string16& value);

void PutIDBKeyPath(LevelDBTransaction* transaction,
                   const base::StringPiece& key,
                   const IndexedDBKeyPath& value);

template <typename DBOrTransaction>
WARN_UNUSED_RESULT leveldb::Status GetMaxObjectStoreId(
    DBOrTransaction* db,
    int64_t database_id,
    int64_t* max_object_store_id);

WARN_UNUSED_RESULT leveldb::Status SetMaxObjectStoreId(
    LevelDBTransaction* transaction,
    int64_t database_id,
    int64_t object_store_id);

WARN_UNUSED_RESULT leveldb::Status GetNewVersionNumber(
    LevelDBTransaction* transaction,
    int64_t database_id,
    int64_t object_store_id,
    int64_t* new_version_number);

WARN_UNUSED_RESULT leveldb::Status SetMaxIndexId(
    LevelDBTransaction* transaction,
    int64_t database_id,
    int64_t object_store_id,
    int64_t index_id);

WARN_UNUSED_RESULT leveldb::Status VersionExists(
    LevelDBTransaction* transaction,
    int64_t database_id,
    int64_t object_store_id,
    int64_t version,
    const std::string& encoded_primary_key,
    bool* exists);

WARN_UNUSED_RESULT leveldb::Status GetNewDatabaseId(
    LevelDBTransaction* transaction,
    int64_t* new_id);

WARN_UNUSED_RESULT bool CheckObjectStoreAndMetaDataType(
    const LevelDBIterator* it,
    const std::string& stop_key,
    int64_t object_store_id,
    int64_t meta_data_type);

WARN_UNUSED_RESULT bool CheckIndexAndMetaDataKey(const LevelDBIterator* it,
                                                 const std::string& stop_key,
                                                 int64_t index_id,
                                                 unsigned char meta_data_type);

WARN_UNUSED_RESULT bool FindGreatestKeyLessThanOrEqual(
    LevelDBTransaction* transaction,
    const std::string& target,
    std::string* found_key,
    leveldb::Status* s);

WARN_UNUSED_RESULT bool GetBlobKeyGeneratorCurrentNumber(
    LevelDBTransaction* leveldb_transaction,
    int64_t database_id,
    int64_t* blob_key_generator_current_number);

WARN_UNUSED_RESULT bool UpdateBlobKeyGeneratorCurrentNumber(
    LevelDBTransaction* leveldb_transaction,
    int64_t database_id,
    int64_t blob_key_generator_current_number);

WARN_UNUSED_RESULT leveldb::Status GetEarliestSweepTime(
    LevelDBDatabase* db,
    base::Time* earliest_sweep);

void SetEarliestSweepTime(LevelDBTransaction* txn, base::Time earliest_sweep);

}  // namespace indexed_db
}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_LEVELDB_OPERATIONS_H_
