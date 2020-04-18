// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/database_impl.h"

#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_math.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "content/browser/bad_message.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/indexed_db/indexed_db_connection.h"
#include "content/browser/indexed_db/indexed_db_context_impl.h"
#include "content/browser/indexed_db/indexed_db_dispatcher_host.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/browser/indexed_db/indexed_db_value.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/quota/quota_manager_proxy.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_exception.h"

using std::swap;

namespace content {
class IndexedDBDatabaseError;

namespace {
const char kInvalidBlobUuid[] = "Blob does not exist";
const char kInvalidBlobFilePath[] = "Blob file path is invalid";
}  // namespace

// Expect to be created on IO thread, and called/destroyed on IDB sequence.
class DatabaseImpl::IDBSequenceHelper {
 public:
  IDBSequenceHelper(std::unique_ptr<IndexedDBConnection> connection,
                    const url::Origin& origin,
                    scoped_refptr<IndexedDBContextImpl> indexed_db_context);
  ~IDBSequenceHelper();

  void ConnectionOpened();

  void CreateObjectStore(int64_t transaction_id,
                         int64_t object_store_id,
                         const base::string16& name,
                         const IndexedDBKeyPath& key_path,
                         bool auto_increment);
  void DeleteObjectStore(int64_t transaction_id, int64_t object_store_id);
  void RenameObjectStore(int64_t transaction_id,
                         int64_t object_store_id,
                         const base::string16& new_name);
  void CreateTransaction(int64_t transaction_id,
                         const std::vector<int64_t>& object_store_ids,
                         blink::WebIDBTransactionMode mode);
  void Close();
  void VersionChangeIgnored();
  void AddObserver(int64_t transaction_id,
                   int32_t observer_id,
                   bool include_transaction,
                   bool no_records,
                   bool values,
                   uint16_t operation_types);
  void RemoveObservers(const std::vector<int32_t>& observers);
  void Get(int64_t transaction_id,
           int64_t object_store_id,
           int64_t index_id,
           const IndexedDBKeyRange& key_range,
           bool key_only,
           scoped_refptr<IndexedDBCallbacks> callbacks);
  void GetAll(int64_t transaction_id,
              int64_t object_store_id,
              int64_t index_id,
              const IndexedDBKeyRange& key_range,
              bool key_only,
              int64_t max_count,
              scoped_refptr<IndexedDBCallbacks> callbacks);
  void Put(int64_t transaction_id,
           int64_t object_store_id,
           ::indexed_db::mojom::ValuePtr value,
           std::vector<std::unique_ptr<storage::BlobDataHandle>> handles,
           std::vector<IndexedDBBlobInfo> blob_info,
           const IndexedDBKey& key,
           blink::WebIDBPutMode mode,
           const std::vector<IndexedDBIndexKeys>& index_keys,
           scoped_refptr<IndexedDBCallbacks> callbacks);
  void SetIndexKeys(int64_t transaction_id,
                    int64_t object_store_id,
                    const IndexedDBKey& primary_key,
                    const std::vector<IndexedDBIndexKeys>& index_keys);
  void SetIndexesReady(int64_t transaction_id,
                       int64_t object_store_id,
                       const std::vector<int64_t>& index_ids);
  void OpenCursor(int64_t transaction_id,
                  int64_t object_store_id,
                  int64_t index_id,
                  const IndexedDBKeyRange& key_range,
                  blink::WebIDBCursorDirection direction,
                  bool key_only,
                  blink::WebIDBTaskType task_type,
                  scoped_refptr<IndexedDBCallbacks> callbacks);
  void Count(int64_t transaction_id,
             int64_t object_store_id,
             int64_t index_id,
             const IndexedDBKeyRange& key_range,
             scoped_refptr<IndexedDBCallbacks> callbacks);
  void DeleteRange(int64_t transaction_id,
                   int64_t object_store_id,
                   const IndexedDBKeyRange& key_range,
                   scoped_refptr<IndexedDBCallbacks> callbacks);
  void Clear(int64_t transaction_id,
             int64_t object_store_id,
             scoped_refptr<IndexedDBCallbacks> callbacks);
  void CreateIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id,
                   const base::string16& name,
                   const IndexedDBKeyPath& key_path,
                   bool unique,
                   bool multi_entry);
  void DeleteIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id);
  void RenameIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id,
                   const base::string16& new_name);
  void Abort(int64_t transaction_id);
  void AbortWithError(int64_t transaction_id,
                      scoped_refptr<IndexedDBCallbacks> callbacks,
                      const IndexedDBDatabaseError& error);
  void Commit(int64_t transaction_id);
  void OnGotUsageAndQuotaForCommit(int64_t transaction_id,
                                   blink::mojom::QuotaStatusCode status,
                                   int64_t usage,
                                   int64_t quota);

 private:
  scoped_refptr<IndexedDBContextImpl> indexed_db_context_;
  std::unique_ptr<IndexedDBConnection> connection_;
  const url::Origin origin_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<IDBSequenceHelper> weak_factory_;
};

