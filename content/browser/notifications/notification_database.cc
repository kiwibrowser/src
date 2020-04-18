// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/notifications/notification_database.h"

#include <string>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "content/browser/notifications/notification_database_data_conversions.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_database_data.h"
#include "storage/common/database/database_identifier.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/filter_policy.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"
#include "url/gurl.h"

// Notification LevelDB database schema (in alphabetized order)
// =======================
//
// key: "DATA:" <origin identifier> '\x00' <notification_id>
// value: String containing the NotificationDatabaseDataProto protocol buffer
//        in serialized form.
//
// key: "NEXT_NOTIFICATION_ID"
// value: Decimal string which fits into an int64_t, containing the next
//        persistent notification ID.

namespace content {
namespace {

// Keys of the fields defined in the database.
const char kNextNotificationIdKey[] = "NEXT_NOTIFICATION_ID";
const char kDataKeyPrefix[] = "DATA:";

// Separates the components of compound keys.
const char kNotificationKeySeparator = '\x00';

// The first notification id which to be handed out by the database.
const int64_t kFirstPersistentNotificationId = 1;

// Converts the LevelDB |status| to one of the notification database's values.
NotificationDatabase::Status LevelDBStatusToNotificationDatabaseStatus(
    const leveldb::Status& status) {
  if (status.ok())
    return NotificationDatabase::STATUS_OK;
  else if (status.IsNotFound())
    return NotificationDatabase::STATUS_ERROR_NOT_FOUND;
  else if (status.IsCorruption())
    return NotificationDatabase::STATUS_ERROR_CORRUPTED;
  else if (status.IsIOError())
    return NotificationDatabase::STATUS_IO_ERROR;
  else if (status.IsNotSupportedError())
    return NotificationDatabase::STATUS_NOT_SUPPORTED;
  else if (status.IsInvalidArgument())
    return NotificationDatabase::STATUS_INVALID_ARGUMENT;

  return NotificationDatabase::STATUS_ERROR_FAILED;
}

// Creates a prefix for the data entries based on |origin|.
std::string CreateDataPrefix(const GURL& origin) {
  if (!origin.is_valid())
    return kDataKeyPrefix;

  return base::StringPrintf("%s%s%c", kDataKeyPrefix,
                            storage::GetIdentifierFromOrigin(origin).c_str(),
                            kNotificationKeySeparator);
}

// Creates the compound data key in which notification data is stored.
std::string CreateDataKey(const GURL& origin,
                          const std::string& notification_id) {
  DCHECK(origin.is_valid());
  DCHECK(!notification_id.empty());

  return CreateDataPrefix(origin) + notification_id;
}

// Deserializes data in |serialized_data| to |notification_database_data|.
// Will return if the deserialization was successful.
NotificationDatabase::Status DeserializedNotificationData(
    const std::string& serialized_data,
    NotificationDatabaseData* notification_database_data) {
  DCHECK(notification_database_data);
  if (DeserializeNotificationDatabaseData(serialized_data,
                                          notification_database_data)) {
    return NotificationDatabase::STATUS_OK;
  }

  DLOG(ERROR) << "Unable to deserialize a notification's data.";
  return NotificationDatabase::STATUS_ERROR_CORRUPTED;
}

}  // namespace

NotificationDatabase::NotificationDatabase(const base::FilePath& path)
    : path_(path) {}

NotificationDatabase::~NotificationDatabase() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
}

NotificationDatabase::Status NotificationDatabase::Open(
    bool create_if_missing) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK_EQ(STATE_UNINITIALIZED, state_);

  if (!create_if_missing) {
    if (IsInMemoryDatabase() || !base::PathExists(path_) ||
        base::IsDirectoryEmpty(path_)) {
      return NotificationDatabase::STATUS_ERROR_NOT_FOUND;
    }
  }

  filter_policy_.reset(leveldb::NewBloomFilterPolicy(10));

  leveldb_env::Options options;
  options.create_if_missing = create_if_missing;
  options.paranoid_checks = true;
  options.filter_policy = filter_policy_.get();
  options.block_cache = leveldb_chrome::GetSharedWebBlockCache();
  if (IsInMemoryDatabase()) {
    env_ = leveldb_chrome::NewMemEnv("notification");
    options.env = env_.get();
  }

  Status status = LevelDBStatusToNotificationDatabaseStatus(
      leveldb_env::OpenDB(options, path_.AsUTF8Unsafe(), &db_));
  if (status != STATUS_OK)
    return status;

  state_ = STATE_INITIALIZED;

  return ReadNextPersistentNotificationId();
}

