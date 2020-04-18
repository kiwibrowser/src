// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model_impl/model_type_store_impl.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/optional.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model_impl/model_type_store_backend.h"
#include "components/sync/protocol/entity_metadata.pb.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace syncer {

namespace {

// Key prefix for data/metadata records.
const char kDataPrefix[] = "-dt-";
const char kMetadataPrefix[] = "-md-";

// Key for global metadata record.
const char kGlobalMetadataKey[] = "-GlobalMetadata";

// Formats key prefix for data records of |type|.
std::string FormatDataPrefix(ModelType type) {
  return std::string(GetModelTypeRootTag(type)) + kDataPrefix;
}

// Formats key prefix for metadata records of |type|.
std::string FormatMetaPrefix(ModelType type) {
  return std::string(GetModelTypeRootTag(type)) + kMetadataPrefix;
}

// Formats key for global metadata record of |type|.
std::string FormatGlobalMetadataKey(ModelType type) {
  return std::string(GetModelTypeRootTag(type)) + kGlobalMetadataKey;
}

// Holds a one to one mapping between profile path and SequencedTaskRunner. This
// class is expected to be accessed on any thread, and uses a lock to guarantee
// thread safety. The task runners are held onto by scoped_refptrs, and since
// this class is leaky, none of these task runners are ever destroyed.
class TaskRunnerMap {
 public:
  TaskRunnerMap() = default;

  scoped_refptr<base::SequencedTaskRunner> GetOrCreateTaskRunner(
      const std::string& path) {
    base::AutoLock scoped_lock(lock_);
    auto iter = task_runner_map_.find(path);
    if (iter == task_runner_map_.end()) {
      scoped_refptr<base::SequencedTaskRunner> task_runner =
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
      task_runner_map_[path] = task_runner;
      return task_runner;
    } else {
      return iter->second;
    }
  }

 private:
  mutable base::Lock lock_;
  std::map<std::string, scoped_refptr<base::SequencedTaskRunner>>
      task_runner_map_;

  DISALLOW_COPY_AND_ASSIGN(TaskRunnerMap);
};

base::LazyInstance<TaskRunnerMap>::Leaky task_runner_map_singleton =
    LAZY_INSTANCE_INITIALIZER;

class LevelDbMetadataChangeList : public MetadataChangeList {
 public:
  LevelDbMetadataChangeList(ModelType type,
                            leveldb::WriteBatch* leveldb_write_batch)
      : leveldb_write_batch_(leveldb_write_batch),
        metadata_prefix_(FormatMetaPrefix(type)),
        global_metadata_key_(FormatGlobalMetadataKey(type)) {
    DCHECK(leveldb_write_batch_);
  }

  // MetadataChangeList implementation.
  void UpdateModelTypeState(
      const sync_pb::ModelTypeState& model_type_state) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Put(global_metadata_key_,
                              model_type_state.SerializeAsString());
  }

  void ClearModelTypeState() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Delete(global_metadata_key_);
  }

  void UpdateMetadata(const std::string& storage_key,
                      const sync_pb::EntityMetadata& metadata) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Put(FormatMetadataKey(storage_key),
                              metadata.SerializeAsString());
  }

  void ClearMetadata(const std::string& storage_key) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Delete(FormatMetadataKey(storage_key));
  }

 private:
  // Format key for metadata records with given id.
  std::string FormatMetadataKey(const std::string& id) const {
    return metadata_prefix_ + id;
  }

  leveldb::WriteBatch* const leveldb_write_batch_;

  // Key for this type's metadata records.
  const std::string metadata_prefix_;
  const std::string global_metadata_key_;

  SEQUENCE_CHECKER(sequence_checker_);
};

class LevelDbWriteBatch : public ModelTypeStore::WriteBatch {
 public:
  static std::unique_ptr<leveldb::WriteBatch> ToLevelDbWriteBatch(
      std::unique_ptr<LevelDbWriteBatch> batch) {
    return std::move(batch->leveldb_write_batch_);
  }