DatabaseImpl::DatabaseImpl(std::unique_ptr<IndexedDBConnection> connection,
                           const url::Origin& origin,
                           IndexedDBDispatcherHost* dispatcher_host,
                           scoped_refptr<base::SequencedTaskRunner> idb_runner)
    : dispatcher_host_(dispatcher_host),
      origin_(origin),
      idb_runner_(std::move(idb_runner)) {
  DCHECK(connection);
  helper_ = new IDBSequenceHelper(std::move(connection), origin,
                                  dispatcher_host->context());
  idb_runner_->PostTask(FROM_HERE,
                        base::BindOnce(&IDBSequenceHelper::ConnectionOpened,
                                       base::Unretained(helper_)));
}

DatabaseImpl::~DatabaseImpl() {
  idb_runner_->DeleteSoon(FROM_HERE, helper_);
}

void DatabaseImpl::CreateObjectStore(int64_t transaction_id,
                                     int64_t object_store_id,
                                     const base::string16& name,
                                     const IndexedDBKeyPath& key_path,
                                     bool auto_increment) {
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::CreateObjectStore,
                     base::Unretained(helper_), transaction_id, object_store_id,
                     name, key_path, auto_increment));
}

void DatabaseImpl::DeleteObjectStore(int64_t transaction_id,
                                     int64_t object_store_id) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::DeleteObjectStore,
                                base::Unretained(helper_), transaction_id,
                                object_store_id));
}

void DatabaseImpl::RenameObjectStore(int64_t transaction_id,
                                     int64_t object_store_id,
                                     const base::string16& new_name) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::RenameObjectStore,
                                base::Unretained(helper_), transaction_id,
                                object_store_id, new_name));
}

void DatabaseImpl::CreateTransaction(
    int64_t transaction_id,
    const std::vector<int64_t>& object_store_ids,
    blink::WebIDBTransactionMode mode) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::CreateTransaction,
                                base::Unretained(helper_), transaction_id,
                                object_store_ids, mode));
}

void DatabaseImpl::Close() {
  idb_runner_->PostTask(FROM_HERE, base::BindOnce(&IDBSequenceHelper::Close,
                                                  base::Unretained(helper_)));
}

void DatabaseImpl::VersionChangeIgnored() {
  idb_runner_->PostTask(FROM_HERE,
                        base::BindOnce(&IDBSequenceHelper::VersionChangeIgnored,
                                       base::Unretained(helper_)));
}

void DatabaseImpl::AddObserver(int64_t transaction_id,
                               int32_t observer_id,
                               bool include_transaction,
                               bool no_records,
                               bool values,
                               uint16_t operation_types) {
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::AddObserver, base::Unretained(helper_),
                     transaction_id, observer_id, include_transaction,
                     no_records, values, operation_types));
}

void DatabaseImpl::RemoveObservers(const std::vector<int32_t>& observers) {
  idb_runner_->PostTask(FROM_HERE,
                        base::BindOnce(&IDBSequenceHelper::RemoveObservers,
                                       base::Unretained(helper_), observers));
}

void DatabaseImpl::Get(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    bool key_only,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::Get, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, key_range,
                     key_only, std::move(callbacks)));
}

void DatabaseImpl::GetAll(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    bool key_only,
    int64_t max_count,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::GetAll, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, key_range,
                     key_only, max_count, std::move(callbacks)));
}

