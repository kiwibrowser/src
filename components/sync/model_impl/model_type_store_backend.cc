// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model_impl/model_type_store_backend.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/synchronization/lock.h"
#include "components/sync/protocol/model_type_store_schema_descriptor.pb.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

using sync_pb::ModelTypeStoreSchemaDescriptor;

namespace syncer {

const int64_t kInvalidSchemaVersion = -1;
const int64_t ModelTypeStoreBackend::kLatestSchemaVersion = 1;
const char ModelTypeStoreBackend::kDBSchemaDescriptorRecordId[] =
    "_mts_schema_descriptor";
const char ModelTypeStoreBackend::kStoreInitResultHistogramName[] =
    "Sync.ModelTypeStoreInitResult";

namespace {

StoreInitResultForHistogram LevelDbStatusToStoreInitResult(
    const leveldb::Status& status) {
  if (status.ok())
    return STORE_INIT_RESULT_SUCCESS;
  if (status.IsNotFound())
    return STORE_INIT_RESULT_NOT_FOUND;
  if (status.IsCorruption())
    return STORE_INIT_RESULT_CORRUPTION;
  if (status.IsNotSupportedError())
    return STORE_INIT_RESULT_NOT_SUPPORTED;
  if (status.IsInvalidArgument())
    return STORE_INIT_RESULT_INVALID_ARGUMENT;
  if (status.IsIOError())
    return STORE_INIT_RESULT_IO_ERROR;
  return STORE_INIT_RESULT_UNKNOWN;
}

// BackendMap tracks created ModelTypeStoreBackends ensuring at most one backend
// exists for a given path. BackendMap keeps non-owning pointer to backend
// allowing backend lifetime to be controlled by consumers. Since backends can
// run concurrently on different threads all map operations are guarded by lock.
class BackendMap {
 public:
  BackendMap() = default;

  // Returns backend reference or nullptr if backend for |path| is not in the
  // map.
  scoped_refptr<ModelTypeStoreBackend> GetBackend(
      const std::string& path) const;
  // Adds backend into the map ensuring it wasn't added before.
  void SetBackend(const std::string& path, ModelTypeStoreBackend* backend);
  void EraseBackend(const std::string& path);

 private:
  mutable base::Lock lock_;

  std::unordered_map<std::string, ModelTypeStoreBackend*> backends_;

  DISALLOW_COPY_AND_ASSIGN(BackendMap);
};

base::LazyInstance<BackendMap>::Leaky backend_map = LAZY_INSTANCE_INITIALIZER;

scoped_refptr<ModelTypeStoreBackend> BackendMap::GetBackend(
    const std::string& path) const {
  base::AutoLock scoped_lock(lock_);
  auto it = backends_.find(path);
  return (it == backends_.end()) ? nullptr : it->second;
}

void BackendMap::SetBackend(const std::string& path,
                            ModelTypeStoreBackend* backend) {
  base::AutoLock scoped_lock(lock_);
  DCHECK(backends_.find(path) == backends_.end());
  backends_[path] = backend;
}

void BackendMap::EraseBackend(const std::string& path) {
  base::AutoLock scoped_lock(lock_);
  backends_.erase(path);
}

}  // namespace

ModelTypeStoreBackend::ModelTypeStoreBackend(const std::string& path)
    : path_(path) {}

ModelTypeStoreBackend::~ModelTypeStoreBackend() {
  backend_map.Get().EraseBackend(path_);
}

std::unique_ptr<leveldb::Env> ModelTypeStoreBackend::CreateInMemoryEnv() {
  return leveldb_chrome::NewMemEnv("ModelTypeStore");
}

// static
scoped_refptr<ModelTypeStoreBackend> ModelTypeStoreBackend::GetOrCreateBackend(
    const std::string& path,
    std::unique_ptr<leveldb::Env> env,
    base::Optional<ModelError>* error) {
  error->reset();
  scoped_refptr<ModelTypeStoreBackend> backend =
      backend_map.Get().GetBackend(path);
  if (backend) {
    return backend;
  }

  backend = new ModelTypeStoreBackend(path);

  *error = backend->Init(path, std::move(env));

  if (!error->has_value()) {
    backend_map.Get().SetBackend(path, backend.get());
  } else {
    backend = nullptr;
  }

  return backend;
}

base::Optional<ModelError> ModelTypeStoreBackend::Init(
    const std::string& path,
    std::unique_ptr<leveldb::Env> env) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

  env_ = std::move(env);

  leveldb::Status status = OpenDatabase(path, env_.get());
  if (status.IsCorruption()) {
    DCHECK(db_ == nullptr);
    status = DestroyDatabase(path, env_.get());
    if (status.ok())
      status = OpenDatabase(path, env_.get());
    if (status.ok())
      RecordStoreInitResultHistogram(
          STORE_INIT_RESULT_RECOVERED_AFTER_CORRUPTION);
  }
  if (!status.ok()) {
    DCHECK(db_ == nullptr);
    RecordStoreInitResultHistogram(LevelDbStatusToStoreInitResult(status));
    return ModelError(FROM_HERE, status.ToString());
  }

  int64_t current_version = GetStoreVersion();
  if (current_version == kInvalidSchemaVersion) {
    RecordStoreInitResultHistogram(STORE_INIT_RESULT_SCHEMA_DESCRIPTOR_ISSUE);
    return ModelError(FROM_HERE, "Invalid schema descriptor");
  }