  explicit LevelDbWriteBatch(ModelType type)
      : data_prefix_(FormatDataPrefix(type)),
        leveldb_write_batch_(std::make_unique<leveldb::WriteBatch>()),
        metadata_change_list_(type, leveldb_write_batch_.get()) {}

  ~LevelDbWriteBatch() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  // WriteBatch implementation.
  void WriteData(const std::string& id, const std::string& value) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Put(FormatDataKey(id), value);
  }

  void DeleteData(const std::string& id) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    leveldb_write_batch_->Delete(FormatDataKey(id));
  }

  MetadataChangeList* GetMetadataChangeList() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return &metadata_change_list_;
  }

 private:
  // Format key for data records with given id.
  std::string FormatDataKey(const std::string& id) const {
    return data_prefix_ + id;
  }

  // Key prefix for data records of this model type.
  const std::string data_prefix_;

  std::unique_ptr<leveldb::WriteBatch> leveldb_write_batch_;
  LevelDbMetadataChangeList metadata_change_list_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace

ModelTypeStoreImpl::ModelTypeStoreImpl(
    ModelType type,
    scoped_refptr<ModelTypeStoreBackend> backend,
    scoped_refptr<base::SequencedTaskRunner> backend_task_runner)
    : backend_(backend),
      backend_task_runner_(backend_task_runner),
      type_(type),
      data_prefix_(FormatDataPrefix(type)),
      metadata_prefix_(FormatMetaPrefix(type)),
      global_metadata_key_(FormatGlobalMetadataKey(type)),
      weak_ptr_factory_(this) {
  DCHECK(backend_);
  DCHECK(backend_task_runner_);
}

ModelTypeStoreImpl::~ModelTypeStoreImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(pkasting): For some reason, using ReleaseSoon() here (which would be
  // more idiomatic) breaks tests... perhaps the non-nestable task that posts
  // runs later than this nestable one, and violates some assumption?
  backend_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::DoNothing::Once<scoped_refptr<ModelTypeStoreBackend>>(),
          std::move(backend_)));
}