void DatabaseImpl::Put(
    int64_t transaction_id,
    int64_t object_store_id,
    ::indexed_db::mojom::ValuePtr value,
    const IndexedDBKey& key,
    blink::WebIDBPutMode mode,
    const std::vector<IndexedDBIndexKeys>& index_keys,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();

  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));

  std::vector<std::unique_ptr<storage::BlobDataHandle>> handles(
      value->blob_or_file_info.size());
  base::CheckedNumeric<uint64_t> total_blob_size = 0;
  std::vector<IndexedDBBlobInfo> blob_info(value->blob_or_file_info.size());
  for (size_t i = 0; i < value->blob_or_file_info.size(); ++i) {
    ::indexed_db::mojom::BlobInfoPtr& info = value->blob_or_file_info[i];

    std::unique_ptr<storage::BlobDataHandle> handle =
        dispatcher_host_->blob_storage_context()->GetBlobDataFromUUID(
            info->uuid);

    // Due to known issue crbug.com/351753, blobs can die while being passed to
    // a different process. So this case must be handled gracefully.
    // TODO(dmurph): Revert back to using mojo::ReportBadMessage once fixed.
    UMA_HISTOGRAM_BOOLEAN("Storage.IndexedDB.PutValidBlob",
                          handle.get() != nullptr);
    if (!handle) {
      IndexedDBDatabaseError error(blink::kWebIDBDatabaseExceptionUnknownError,
                                   kInvalidBlobUuid);
      idb_runner_->PostTask(
          FROM_HERE, base::BindOnce(&IDBSequenceHelper::AbortWithError,
                                    base::Unretained(helper_), transaction_id,
                                    std::move(callbacks), error));
      return;
    }
    uint64_t size = handle->size();
    UMA_HISTOGRAM_MEMORY_KB("Storage.IndexedDB.PutBlobSizeKB", size / 1024ull);
    total_blob_size += size;
    handles[i] = std::move(handle);

    if (info->file) {
      if (!info->file->path.empty() &&
          !policy->CanReadFile(dispatcher_host_->ipc_process_id(),
                               info->file->path)) {
        mojo::ReportBadMessage(kInvalidBlobFilePath);
        return;
      }
      blob_info[i] = IndexedDBBlobInfo(info->uuid, info->file->path,
                                       info->file->name, info->mime_type);
      if (info->size != -1) {
        blob_info[i].set_last_modified(info->file->last_modified);
        blob_info[i].set_size(info->size);
      }
    } else {
      blob_info[i] = IndexedDBBlobInfo(info->uuid, info->mime_type, info->size);
    }
  }
  UMA_HISTOGRAM_COUNTS_1000("WebCore.IndexedDB.PutBlobsCount",
                            blob_info.size());
  uint64_t blob_size = total_blob_size.ValueOrDefault(0U);
  if (blob_size != 0) {
    // 1KB to 1GB.
    UMA_HISTOGRAM_COUNTS_1M("WebCore.IndexedDB.PutBlobsTotalSize",
                            blob_size / 1024);
  }
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::Put, base::Unretained(helper_),
                     transaction_id, object_store_id, std::move(value),
                     std::move(handles), std::move(blob_info), key, mode,
                     index_keys, std::move(callbacks)));
}

void DatabaseImpl::SetIndexKeys(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKey& primary_key,
    const std::vector<IndexedDBIndexKeys>& index_keys) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::SetIndexKeys,
                                base::Unretained(helper_), transaction_id,
                                object_store_id, primary_key, index_keys));
}

void DatabaseImpl::SetIndexesReady(int64_t transaction_id,
                                   int64_t object_store_id,
                                   const std::vector<int64_t>& index_ids) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::SetIndexesReady,
                                base::Unretained(helper_), transaction_id,
                                object_store_id, index_ids));
}

void DatabaseImpl::OpenCursor(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    blink::WebIDBCursorDirection direction,
    bool key_only,
    blink::WebIDBTaskType task_type,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::OpenCursor, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, key_range,
                     direction, key_only, task_type, std::move(callbacks)));
}

void DatabaseImpl::Count(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::Count, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, key_range,
                     std::move(callbacks)));
}

void DatabaseImpl::DeleteRange(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKeyRange& key_range,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::DeleteRange, base::Unretained(helper_),
                     transaction_id, object_store_id, key_range,
                     std::move(callbacks)));
}

void DatabaseImpl::Clear(
    int64_t transaction_id,
    int64_t object_store_id,
    ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info) {
  scoped_refptr<IndexedDBCallbacks> callbacks(
      new IndexedDBCallbacks(dispatcher_host_->AsWeakPtr(), origin_,
                             std::move(callbacks_info), idb_runner_));
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::Clear, base::Unretained(helper_),
                     transaction_id, object_store_id, std::move(callbacks)));
}

void DatabaseImpl::CreateIndex(int64_t transaction_id,
                               int64_t object_store_id,
                               int64_t index_id,
                               const base::string16& name,
                               const IndexedDBKeyPath& key_path,
                               bool unique,
                               bool multi_entry) {
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::CreateIndex, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, name, key_path,
                     unique, multi_entry));
}

void DatabaseImpl::DeleteIndex(int64_t transaction_id,
                               int64_t object_store_id,
                               int64_t index_id) {
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::DeleteIndex, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id));
}

