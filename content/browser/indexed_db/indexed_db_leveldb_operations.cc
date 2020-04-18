// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_leveldb_operations.h"

#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "content/browser/indexed_db/indexed_db_reporting.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"
#include "content/browser/indexed_db/leveldb/leveldb_transaction.h"

using leveldb::Status;
using base::StringPiece;

namespace content {
namespace indexed_db {

leveldb::Status InternalInconsistencyStatus() {
  return leveldb::Status::Corruption("Internal inconsistency");
}

leveldb::Status InvalidDBKeyStatus() {
  return leveldb::Status::InvalidArgument("Invalid database key ID");
}

leveldb::Status IOErrorStatus() {
  return leveldb::Status::IOError("IO Error");
}

namespace {
template <typename DBOrTransaction>
Status GetIntInternal(DBOrTransaction* db,
                      const StringPiece& key,
                      int64_t* found_int,
                      bool* found) {
  std::string result;
  Status s = db->Get(key, &result, found);
  if (!s.ok())
    return s;
  if (!*found)
    return Status::OK();
  StringPiece slice(result);
  if (DecodeInt(&slice, found_int) && slice.empty())
    return s;
  return InternalInconsistencyStatus();
}
}  // namespace

Status GetInt(LevelDBTransaction* txn,
              const StringPiece& key,
              int64_t* found_int,
              bool* found) {
  return GetIntInternal(txn, key, found_int, found);
}

Status GetInt(LevelDBDatabase* db,
              const StringPiece& key,
              int64_t* found_int,
              bool* found) {
  return GetIntInternal(db, key, found_int, found);
}

void PutBool(LevelDBTransaction* transaction,
             const StringPiece& key,
             bool value) {
  std::string buffer;
  EncodeBool(value, &buffer);
  transaction->Put(key, &buffer);
}

void PutInt(LevelDBTransaction* transaction,
            const StringPiece& key,
            int64_t value) {
  DCHECK_GE(value, 0);
  std::string buffer;
  EncodeInt(value, &buffer);
  transaction->Put(key, &buffer);
}

template <typename DBOrTransaction>
Status GetVarInt(DBOrTransaction* db,
                 const StringPiece& key,
                 int64_t* found_int,
                 bool* found) {
  std::string result;
  Status s = db->Get(key, &result, found);
  if (!s.ok())
    return s;
  if (!*found)
    return Status::OK();
  StringPiece slice(result);
  if (DecodeVarInt(&slice, found_int) && slice.empty())
    return s;
  return InternalInconsistencyStatus();
}

template Status GetVarInt<LevelDBTransaction>(LevelDBTransaction* txn,
                                              const StringPiece& key,
                                              int64_t* found_int,
                                              bool* found);
template Status GetVarInt<LevelDBDatabase>(LevelDBDatabase* db,
                                           const StringPiece& key,
                                           int64_t* found_int,
                                           bool* found);

void PutVarInt(LevelDBTransaction* transaction,
               const StringPiece& key,
               int64_t value) {
  std::string buffer;
  EncodeVarInt(value, &buffer);
  transaction->Put(key, &buffer);
}

template <typename DBOrTransaction>
Status GetString(DBOrTransaction* db,
                 const StringPiece& key,
                 base::string16* found_string,
                 bool* found) {
  std::string result;
  *found = false;
  Status s = db->Get(key, &result, found);
  if (!s.ok())
    return s;
  if (!*found)
    return Status::OK();
  StringPiece slice(result);
  if (DecodeString(&slice, found_string) && slice.empty())
    return s;
  return InternalInconsistencyStatus();
}

template Status GetString<LevelDBTransaction>(LevelDBTransaction* txn,
                                              const StringPiece& key,
                                              base::string16* found_string,
                                              bool* found);
template Status GetString<LevelDBDatabase>(LevelDBDatabase* db,
                                           const StringPiece& key,
                                           base::string16* found_string,
                                           bool* found);

void PutString(LevelDBTransaction* transaction,
               const StringPiece& key,
               const base::string16& value) {
  std::string buffer;
  EncodeString(value, &buffer);
  transaction->Put(key, &buffer);
}

void PutIDBKeyPath(LevelDBTransaction* transaction,
                   const StringPiece& key,
                   const IndexedDBKeyPath& value) {
  std::string buffer;
  EncodeIDBKeyPath(value, &buffer);
  transaction->Put(key, &buffer);
}

template <typename DBOrTransaction>
Status GetMaxObjectStoreId(DBOrTransaction* db,
                           int64_t database_id,
                           int64_t* max_object_store_id) {
  const std::string max_object_store_id_key = DatabaseMetaDataKey::Encode(
      database_id, DatabaseMetaDataKey::MAX_OBJECT_STORE_ID);
  *max_object_store_id = -1;
  bool found = false;
  Status s = indexed_db::GetInt(db, max_object_store_id_key,
                                max_object_store_id, &found);
  if (!s.ok())
    return s;
  if (!found)
    *max_object_store_id = 0;

  DCHECK_GE(*max_object_store_id, 0);
  return s;
}

template Status GetMaxObjectStoreId<LevelDBTransaction>(
    LevelDBTransaction* db,
    int64_t database_id,
    int64_t* max_object_store_id);
template Status GetMaxObjectStoreId<LevelDBDatabase>(
    LevelDBDatabase* db,
    int64_t database_id,
    int64_t* max_object_store_id);

Status SetMaxObjectStoreId(LevelDBTransaction* transaction,
                           int64_t database_id,
                           int64_t object_store_id) {
  const std::string max_object_store_id_key = DatabaseMetaDataKey::Encode(
      database_id, DatabaseMetaDataKey::MAX_OBJECT_STORE_ID);
  int64_t max_object_store_id = -1;
  bool found = false;
  Status s = GetInt(transaction, max_object_store_id_key, &max_object_store_id,
                    &found);
  if (!s.ok())
    return s;
  if (!found)
    max_object_store_id = 0;

  DCHECK_GE(max_object_store_id, 0);
  if (!s.ok()) {
    INTERNAL_READ_ERROR_UNTESTED(SET_MAX_OBJECT_STORE_ID);
    return s;
  }

  if (object_store_id <= max_object_store_id) {
    INTERNAL_CONSISTENCY_ERROR_UNTESTED(SET_MAX_OBJECT_STORE_ID);
    return indexed_db::InternalInconsistencyStatus();
  }
  indexed_db::PutInt(transaction, max_object_store_id_key, object_store_id);
  return s;
}

Status GetNewVersionNumber(LevelDBTransaction* transaction,
                           int64_t database_id,
                           int64_t object_store_id,
                           int64_t* new_version_number) {
  const std::string last_version_key = ObjectStoreMetaDataKey::Encode(
      database_id, object_store_id, ObjectStoreMetaDataKey::LAST_VERSION);

  *new_version_number = -1;
  int64_t last_version = -1;
  bool found = false;
  Status s = GetInt(transaction, last_version_key, &last_version, &found);
  if (!s.ok()) {
    INTERNAL_READ_ERROR_UNTESTED(GET_NEW_VERSION_NUMBER);
    return s;
  }
  if (!found)
    last_version = 0;

  DCHECK_GE(last_version, 0);

  int64_t version = last_version + 1;
  PutInt(transaction, last_version_key, version);

  // TODO(jsbell): Think about how we want to handle the overflow scenario.
  DCHECK(version > last_version);

  *new_version_number = version;
  return s;
}

Status SetMaxIndexId(LevelDBTransaction* transaction,
                     int64_t database_id,
                     int64_t object_store_id,
                     int64_t index_id) {
  int64_t max_index_id = -1;
  const std::string max_index_id_key = ObjectStoreMetaDataKey::Encode(
      database_id, object_store_id, ObjectStoreMetaDataKey::MAX_INDEX_ID);
  bool found = false;
  Status s = GetInt(transaction, max_index_id_key, &max_index_id, &found);
  if (!s.ok()) {
    INTERNAL_READ_ERROR_UNTESTED(SET_MAX_INDEX_ID);
    return s;
  }
  if (!found)
    max_index_id = kMinimumIndexId;

  if (index_id <= max_index_id) {
    INTERNAL_CONSISTENCY_ERROR_UNTESTED(SET_MAX_INDEX_ID);
    return InternalInconsistencyStatus();
  }

  PutInt(transaction, max_index_id_key, index_id);
  return s;
}

Status VersionExists(LevelDBTransaction* transaction,
                     int64_t database_id,
                     int64_t object_store_id,
                     int64_t version,
                     const std::string& encoded_primary_key,
                     bool* exists) {
  const std::string key =
      ExistsEntryKey::Encode(database_id, object_store_id, encoded_primary_key);
  std::string data;

  Status s = transaction->Get(key, &data, exists);
  if (!s.ok()) {
    INTERNAL_READ_ERROR_UNTESTED(VERSION_EXISTS);
    return s;
  }
  if (!*exists)
    return s;

  StringPiece slice(data);
  int64_t decoded;
  if (!DecodeInt(&slice, &decoded) || !slice.empty())
    return InternalInconsistencyStatus();
  *exists = (decoded == version);
  return s;
}

Status GetNewDatabaseId(LevelDBTransaction* transaction, int64_t* new_id) {
  *new_id = -1;
  int64_t max_database_id = -1;
  bool found = false;
  Status s = indexed_db::GetInt(transaction, MaxDatabaseIdKey::Encode(),
                                &max_database_id, &found);
  if (!s.ok()) {
    INTERNAL_READ_ERROR_UNTESTED(GET_NEW_DATABASE_ID);
    return s;
  }
  if (!found)
    max_database_id = 0;

  DCHECK_GE(max_database_id, 0);

  int64_t database_id = max_database_id + 1;
  indexed_db::PutInt(transaction, MaxDatabaseIdKey::Encode(), database_id);
  *new_id = database_id;
  return Status::OK();
}

bool CheckObjectStoreAndMetaDataType(const LevelDBIterator* it,
                                     const std::string& stop_key,
                                     int64_t object_store_id,
                                     int64_t meta_data_type) {
  if (!it->IsValid() || CompareKeys(it->Key(), stop_key) >= 0)
    return false;

  StringPiece slice(it->Key());
  ObjectStoreMetaDataKey meta_data_key;
  bool ok =
      ObjectStoreMetaDataKey::Decode(&slice, &meta_data_key) && slice.empty();
  DCHECK(ok);
  if (meta_data_key.ObjectStoreId() != object_store_id)
    return false;
  if (meta_data_key.MetaDataType() != meta_data_type)
    return false;
  return ok;
}

bool CheckIndexAndMetaDataKey(const LevelDBIterator* it,
                              const std::string& stop_key,
                              int64_t index_id,
                              unsigned char meta_data_type) {
  if (!it->IsValid() || CompareKeys(it->Key(), stop_key) >= 0)
    return false;

  StringPiece slice(it->Key());
  IndexMetaDataKey meta_data_key;
  bool ok = IndexMetaDataKey::Decode(&slice, &meta_data_key);
  DCHECK(ok);
  if (meta_data_key.IndexId() != index_id)
    return false;
  if (meta_data_key.meta_data_type() != meta_data_type)
    return false;
  return true;
}

bool FindGreatestKeyLessThanOrEqual(LevelDBTransaction* transaction,
                                    const std::string& target,
                                    std::string* found_key,
                                    Status* s) {
  std::unique_ptr<LevelDBIterator> it = transaction->CreateIterator();
  *s = it->Seek(target);
  if (!s->ok())
    return false;

  if (!it->IsValid()) {
    *s = it->SeekToLast();
    if (!s->ok() || !it->IsValid())
      return false;
  }

  while (CompareIndexKeys(it->Key(), target) > 0) {
    *s = it->Prev();
    if (!s->ok() || !it->IsValid())
      return false;
  }

  do {
    *found_key = it->Key().as_string();

    // There can be several index keys that compare equal. We want the last one.
    *s = it->Next();
  } while (s->ok() && it->IsValid() && !CompareIndexKeys(it->Key(), target));

  return true;
}

bool GetBlobKeyGeneratorCurrentNumber(
    LevelDBTransaction* leveldb_transaction,
    int64_t database_id,
    int64_t* blob_key_generator_current_number) {
  const std::string key_gen_key = DatabaseMetaDataKey::Encode(
      database_id, DatabaseMetaDataKey::BLOB_KEY_GENERATOR_CURRENT_NUMBER);

  // Default to initial number if not found.
  int64_t cur_number = DatabaseMetaDataKey::kBlobKeyGeneratorInitialNumber;
  std::string data;

  bool found = false;
  bool ok = leveldb_transaction->Get(key_gen_key, &data, &found).ok();
  if (!ok) {
    INTERNAL_READ_ERROR_UNTESTED(GET_BLOB_KEY_GENERATOR_CURRENT_NUMBER);
    return false;
  }
  if (found) {
    StringPiece slice(data);
    if (!DecodeVarInt(&slice, &cur_number) || !slice.empty() ||
        !DatabaseMetaDataKey::IsValidBlobKey(cur_number)) {
      INTERNAL_READ_ERROR_UNTESTED(GET_BLOB_KEY_GENERATOR_CURRENT_NUMBER);
      return false;
    }
  }
  *blob_key_generator_current_number = cur_number;
  return true;
}

bool UpdateBlobKeyGeneratorCurrentNumber(
    LevelDBTransaction* leveldb_transaction,
    int64_t database_id,
    int64_t blob_key_generator_current_number) {
#ifndef NDEBUG
  int64_t old_number;
  if (!GetBlobKeyGeneratorCurrentNumber(leveldb_transaction, database_id,
                                        &old_number))
    return false;
  DCHECK_LT(old_number, blob_key_generator_current_number);
#endif
  DCHECK(
      DatabaseMetaDataKey::IsValidBlobKey(blob_key_generator_current_number));
  const std::string key = DatabaseMetaDataKey::Encode(
      database_id, DatabaseMetaDataKey::BLOB_KEY_GENERATOR_CURRENT_NUMBER);

  PutVarInt(leveldb_transaction, key, blob_key_generator_current_number);
  return true;
}

Status GetEarliestSweepTime(LevelDBDatabase* db, base::Time* earliest_sweep) {
  const std::string earliest_sweep_time_key = EarliestSweepKey::Encode();
  *earliest_sweep = base::Time();
  bool found = false;
  int64_t time_micros = 0;
  Status s =
      indexed_db::GetInt(db, earliest_sweep_time_key, &time_micros, &found);
  if (!s.ok())
    return s;
  if (!found)
    time_micros = 0;

  DCHECK_GE(time_micros, 0);
  *earliest_sweep += base::TimeDelta::FromMicroseconds(time_micros);

  return s;
}

void SetEarliestSweepTime(LevelDBTransaction* txn, base::Time earliest_sweep) {
  const std::string earliest_sweep_time_key = EarliestSweepKey::Encode();
  int64_t time_micros = (earliest_sweep - base::Time()).InMicroseconds();
  indexed_db::PutInt(txn, earliest_sweep_time_key, time_micros);
}

}  // namespace indexed_db
}  // namespace content