  if (current_version != kLatestSchemaVersion) {
    base::Optional<ModelError> error =
        Migrate(current_version, kLatestSchemaVersion);
    if (error) {
      RecordStoreInitResultHistogram(STORE_INIT_RESULT_MIGRATION);
      return error;
    }
  }
  RecordStoreInitResultHistogram(STORE_INIT_RESULT_SUCCESS);
  return base::nullopt;
}

leveldb::Status ModelTypeStoreBackend::OpenDatabase(const std::string& path,
                                                    leveldb::Env* env) {
  leveldb_env::Options options;
  options.create_if_missing = true;
  options.paranoid_checks = true;
  if (env)
    options.env = env;

  return leveldb_env::OpenDB(options, path, &db_);
}

leveldb::Status ModelTypeStoreBackend::DestroyDatabase(const std::string& path,
                                                       leveldb::Env* env) {
  leveldb_env::Options options;
  if (env)
    options.env = env;
  return leveldb::DestroyDB(path, options);
}

base::Optional<ModelError> ModelTypeStoreBackend::ReadRecordsWithPrefix(
    const std::string& prefix,
    const ModelTypeStore::IdList& id_list,
    ModelTypeStore::RecordList* record_list,
    ModelTypeStore::IdList* missing_id_list) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(db_);
  record_list->reserve(id_list.size());
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string key;
  std::string value;
  for (const std::string& id : id_list) {
    key = prefix + id;
    leveldb::Status status = db_->Get(read_options, key, &value);
    if (status.ok()) {
      record_list->emplace_back(id, value);
    } else if (status.IsNotFound()) {
      missing_id_list->push_back(id);
    } else {
      return ModelError(FROM_HERE, status.ToString());
    }
  }
  return base::nullopt;
}

base::Optional<ModelError> ModelTypeStoreBackend::ReadAllRecordsWithPrefix(
    const std::string& prefix,
    ModelTypeStore::RecordList* record_list) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(db_);
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::unique_ptr<leveldb::Iterator> iter(db_->NewIterator(read_options));
  const leveldb::Slice prefix_slice(prefix);
  for (iter->Seek(prefix_slice); iter->Valid(); iter->Next()) {
    leveldb::Slice key = iter->key();
    if (!key.starts_with(prefix_slice))
      break;
    key.remove_prefix(prefix_slice.size());
    record_list->emplace_back(key.ToString(), iter->value().ToString());
  }
  return iter->status().ok() ? base::nullopt
                             : base::Optional<ModelError>(
                                   {FROM_HERE, iter->status().ToString()});
}

base::Optional<ModelError> ModelTypeStoreBackend::WriteModifications(
    std::unique_ptr<leveldb::WriteBatch> write_batch) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(db_);
  leveldb::Status status =
      db_->Write(leveldb::WriteOptions(), write_batch.get());
  return status.ok()
             ? base::nullopt
             : base::Optional<ModelError>({FROM_HERE, status.ToString()});
}

base::Optional<ModelError>
ModelTypeStoreBackend::DeleteDataAndMetadataForPrefix(
    const std::string& prefix) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DCHECK(db_);
  leveldb::WriteBatch write_batch;
  std::unique_ptr<leveldb::Iterator> iter(
      db_->NewIterator(leveldb::ReadOptions()));
  const leveldb::Slice prefix_slice(prefix);
  for (iter->Seek(prefix_slice); iter->Valid(); iter->Next()) {
    leveldb::Slice key = iter->key();
    if (!key.starts_with(prefix_slice))
      break;
    write_batch.Delete(key);
  }
  leveldb::Status status = db_->Write(leveldb::WriteOptions(), &write_batch);
  return status.ok()
             ? base::nullopt
             : base::Optional<ModelError>({FROM_HERE, status.ToString()});
}

int64_t ModelTypeStoreBackend::GetStoreVersion() {
  DCHECK(db_);
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  ModelTypeStoreSchemaDescriptor schema_descriptor;
  leveldb::Status status =
      db_->Get(read_options, kDBSchemaDescriptorRecordId, &value);
  if (status.IsNotFound()) {
    return 0;
  } else if (!status.ok() || !schema_descriptor.ParseFromString(value)) {
    return kInvalidSchemaVersion;
  }
  return schema_descriptor.version_number();
}

base::Optional<ModelError> ModelTypeStoreBackend::Migrate(
    int64_t current_version,
    int64_t desired_version) {
  DCHECK(db_);
  if (current_version == 0) {
    if (Migrate0To1()) {
      current_version = 1;
    }
  }
  if (current_version == desired_version) {
    return base::nullopt;
  } else if (current_version > desired_version) {
    return ModelError(FROM_HERE, "Schema version too high");
  } else {
    return ModelError(FROM_HERE, "Schema upgrade failed");
  }
}

bool ModelTypeStoreBackend::Migrate0To1() {
  DCHECK(db_);
  ModelTypeStoreSchemaDescriptor schema_descriptor;
  schema_descriptor.set_version_number(1);
  leveldb::Status status =
      db_->Put(leveldb::WriteOptions(), kDBSchemaDescriptorRecordId,
               schema_descriptor.SerializeAsString());
  return status.ok();
}

// static
void ModelTypeStoreBackend::RecordStoreInitResultHistogram(
    StoreInitResultForHistogram result) {
  UMA_HISTOGRAM_ENUMERATION(kStoreInitResultHistogramName, result,
                            STORE_INIT_RESULT_COUNT);
}

// static
bool ModelTypeStoreBackend::BackendExistsForTest(const std::string& path) {
  return backend_map.Get().GetBackend(path) != nullptr;
}

}  // namespace syncer