void DatabaseImpl::RenameIndex(int64_t transaction_id,
                               int64_t object_store_id,
                               int64_t index_id,
                               const base::string16& new_name) {
  idb_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IDBSequenceHelper::RenameIndex, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, new_name));
}

void DatabaseImpl::Abort(int64_t transaction_id) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::Abort,
                                base::Unretained(helper_), transaction_id));
}

void DatabaseImpl::Commit(int64_t transaction_id) {
  idb_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IDBSequenceHelper::Commit,
                                base::Unretained(helper_), transaction_id));
}

DatabaseImpl::IDBSequenceHelper::IDBSequenceHelper(
    std::unique_ptr<IndexedDBConnection> connection,
    const url::Origin& origin,
    scoped_refptr<IndexedDBContextImpl> indexed_db_context)
    : indexed_db_context_(indexed_db_context),
      connection_(std::move(connection)),
      origin_(origin),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

DatabaseImpl::IDBSequenceHelper::~IDBSequenceHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_->IsConnected())
    connection_->Close();
  indexed_db_context_->ConnectionClosed(origin_, connection_.get());
}

void DatabaseImpl::IDBSequenceHelper::ConnectionOpened() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  indexed_db_context_->ConnectionOpened(origin_, connection_.get());
}

void DatabaseImpl::IDBSequenceHelper::CreateObjectStore(
    int64_t transaction_id,
    int64_t object_store_id,
    const base::string16& name,
    const IndexedDBKeyPath& key_path,
    bool auto_increment) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->CreateObjectStore(transaction, object_store_id, name,
                                             key_path, auto_increment);
}

void DatabaseImpl::IDBSequenceHelper::DeleteObjectStore(
    int64_t transaction_id,
    int64_t object_store_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->DeleteObjectStore(transaction, object_store_id);
}

void DatabaseImpl::IDBSequenceHelper::RenameObjectStore(
    int64_t transaction_id,
    int64_t object_store_id,
    const base::string16& new_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->RenameObjectStore(transaction, object_store_id,
                                             new_name);
}

void DatabaseImpl::IDBSequenceHelper::CreateTransaction(
    int64_t transaction_id,
    const std::vector<int64_t>& object_store_ids,
    blink::WebIDBTransactionMode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  // Can't call BadMessage as we're no longer on the IO thread. So ignore.
  if (connection_->GetTransaction(transaction_id))
    return;

  connection_->database()->CreateTransaction(transaction_id, connection_.get(),
                                             object_store_ids, mode);
}

void DatabaseImpl::IDBSequenceHelper::Close() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  connection_->Close();
}

void DatabaseImpl::IDBSequenceHelper::VersionChangeIgnored() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  connection_->VersionChangeIgnored();
}

void DatabaseImpl::IDBSequenceHelper::AddObserver(int64_t transaction_id,
                                                  int32_t observer_id,
                                                  bool include_transaction,
                                                  bool no_records,
                                                  bool values,
                                                  uint16_t operation_types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  IndexedDBObserver::Options options(include_transaction, no_records, values,
                                     operation_types);
  connection_->database()->AddPendingObserver(transaction, observer_id,
                                              options);
}

void DatabaseImpl::IDBSequenceHelper::RemoveObservers(
    const std::vector<int32_t>& observers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  connection_->RemoveObservers(observers);
}

void DatabaseImpl::IDBSequenceHelper::Get(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    bool key_only,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->Get(transaction, object_store_id, index_id,
                               std::make_unique<IndexedDBKeyRange>(key_range),
                               key_only, callbacks);
}

void DatabaseImpl::IDBSequenceHelper::GetAll(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    bool key_only,
    int64_t max_count,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->GetAll(
      transaction, object_store_id, index_id,
      std::make_unique<IndexedDBKeyRange>(key_range), key_only, max_count,
      std::move(callbacks));
}

void DatabaseImpl::IDBSequenceHelper::Put(
    int64_t transaction_id,
    int64_t object_store_id,
    ::indexed_db::mojom::ValuePtr mojo_value,
    std::vector<std::unique_ptr<storage::BlobDataHandle>> handles,
    std::vector<IndexedDBBlobInfo> blob_info,
    const IndexedDBKey& key,
    blink::WebIDBPutMode mode,
    const std::vector<IndexedDBIndexKeys>& index_keys,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  // Value size recorded in IDBObjectStore before we can auto-wrap in a blob.
  // 1KB to 10MB.
  UMA_HISTOGRAM_COUNTS_10000("WebCore.IndexedDB.PutKeySize",
                             key.size_estimate() / 1024);

  uint64_t commit_size = mojo_value->bits.size() + key.size_estimate();
  IndexedDBValue value;
  swap(value.bits, mojo_value->bits);
  swap(value.blob_info, blob_info);
  connection_->database()->Put(transaction, object_store_id, &value, &handles,
                               std::make_unique<IndexedDBKey>(key), mode,
                               std::move(callbacks), index_keys);

  // Size can't be big enough to overflow because it represents the
  // actual bytes passed through IPC.
  transaction->set_size(transaction->size() + commit_size);
}

