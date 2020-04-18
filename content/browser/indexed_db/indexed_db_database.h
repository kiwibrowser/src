// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/queue.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/indexed_db/indexed_db.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_callbacks.h"
#include "content/browser/indexed_db/indexed_db_observer.h"
#include "content/browser/indexed_db/indexed_db_pending_connection.h"
#include "content/browser/indexed_db/indexed_db_transaction_coordinator.h"
#include "content/browser/indexed_db/list_set.h"
#include "content/common/content_export.h"
#include "content/common/indexed_db/indexed_db_metadata.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace url {
class Origin;
}

namespace content {

class IndexedDBConnection;
class IndexedDBDatabaseCallbacks;
class IndexedDBFactory;
class IndexedDBKey;
class IndexedDBKeyPath;
class IndexedDBKeyRange;
class IndexedDBMetadataCoding;
class IndexedDBTransaction;
struct IndexedDBValue;

class CONTENT_EXPORT IndexedDBDatabase
    : public base::RefCounted<IndexedDBDatabase> {
 public:
  // Identifier is pair of (origin, database name).
  using Identifier = std::pair<url::Origin, base::string16>;

  static const int64_t kInvalidId = 0;
  static const int64_t kMinimumIndexId = 30;

  static std::tuple<scoped_refptr<IndexedDBDatabase>, leveldb::Status> Create(
      const base::string16& name,
      scoped_refptr<IndexedDBBackingStore> backing_store,
      scoped_refptr<IndexedDBFactory> factory,
      std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
      const Identifier& unique_identifier);

  const Identifier& identifier() const { return identifier_; }
  IndexedDBBackingStore* backing_store() { return backing_store_.get(); }

  int64_t id() const { return metadata_.id; }
  const base::string16& name() const { return metadata_.name; }
  const url::Origin& origin() const { return identifier_.first; }

  void AddObjectStore(IndexedDBObjectStoreMetadata metadata,
                      int64_t new_max_object_store_id);
  IndexedDBObjectStoreMetadata RemoveObjectStore(int64_t object_store_id);
  void AddIndex(int64_t object_store_id,
                IndexedDBIndexMetadata metadata,
                int64_t new_max_index_id);
  IndexedDBIndexMetadata RemoveIndex(int64_t object_store_id, int64_t index_id);

  void OpenConnection(std::unique_ptr<IndexedDBPendingConnection> connection);
  void DeleteDatabase(scoped_refptr<IndexedDBCallbacks> callbacks,
                      bool force_close);
  const IndexedDBDatabaseMetadata& metadata() const { return metadata_; }

  void CreateObjectStore(IndexedDBTransaction* transaction,
                         int64_t object_store_id,
                         const base::string16& name,
                         const IndexedDBKeyPath& key_path,
                         bool auto_increment);
  void DeleteObjectStore(IndexedDBTransaction* transaction,
                         int64_t object_store_id);
  void RenameObjectStore(IndexedDBTransaction* transaction,
                         int64_t object_store_id,
                         const base::string16& new_name);

  // Returns a pointer to a newly created transaction. The object is owned
  // by |transaction_coordinator_|.
  IndexedDBTransaction* CreateTransaction(
      int64_t transaction_id,
      IndexedDBConnection* connection,
      const std::vector<int64_t>& object_store_ids,
      blink::WebIDBTransactionMode mode);
  void Close(IndexedDBConnection* connection, bool forced);
  void ForceClose();

  // Ack that one of the connections notified with a "versionchange" event did
  // not promptly close. Therefore a "blocked" event should be fired at the
  // pending connection.
  void VersionChangeIgnored();

  void Commit(IndexedDBTransaction* transaction);

  void CreateIndex(IndexedDBTransaction* transaction,
                   int64_t object_store_id,
                   int64_t index_id,
                   const base::string16& name,
                   const IndexedDBKeyPath& key_path,
                   bool unique,
                   bool multi_entry);
  void DeleteIndex(IndexedDBTransaction* transaction,
                   int64_t object_store_id,
                   int64_t index_id);
  void RenameIndex(IndexedDBTransaction* transaction,
                   int64_t object_store_id,
                   int64_t index_id,
                   const base::string16& new_name);

  IndexedDBTransactionCoordinator& transaction_coordinator() {
    return transaction_coordinator_;
  }
  const IndexedDBTransactionCoordinator& transaction_coordinator() const {
    return transaction_coordinator_;
  }

  void TransactionCreated(IndexedDBTransaction* transaction);
  void TransactionFinished(IndexedDBTransaction* transaction, bool committed);

  void AbortAllTransactionsForConnections();

  void AddPendingObserver(IndexedDBTransaction* transaction,
                          int32_t observer_id,
                          const IndexedDBObserver::Options& options);

  // |value| can be null for delete and clear operations.
  void FilterObservation(IndexedDBTransaction*,
                         int64_t object_store_id,
                         blink::WebIDBOperationType type,
                         const IndexedDBKeyRange& key_range,
                         const IndexedDBValue* value);
  void SendObservations(
      std::map<int32_t, ::indexed_db::mojom::ObserverChangesPtr> change_map);

  void Get(IndexedDBTransaction* transaction,
           int64_t object_store_id,
           int64_t index_id,
           std::unique_ptr<IndexedDBKeyRange> key_range,
           bool key_only,
           scoped_refptr<IndexedDBCallbacks> callbacks);
  void GetAll(IndexedDBTransaction* transaction,
              int64_t object_store_id,
              int64_t index_id,
              std::unique_ptr<IndexedDBKeyRange> key_range,
              bool key_only,
              int64_t max_count,
              scoped_refptr<IndexedDBCallbacks> callbacks);
  void Put(IndexedDBTransaction* transaction,
           int64_t object_store_id,
           IndexedDBValue* value,
           std::vector<std::unique_ptr<storage::BlobDataHandle>>* handles,
           std::unique_ptr<IndexedDBKey> key,
           blink::WebIDBPutMode mode,
           scoped_refptr<IndexedDBCallbacks> callbacks,
           const std::vector<IndexedDBIndexKeys>& index_keys);
  void SetIndexKeys(IndexedDBTransaction* transaction,
                    int64_t object_store_id,
                    std::unique_ptr<IndexedDBKey> primary_key,
                    const std::vector<IndexedDBIndexKeys>& index_keys);
  void SetIndexesReady(IndexedDBTransaction* transaction,
                       int64_t object_store_id,
                       const std::vector<int64_t>& index_ids);
  void OpenCursor(IndexedDBTransaction* transaction,
                  int64_t object_store_id,
                  int64_t index_id,
                  std::unique_ptr<IndexedDBKeyRange> key_range,
                  blink::WebIDBCursorDirection,
                  bool key_only,
                  blink::WebIDBTaskType task_type,
                  scoped_refptr<IndexedDBCallbacks> callbacks);
  void Count(IndexedDBTransaction* transaction,
             int64_t object_store_id,
             int64_t index_id,
             std::unique_ptr<IndexedDBKeyRange> key_range,
             scoped_refptr<IndexedDBCallbacks> callbacks);
  void DeleteRange(IndexedDBTransaction* transaction,
                   int64_t object_store_id,
                   std::unique_ptr<IndexedDBKeyRange> key_range,
                   scoped_refptr<IndexedDBCallbacks> callbacks);
  void Clear(IndexedDBTransaction* transaction,
             int64_t object_store_id,
             scoped_refptr<IndexedDBCallbacks> callbacks);

  // Number of connections that have progressed passed initial open call.
  size_t ConnectionCount() const { return connections_.size(); }

  // Number of active open/delete calls (running or blocked on other
  // connections).
  size_t ActiveOpenDeleteCount() const { return active_request_ ? 1 : 0; }

  // Number of open/delete calls that are waiting their turn.
  size_t PendingOpenDeleteCount() const { return pending_requests_.size(); }

  // Asynchronous tasks scheduled within transactions:
  void CreateObjectStoreAbortOperation(int64_t object_store_id);
  leveldb::Status DeleteObjectStoreOperation(int64_t object_store_id,
                                             IndexedDBTransaction* transaction);
  void DeleteObjectStoreAbortOperation(
      IndexedDBObjectStoreMetadata object_store_metadata);
  void RenameObjectStoreAbortOperation(int64_t object_store_id,
                                       base::string16 old_name);
  leveldb::Status VersionChangeOperation(
      int64_t version,
      scoped_refptr<IndexedDBCallbacks> callbacks,
      IndexedDBTransaction* transaction);
  void VersionChangeAbortOperation(int64_t previous_version);
  leveldb::Status DeleteIndexOperation(int64_t object_store_id,
                                       int64_t index_id,
                                       IndexedDBTransaction* transaction);
  void CreateIndexAbortOperation(int64_t object_store_id, int64_t index_id);
  void DeleteIndexAbortOperation(int64_t object_store_id,
                                 IndexedDBIndexMetadata index_metadata);
  void RenameIndexAbortOperation(int64_t object_store_id,
                                 int64_t index_id,
                                 base::string16 old_name);
  leveldb::Status GetOperation(int64_t object_store_id,
                               int64_t index_id,
                               std::unique_ptr<IndexedDBKeyRange> key_range,
                               indexed_db::CursorType cursor_type,
                               scoped_refptr<IndexedDBCallbacks> callbacks,
                               IndexedDBTransaction* transaction);
  leveldb::Status GetAllOperation(int64_t object_store_id,
                                  int64_t index_id,
                                  std::unique_ptr<IndexedDBKeyRange> key_range,
                                  indexed_db::CursorType cursor_type,
                                  int64_t max_count,
                                  scoped_refptr<IndexedDBCallbacks> callbacks,
                                  IndexedDBTransaction* transaction);
  struct PutOperationParams;
  leveldb::Status PutOperation(std::unique_ptr<PutOperationParams> params,
                               IndexedDBTransaction* transaction);
  leveldb::Status SetIndexesReadyOperation(size_t index_count,
                                           IndexedDBTransaction* transaction);
  struct OpenCursorOperationParams;
  leveldb::Status OpenCursorOperation(
      std::unique_ptr<OpenCursorOperationParams> params,
      IndexedDBTransaction* transaction);
  leveldb::Status CountOperation(int64_t object_store_id,
                                 int64_t index_id,
                                 std::unique_ptr<IndexedDBKeyRange> key_range,
                                 scoped_refptr<IndexedDBCallbacks> callbacks,
                                 IndexedDBTransaction* transaction);
  leveldb::Status DeleteRangeOperation(
      int64_t object_store_id,
      std::unique_ptr<IndexedDBKeyRange> key_range,
      scoped_refptr<IndexedDBCallbacks> callbacks,
      IndexedDBTransaction* transaction);
  leveldb::Status ClearOperation(int64_t object_store_id,
                                 scoped_refptr<IndexedDBCallbacks> callbacks,
                                 IndexedDBTransaction* transaction);

  // Called when a backing store operation has failed. The database will be
  // closed (IndexedDBFactory::ForceClose) during this call. This should NOT
  // be used in an method scheduled as a transaction operation.
  void ReportError(leveldb::Status status);
  void ReportErrorWithDetails(leveldb::Status status, const char* message);

  IndexedDBFactory* factory() const { return factory_.get(); }

 protected:
  friend class IndexedDBTransaction;

  IndexedDBDatabase(const base::string16& name,
                    scoped_refptr<IndexedDBBackingStore> backing_store,
                    scoped_refptr<IndexedDBFactory> factory,
                    std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
                    const Identifier& unique_identifier);
  virtual ~IndexedDBDatabase();

  // May be overridden in tests.
  virtual size_t GetMaxMessageSizeInBytes() const;

 private:
  friend class base::RefCounted<IndexedDBDatabase>;
  friend class IndexedDBClassFactory;

  FRIEND_TEST_ALL_PREFIXES(IndexedDBDatabaseTest, OpenDeleteClear);

  void CallUpgradeTransactionStartedForTesting(int64_t old_version);

  class ConnectionRequest;
  class OpenRequest;
  class DeleteRequest;

  leveldb::Status OpenInternal();

  // Called internally when an open or delete request comes in. Processes
  // the queue immediately if there are no other requests.
  void AppendRequest(std::unique_ptr<ConnectionRequest> request);

  // Called by requests when complete. The request will be freed, so the
  // request must do no other work after calling this. If there are pending
  // requests, the queue will be synchronously processed.
  void RequestComplete(ConnectionRequest* request);

  // Pop the first request from the queue and start it.
  void ProcessRequestQueue();

  std::unique_ptr<IndexedDBConnection> CreateConnection(
      scoped_refptr<IndexedDBDatabaseCallbacks> database_callbacks,
      int child_process_id);

  bool ValidateObjectStoreId(int64_t object_store_id) const;
  bool ValidateObjectStoreIdAndIndexId(int64_t object_store_id,
                                       int64_t index_id) const;
  bool ValidateObjectStoreIdAndOptionalIndexId(int64_t object_store_id,
                                               int64_t index_id) const;
  bool ValidateObjectStoreIdAndNewIndexId(int64_t object_store_id,
                                          int64_t index_id) const;

  scoped_refptr<IndexedDBBackingStore> backing_store_;
  IndexedDBDatabaseMetadata metadata_;

  const Identifier identifier_;
  scoped_refptr<IndexedDBFactory> factory_;
  std::unique_ptr<IndexedDBMetadataCoding> metadata_coding_;

  IndexedDBTransactionCoordinator transaction_coordinator_;
  int64_t transaction_count_ = 0;

  list_set<IndexedDBConnection*> connections_;

  // This holds the first open or delete request that is currently being
  // processed. The request has already broadcast OnVersionChange if
  // necessary.
  std::unique_ptr<ConnectionRequest> active_request_;

  // This holds open or delete requests that are waiting for the active
  // request to be completed. The requests have not yet broadcast
  // OnVersionChange (if necessary).
  base::queue<std::unique_ptr<ConnectionRequest>> pending_requests_;

  // The |processing_pending_requests_| flag is set while ProcessRequestQueue()
  // is executing. It prevents rentrant calls if the active request completes
  // synchronously.
  bool processing_pending_requests_ = false;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBDatabase);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_H_