// static
void ModelTypeStoreImpl::CreateStore(ModelType type,
                                     const std::string& path,
                                     InitCallback callback) {
  DCHECK(!callback.is_null());
  std::unique_ptr<leveldb::Env> env;
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      task_runner_map_singleton.Get().GetOrCreateTaskRunner(path);
  auto error = std::make_unique<base::Optional<ModelError>>();
  auto task = base::BindOnce(&ModelTypeStoreBackend::GetOrCreateBackend, path,
                             std::move(env), error.get());
  auto reply =
      base::BindOnce(&ModelTypeStoreImpl::BackendInitDone, type,
                     std::move(error), task_runner, std::move(callback));
  base::PostTaskAndReplyWithResult(task_runner.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

// static
void ModelTypeStoreImpl::CreateInMemoryStoreForTest(ModelType type,
                                                    InitCallback callback) {
  DCHECK(!callback.is_null());

  std::unique_ptr<leveldb::Env> env =
      ModelTypeStoreBackend::CreateInMemoryEnv();

  std::string path;
  env->GetTestDirectory(&path);
  path += "/in-memory";

  // In-memory store backend works on the same thread as test.
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();

  auto error = std::make_unique<base::Optional<ModelError>>();
  auto task = base::BindOnce(&ModelTypeStoreBackend::GetOrCreateBackend, path,
                             std::move(env), error.get());
  auto reply =
      base::BindOnce(&ModelTypeStoreImpl::BackendInitDone, type,
                     std::move(error), task_runner, std::move(callback));

  base::PostTaskAndReplyWithResult(task_runner.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

// static
void ModelTypeStoreImpl::BackendInitDone(
    ModelType type,
    std::unique_ptr<base::Optional<ModelError>> error,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner,
    InitCallback callback,
    scoped_refptr<ModelTypeStoreBackend> backend) {
  std::unique_ptr<ModelTypeStoreImpl> store;
  if (!error->has_value()) {
    store.reset(new ModelTypeStoreImpl(type, backend, blocking_task_runner));
  }

  std::move(callback).Run(*error, std::move(store));
}

// Note on pattern for communicating with backend:
//  - API function (e.g. ReadData) allocates lists for output.
//  - API function prepares two callbacks: task that will be posted on backend
//    thread and reply which will be posted on model type thread once task
//    finishes.
//  - Task for backend thread takes raw pointers to output lists while reply
//    takes ownership of those lists. This allows backend interface to be simple
//    while ensuring proper objects' lifetime.
//  - Function bound by reply calls consumer's callback and passes ownership of
//    output lists to it.

void ModelTypeStoreImpl::ReadData(const IdList& id_list,
                                  ReadDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback.is_null());
  std::unique_ptr<RecordList> record_list(new RecordList());
  std::unique_ptr<IdList> missing_id_list(new IdList());

  auto task = base::BindOnce(&ModelTypeStoreBackend::ReadRecordsWithPrefix,
                             base::Unretained(backend_.get()), data_prefix_,
                             id_list, base::Unretained(record_list.get()),
                             base::Unretained(missing_id_list.get()));
  auto reply = base::BindOnce(
      &ModelTypeStoreImpl::ReadDataDone, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(record_list), std::move(missing_id_list));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::ReadDataDone(ReadDataCallback callback,
                                      std::unique_ptr<RecordList> record_list,
                                      std::unique_ptr<IdList> missing_id_list,
                                      const base::Optional<ModelError>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(error, std::move(record_list),
                          std::move(missing_id_list));
}

void ModelTypeStoreImpl::ReadAllData(ReadAllDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback.is_null());
  std::unique_ptr<RecordList> record_list(new RecordList());
  auto task = base::BindOnce(&ModelTypeStoreBackend::ReadAllRecordsWithPrefix,
                             base::Unretained(backend_.get()), data_prefix_,
                             base::Unretained(record_list.get()));
  auto reply = base::BindOnce(&ModelTypeStoreImpl::ReadAllDataDone,
                              weak_ptr_factory_.GetWeakPtr(),
                              std::move(callback), std::move(record_list));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::ReadAllDataDone(
    ReadAllDataCallback callback,
    std::unique_ptr<RecordList> record_list,
    const base::Optional<ModelError>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(error, std::move(record_list));
}

void ModelTypeStoreImpl::ReadAllMetadata(ReadMetadataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback.is_null());

  // ReadAllMetadata performs two steps sequentially: read all metadata records
  // and then read global metadata record. Start reading metadata records here.
  // When this read operation is done ReadMetadataRecordsDone callback will
  // issue read operation for global metadata record.
  std::unique_ptr<RecordList> metadata_records(new RecordList());
  auto task = base::BindOnce(&ModelTypeStoreBackend::ReadAllRecordsWithPrefix,
                             base::Unretained(backend_.get()), metadata_prefix_,
                             base::Unretained(metadata_records.get()));
  auto reply = base::BindOnce(&ModelTypeStoreImpl::ReadMetadataRecordsDone,
                              weak_ptr_factory_.GetWeakPtr(),
                              std::move(callback), std::move(metadata_records));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::ReadMetadataRecordsDone(
    ReadMetadataCallback callback,
    std::unique_ptr<RecordList> metadata_records,
    const base::Optional<ModelError>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    std::move(callback).Run(*error, std::make_unique<MetadataBatch>());
    return;
  }

  IdList global_metadata_id;
  global_metadata_id.push_back(global_metadata_key_);
  std::unique_ptr<RecordList> global_metadata_records(new RecordList());
  std::unique_ptr<IdList> missing_id_list(new IdList());
  auto task = base::BindOnce(&ModelTypeStoreBackend::ReadRecordsWithPrefix,
                             base::Unretained(backend_.get()), std::string(),
                             global_metadata_id,
                             base::Unretained(global_metadata_records.get()),
                             base::Unretained(missing_id_list.get()));
  auto reply = base::BindOnce(
      &ModelTypeStoreImpl::ReadAllMetadataDone, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(metadata_records),
      std::move(global_metadata_records), std::move(missing_id_list));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::ReadAllMetadataDone(
    ReadMetadataCallback callback,
    std::unique_ptr<RecordList> metadata_records,
    std::unique_ptr<RecordList> global_metadata_records,
    std::unique_ptr<IdList> missing_id_list,
    const base::Optional<ModelError>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (error) {
    std::move(callback).Run(*error, std::make_unique<MetadataBatch>());
    return;
  }

  std::string global_metadata;
  if (!missing_id_list->empty()) {
    // Missing global metadata record is not an error; we can just return the
    // default instance using the empty string above.
    DCHECK_EQ(global_metadata_key_, (*missing_id_list)[0]);
    DCHECK(global_metadata_records->empty());
  } else {
    DCHECK_EQ(1U, global_metadata_records->size());
    DCHECK_EQ(global_metadata_key_, (*global_metadata_records)[0].id);
    global_metadata = (*global_metadata_records)[0].value;
  }

  DeserializeMetadata(std::move(callback), global_metadata,
                      std::move(metadata_records));
}

void ModelTypeStoreImpl::DeserializeMetadata(
    ReadMetadataCallback callback,
    const std::string& global_metadata,
    std::unique_ptr<RecordList> metadata_records) {
  auto metadata_batch = std::make_unique<MetadataBatch>();

  sync_pb::ModelTypeState state;
  if (!state.ParseFromString(global_metadata)) {
    std::move(callback).Run(
        ModelError(FROM_HERE, "Failed to deserialize model type state."),
        std::make_unique<MetadataBatch>());
    return;
  }
  metadata_batch->SetModelTypeState(state);

  for (const Record& r : *metadata_records) {
    sync_pb::EntityMetadata entity_metadata;
    if (!entity_metadata.ParseFromString(r.value)) {
      std::move(callback).Run(
          ModelError(FROM_HERE, "Failed to deserialize entity metadata."),
          std::make_unique<MetadataBatch>());
      return;
    }
    metadata_batch->AddMetadata(r.id, entity_metadata);
  }

  std::move(callback).Run({}, std::move(metadata_batch));
}

std::unique_ptr<ModelTypeStore::WriteBatch>
ModelTypeStoreImpl::CreateWriteBatch() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::make_unique<LevelDbWriteBatch>(type_);
}

void ModelTypeStoreImpl::CommitWriteBatch(
    std::unique_ptr<WriteBatch> write_batch,
    CallbackWithResult callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback.is_null());
  std::unique_ptr<LevelDbWriteBatch> write_batch_impl(
      static_cast<LevelDbWriteBatch*>(write_batch.release()));
  auto task = base::BindOnce(
      &ModelTypeStoreBackend::WriteModifications,
      base::Unretained(backend_.get()),
      LevelDbWriteBatch::ToLevelDbWriteBatch(std::move(write_batch_impl)));
  auto reply =
      base::BindOnce(&ModelTypeStoreImpl::WriteModificationsDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::DeleteAllDataAndMetadata(CallbackWithResult callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!callback.is_null());
  auto task = base::BindOnce(
      &ModelTypeStoreBackend::DeleteDataAndMetadataForPrefix,
      base::Unretained(backend_.get()), GetModelTypeRootTag(type_));
  auto reply =
      base::BindOnce(&ModelTypeStoreImpl::WriteModificationsDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  base::PostTaskAndReplyWithResult(backend_task_runner_.get(), FROM_HERE,
                                   std::move(task), std::move(reply));
}

void ModelTypeStoreImpl::WriteModificationsDone(
    CallbackWithResult callback,
    const base::Optional<ModelError>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(error);
}

}  // namespace syncer