void DatabaseImpl::IDBSequenceHelper::SetIndexKeys(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKey& primary_key,
    const std::vector<IndexedDBIndexKeys>& index_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->SetIndexKeys(
      transaction, object_store_id, std::make_unique<IndexedDBKey>(primary_key),
      index_keys);
}

void DatabaseImpl::IDBSequenceHelper::SetIndexesReady(
    int64_t transaction_id,
    int64_t object_store_id,
    const std::vector<int64_t>& index_ids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->SetIndexesReady(transaction, object_store_id,
                                           index_ids);
}

void DatabaseImpl::IDBSequenceHelper::OpenCursor(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    blink::WebIDBCursorDirection direction,
    bool key_only,
    blink::WebIDBTaskType task_type,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->OpenCursor(
      transaction, object_store_id, index_id,
      std::make_unique<IndexedDBKeyRange>(key_range), direction, key_only,
      task_type, std::move(callbacks));
}

void DatabaseImpl::IDBSequenceHelper::Count(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->Count(transaction, object_store_id, index_id,
                                 std::make_unique<IndexedDBKeyRange>(key_range),
                                 std::move(callbacks));
}

void DatabaseImpl::IDBSequenceHelper::DeleteRange(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKeyRange& key_range,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->DeleteRange(
      transaction, object_store_id,
      std::make_unique<IndexedDBKeyRange>(key_range), std::move(callbacks));
}

void DatabaseImpl::IDBSequenceHelper::Clear(
    int64_t transaction_id,
    int64_t object_store_id,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->Clear(transaction, object_store_id, callbacks);
}

void DatabaseImpl::IDBSequenceHelper::CreateIndex(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const base::string16& name,
    const IndexedDBKeyPath& key_path,
    bool unique,
    bool multi_entry) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->CreateIndex(transaction, object_store_id, index_id,
                                       name, key_path, unique, multi_entry);
}

void DatabaseImpl::IDBSequenceHelper::DeleteIndex(int64_t transaction_id,
                                                  int64_t object_store_id,
                                                  int64_t index_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->DeleteIndex(transaction, object_store_id, index_id);
}

void DatabaseImpl::IDBSequenceHelper::RenameIndex(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const base::string16& new_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->database()->RenameIndex(transaction, object_store_id, index_id,
                                       new_name);
}

void DatabaseImpl::IDBSequenceHelper::Abort(int64_t transaction_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->AbortTransaction(
      transaction,
      IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionAbortError,
                             "Transaction aborted by user."));
}

void DatabaseImpl::IDBSequenceHelper::AbortWithError(
    int64_t transaction_id,
    scoped_refptr<IndexedDBCallbacks> callbacks,
    const IndexedDBDatabaseError& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  callbacks->OnError(error);

  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  connection_->AbortTransaction(transaction, error);
}

void DatabaseImpl::IDBSequenceHelper::Commit(int64_t transaction_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  // Always allow empty or delete-only transactions.
  if (transaction->size() == 0) {
    connection_->database()->Commit(transaction);
    return;
  }

  indexed_db_context_->quota_manager_proxy()->GetUsageAndQuota(
      indexed_db_context_->TaskRunner(), origin_,
      blink::mojom::StorageType::kTemporary,
      base::BindOnce(&IDBSequenceHelper::OnGotUsageAndQuotaForCommit,
                     weak_factory_.GetWeakPtr(), transaction_id));
}

void DatabaseImpl::IDBSequenceHelper::OnGotUsageAndQuotaForCommit(
    int64_t transaction_id,
    blink::mojom::QuotaStatusCode status,
    int64_t usage,
    int64_t quota) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // May have disconnected while quota check was pending.
  if (!connection_->IsConnected())
    return;

  IndexedDBTransaction* transaction =
      connection_->GetTransaction(transaction_id);
  if (!transaction)
    return;

  if (status == blink::mojom::QuotaStatusCode::kOk &&
      usage + transaction->size() <= quota) {
    connection_->database()->Commit(transaction);
  } else {
    connection_->AbortTransaction(
        transaction,
        IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionQuotaError));
  }
}

}  // namespace content
