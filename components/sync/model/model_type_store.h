// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_MODEL_TYPE_STORE_H_
#define COMPONENTS_SYNC_MODEL_MODEL_TYPE_STORE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/sync/base/model_type.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"

namespace syncer {

// ModelTypeStore is leveldb backed store for model type's data, metadata and
// global metadata.
//
// Store keeps records for entries identified by ids. For each entry store keeps
// data and metadata. Also store keeps one record for global metadata.
//
// To create store call one of Create*Store static factory functions. Model type
// controls store's lifetime with returned unique_ptr. Call to Create*Store
// function triggers asynchronous store backend initialization, callback will be
// called with results when initialization is done.
//
// Read operations are asynchronous, initiated with one of Read* functions,
// provided callback will be called with result code and output of read
// operation.
//
// Write operations are done in context of write batch. To get one call
// CreateWriteBatch(). After that pass write batch object to Write/Delete
// functions. WriteBatch only accumulates pending changes, doesn't actually do
// data modification. Calling CommitWriteBatch writes all accumulated changes to
// disk atomically. Callback passed to CommitWriteBatch will be called with
// result of write operation. If write batch object is destroyed without
// comitting accumulated write operations will not be persisted.
//
// Destroying store object doesn't necessarily cancel asynchronous operations
// issued previously. You should be prepared to handle callbacks from those
// operations.
class ModelTypeStore {
 public:
  // Output of read operations is passed back as list of Record structures.
  struct Record {
    Record(const std::string& id, const std::string& value)
        : id(id), value(value) {}

    std::string id;
    std::string value;
  };

  // WriteBatch object is used in all modification operations.
  class WriteBatch {
   public:
    // Creates a MetadataChangeList that will accumulate metadata changes and
    // can later be passed to a WriteBatch via TransferChanges. Use this when
    // you need a MetadataChangeList and do not have a WriteBatch in scope.
    static std::unique_ptr<MetadataChangeList> CreateMetadataChangeList();

    WriteBatch();
    virtual ~WriteBatch();

    // Write the given |value| for data with |id|.
    virtual void WriteData(const std::string& id, const std::string& value) = 0;

    // Delete the record for data with |id|.
    virtual void DeleteData(const std::string& id) = 0;

    // Provides access to a MetadataChangeList that will pass its changes
    // directly into this WriteBatch.
    virtual MetadataChangeList* GetMetadataChangeList() = 0;

    // Transfers the changes from a MetadataChangeList into this WriteBatch.
    // |mcl| must have previously been created by CreateMetadataChangeList().
    // TODO(mastiz): Revisit whether the last requirement above can be removed
    // and make this API more type-safe.
    void TakeMetadataChangesFrom(std::unique_ptr<MetadataChangeList> mcl);

   private:
    DISALLOW_COPY_AND_ASSIGN(WriteBatch);
  };

  using RecordList = std::vector<Record>;
  using IdList = std::vector<std::string>;

  using InitCallback =
      base::OnceCallback<void(const base::Optional<ModelError>& error,
                              std::unique_ptr<ModelTypeStore> store)>;
  using CallbackWithResult =
      base::OnceCallback<void(const base::Optional<ModelError>& error)>;
  using ReadDataCallback =
      base::OnceCallback<void(const base::Optional<ModelError>& error,
                              std::unique_ptr<RecordList> data_records,
                              std::unique_ptr<IdList> missing_id_list)>;
  using ReadAllDataCallback =
      base::OnceCallback<void(const base::Optional<ModelError>& error,
                              std::unique_ptr<RecordList> data_records)>;
  using ReadMetadataCallback =
      base::OnceCallback<void(const base::Optional<ModelError>& error,
                              std::unique_ptr<MetadataBatch> metadata_batch)>;

  // CreateStore takes |path|, and will run blocking calls on a task runner
  // scoped to the given path. Tests likely don't want to use this method.
  static void CreateStore(const std::string& path,
                          ModelType type,
                          InitCallback callback);
  // Creates store object backed by in-memory leveldb database, gets its task
  // runner from MessageLoop::task_runner(), and should only be used in tests.
  static void CreateInMemoryStoreForTest(ModelType type, InitCallback callback);

  virtual ~ModelTypeStore();

  // Read operations return records either for all entries or only for ones
  // identified in |id_list|. |error| is nullopt if all records were read
  // successfully, otherwise an empty or partial list of read records is
  // returned.
  // Callback for ReadData (ReadDataCallback) in addition receives list of ids
  // that were not found in store (missing_id_list).
  virtual void ReadData(const IdList& id_list, ReadDataCallback callback) = 0;
  virtual void ReadAllData(ReadAllDataCallback callback) = 0;
  // ReadMetadataCallback will be invoked with three parameters: result of
  // operation, list of metadata records and global metadata.
  virtual void ReadAllMetadata(ReadMetadataCallback callback) = 0;

  // Creates write batch for write operations.
  virtual std::unique_ptr<WriteBatch> CreateWriteBatch() = 0;

  // Commits write operations accumulated in write batch. If write operation
  // fails result is UNSPECIFIED_ERROR and write operations will not be
  // reflected in the store.
  virtual void CommitWriteBatch(std::unique_ptr<WriteBatch> write_batch,
                                CallbackWithResult callback) = 0;

  // Deletion of everything, usually exercised during DisableSync().
  virtual void DeleteAllDataAndMetadata(CallbackWithResult callback) = 0;
};

// Typedef for a store factory that has all params bound except InitCallback.
using RepeatingModelTypeStoreFactory =
    base::RepeatingCallback<void(ModelType type, ModelTypeStore::InitCallback)>;

// Same as above but as a OnceCallback.
using OnceModelTypeStoreFactory =
    base::OnceCallback<void(ModelType type, ModelTypeStore::InitCallback)>;

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_MODEL_TYPE_STORE_H_