int64_t NotificationDatabase::GetNextPersistentNotificationId() {
  return next_persistent_notification_id_++;
}

NotificationDatabase::Status NotificationDatabase::ReadNotificationData(
    const std::string& notification_id,
    const GURL& origin,
    NotificationDatabaseData* notification_database_data) const {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK_EQ(STATE_INITIALIZED, state_);
  DCHECK(!notification_id.empty());
  DCHECK(origin.is_valid());
  DCHECK(notification_database_data);

  std::string key = CreateDataKey(origin, notification_id);
  std::string serialized_data;

  Status status = LevelDBStatusToNotificationDatabaseStatus(
      db_->Get(leveldb::ReadOptions(), key, &serialized_data));
  if (status != STATUS_OK)
    return status;

  return DeserializedNotificationData(serialized_data,
                                      notification_database_data);
}

NotificationDatabase::Status NotificationDatabase::ReadAllNotificationData(
    std::vector<NotificationDatabaseData>* notification_data_vector) const {
  return ReadAllNotificationDataInternal(
      GURL() /* origin */, blink::mojom::kInvalidServiceWorkerRegistrationId,
      notification_data_vector);
}

NotificationDatabase::Status
NotificationDatabase::ReadAllNotificationDataForOrigin(
    const GURL& origin,
    std::vector<NotificationDatabaseData>* notification_data_vector) const {
  return ReadAllNotificationDataInternal(
      origin, blink::mojom::kInvalidServiceWorkerRegistrationId,
      notification_data_vector);
}

NotificationDatabase::Status
NotificationDatabase::ReadAllNotificationDataForServiceWorkerRegistration(
    const GURL& origin,
    int64_t service_worker_registration_id,
    std::vector<NotificationDatabaseData>* notification_data_vector) const {
  return ReadAllNotificationDataInternal(origin, service_worker_registration_id,
                                         notification_data_vector);
}

NotificationDatabase::Status NotificationDatabase::WriteNotificationData(
    const GURL& origin,
    const NotificationDatabaseData& notification_data) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK_EQ(STATE_INITIALIZED, state_);
  DCHECK(origin.is_valid());

  const std::string& notification_id = notification_data.notification_id;
  DCHECK(!notification_id.empty());

  std::string serialized_data;
  if (!SerializeNotificationDatabaseData(notification_data, &serialized_data)) {
    DLOG(ERROR) << "Unable to serialize data for a notification belonging "
                << "to: " << origin;
    return STATUS_ERROR_FAILED;
  }

  leveldb::WriteBatch batch;
  batch.Put(CreateDataKey(origin, notification_id), serialized_data);

  if (written_persistent_notification_id_ != next_persistent_notification_id_) {
    written_persistent_notification_id_ = next_persistent_notification_id_;
    batch.Put(kNextNotificationIdKey,
              base::Int64ToString(next_persistent_notification_id_));
  }

  return LevelDBStatusToNotificationDatabaseStatus(
      db_->Write(leveldb::WriteOptions(), &batch));
}

NotificationDatabase::Status NotificationDatabase::DeleteNotificationData(
    const std::string& notification_id,
    const GURL& origin) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK_EQ(STATE_INITIALIZED, state_);
  DCHECK(!notification_id.empty());
  DCHECK(origin.is_valid());

  std::string key = CreateDataKey(origin, notification_id);
  return LevelDBStatusToNotificationDatabaseStatus(
      db_->Delete(leveldb::WriteOptions(), key));
}

NotificationDatabase::Status
NotificationDatabase::DeleteAllNotificationDataForOrigin(
    const GURL& origin,
    const std::string& tag,
    std::set<std::string>* deleted_notification_ids) {
  return DeleteAllNotificationDataInternal(
      origin, tag, blink::mojom::kInvalidServiceWorkerRegistrationId,
      deleted_notification_ids);
}

NotificationDatabase::Status
NotificationDatabase::DeleteAllNotificationDataForServiceWorkerRegistration(
    const GURL& origin,
    int64_t service_worker_registration_id,
    std::set<std::string>* deleted_notification_ids) {
  return DeleteAllNotificationDataInternal(origin, "" /* tag */,
                                           service_worker_registration_id,
                                           deleted_notification_ids);
}

NotificationDatabase::Status NotificationDatabase::Destroy() {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  leveldb_env::Options options;
  if (IsInMemoryDatabase()) {
    if (!env_)
      return STATUS_OK;  // The database has not been initialized.

    options.env = env_.get();
  }

  state_ = STATE_DISABLED;
  db_.reset();

  return LevelDBStatusToNotificationDatabaseStatus(
      leveldb::DestroyDB(path_.AsUTF8Unsafe(), options));
}

NotificationDatabase::Status
NotificationDatabase::ReadNextPersistentNotificationId() {
  std::string value;
  Status status = LevelDBStatusToNotificationDatabaseStatus(
      db_->Get(leveldb::ReadOptions(), kNextNotificationIdKey, &value));

  if (status == STATUS_ERROR_NOT_FOUND) {
    next_persistent_notification_id_ = kFirstPersistentNotificationId;
    written_persistent_notification_id_ = kFirstPersistentNotificationId;
    return STATUS_OK;
  }

  if (status != STATUS_OK)
    return status;

  if (!base::StringToInt64(value, &next_persistent_notification_id_) ||
      next_persistent_notification_id_ < kFirstPersistentNotificationId) {
    return STATUS_ERROR_CORRUPTED;
  }

  written_persistent_notification_id_ = next_persistent_notification_id_;

  return STATUS_OK;
}

NotificationDatabase::Status
NotificationDatabase::ReadAllNotificationDataInternal(
    const GURL& origin,
    int64_t service_worker_registration_id,
    std::vector<NotificationDatabaseData>* notification_data_vector) const {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(notification_data_vector);

  const std::string prefix = CreateDataPrefix(origin);

  leveldb::Slice prefix_slice(prefix);

  NotificationDatabaseData notification_database_data;
  std::unique_ptr<leveldb::Iterator> iter(
      db_->NewIterator(leveldb::ReadOptions()));
  for (iter->Seek(prefix_slice); iter->Valid(); iter->Next()) {
    if (!iter->key().starts_with(prefix_slice))
      break;

    Status status = DeserializedNotificationData(iter->value().ToString(),
                                                 &notification_database_data);
    if (status != STATUS_OK)
      return status;

    if (service_worker_registration_id !=
            blink::mojom::kInvalidServiceWorkerRegistrationId &&
        notification_database_data.service_worker_registration_id !=
            service_worker_registration_id) {
      continue;
    }

    notification_data_vector->push_back(notification_database_data);
  }

  return LevelDBStatusToNotificationDatabaseStatus(iter->status());
}

NotificationDatabase::Status
NotificationDatabase::DeleteAllNotificationDataInternal(
    const GURL& origin,
    const std::string& tag,
    int64_t service_worker_registration_id,
    std::set<std::string>* deleted_notification_ids) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(deleted_notification_ids);
  DCHECK(origin.is_valid());

  const std::string prefix = CreateDataPrefix(origin);

  leveldb::Slice prefix_slice(prefix);
  leveldb::WriteBatch batch;

  NotificationDatabaseData notification_database_data;
  std::unique_ptr<leveldb::Iterator> iter(
      db_->NewIterator(leveldb::ReadOptions()));
  for (iter->Seek(prefix_slice); iter->Valid(); iter->Next()) {
    if (!iter->key().starts_with(prefix_slice))
      break;

    Status status = DeserializedNotificationData(iter->value().ToString(),
                                                 &notification_database_data);
    if (status != STATUS_OK)
      return status;

    if (!tag.empty() &&
        notification_database_data.notification_data.tag != tag) {
      continue;
    }

    if (service_worker_registration_id !=
            blink::mojom::kInvalidServiceWorkerRegistrationId &&
        notification_database_data.service_worker_registration_id !=
            service_worker_registration_id) {
      continue;
    }

    batch.Delete(iter->key());

    // Silently remove the notification if it doesn't have an ID assigned.
    // TODO(peter): Remove this clause when Chrome 55 has branched.
    if (notification_database_data.notification_id.empty())
      continue;

    deleted_notification_ids->insert(
        notification_database_data.notification_id);
  }

  if (deleted_notification_ids->empty())
    return STATUS_OK;

  return LevelDBStatusToNotificationDatabaseStatus(
      db_->Write(leveldb::WriteOptions(), &batch));
}

}  // namespace content
