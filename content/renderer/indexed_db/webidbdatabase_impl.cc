// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/webidbdatabase_impl.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "content/renderer/indexed_db/indexed_db_callbacks_impl.h"
#include "content/renderer/indexed_db/indexed_db_dispatcher.h"
#include "content/renderer/indexed_db/indexed_db_key_builders.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_error.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_exception.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key_path.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_metadata.h"
#include "third_party/blink/public/platform/web_blob_info.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

using blink::WebBlobInfo;
using blink::WebIDBCallbacks;
using blink::WebIDBDatabase;
using blink::WebIDBDatabaseCallbacks;
using blink::WebIDBMetadata;
using blink::WebIDBKey;
using blink::WebIDBKeyPath;
using blink::WebIDBKeyRange;
using blink::WebIDBKeyView;
using blink::WebString;
using blink::WebVector;
using indexed_db::mojom::CallbacksAssociatedPtrInfo;
using indexed_db::mojom::DatabaseAssociatedPtrInfo;

namespace content {

namespace {

std::vector<content::IndexedDBIndexKeys> ConvertWebIndexKeys(
    const WebVector<long long>& index_ids,
    const WebVector<WebIDBDatabase::WebIndexKeys>& index_keys) {
  DCHECK_EQ(index_ids.size(), index_keys.size());
  std::vector<content::IndexedDBIndexKeys> result;
  result.reserve(index_ids.size());
  for (size_t i = 0, len = index_ids.size(); i < len; ++i) {
    result.emplace_back(index_ids[i], std::vector<content::IndexedDBKey>());
    std::vector<content::IndexedDBKey>& result_keys = result.back().second;
    result_keys.reserve(index_keys[i].size());
    for (const WebIDBKey& index_key : index_keys[i])
      result_keys.emplace_back(IndexedDBKeyBuilder::Build(index_key.View()));
  }
  return result;
}

}  // namespace

class WebIDBDatabaseImpl::IOThreadHelper {
 public:
  explicit IOThreadHelper(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~IOThreadHelper();

  void Bind(DatabaseAssociatedPtrInfo database_info);
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
           std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void GetAll(int64_t transaction_id,
              int64_t object_store_id,
              int64_t index_id,
              const IndexedDBKeyRange& key_range,
              int64_t max_count,
              bool key_only,
              std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void Put(int64_t transaction_id,
           int64_t object_store_id,
           indexed_db::mojom::ValuePtr value,
           const IndexedDBKey& key,
           blink::WebIDBPutMode mode,
           std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
           const std::vector<content::IndexedDBIndexKeys>& index_keys);
  void SetIndexKeys(int64_t transaction_id,
                    int64_t object_store_id,
                    const IndexedDBKey& primary_key,
                    const std::vector<content::IndexedDBIndexKeys>& index_keys);
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
                  std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void Count(int64_t transaction_id,
             int64_t object_store_id,
             int64_t index_id,
             const IndexedDBKeyRange& key_range,
             std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void DeleteRange(int64_t transaction_id,
                   int64_t object_store_id,
                   const IndexedDBKeyRange& key_range,
                   std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void Clear(int64_t transaction_id,
             int64_t object_store_id,
             std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
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
  void Commit(int64_t transaction_id);

 private:
  CallbacksAssociatedPtrInfo GetCallbacksProxy(
      std::unique_ptr<IndexedDBCallbacksImpl> callbacks);

  indexed_db::mojom::DatabaseAssociatedPtr database_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadHelper);
};

WebIDBDatabaseImpl::WebIDBDatabaseImpl(
    DatabaseAssociatedPtrInfo database_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    scoped_refptr<base::SingleThreadTaskRunner> callback_runner)
    : helper_(new IOThreadHelper(io_runner)),
      io_runner_(std::move(io_runner)),
      callback_runner_(std::move(callback_runner)) {
  io_runner_->PostTask(FROM_HERE, base::BindOnce(&IOThreadHelper::Bind,
                                                 base::Unretained(helper_),
                                                 std::move(database_info)));
}

WebIDBDatabaseImpl::~WebIDBDatabaseImpl() {
  io_runner_->DeleteSoon(FROM_HERE, helper_);
}

void WebIDBDatabaseImpl::CreateObjectStore(long long transaction_id,
                                           long long object_store_id,
                                           const WebString& name,
                                           const WebIDBKeyPath& key_path,
                                           bool auto_increment) {
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::CreateObjectStore,
                     base::Unretained(helper_), transaction_id, object_store_id,
                     name.Utf16(), IndexedDBKeyPathBuilder::Build(key_path),
                     auto_increment));
}

void WebIDBDatabaseImpl::DeleteObjectStore(long long transaction_id,
                                           long long object_store_id) {
  io_runner_->PostTask(FROM_HERE,
                       base::BindOnce(&IOThreadHelper::DeleteObjectStore,
                                      base::Unretained(helper_), transaction_id,
                                      object_store_id));
}

void WebIDBDatabaseImpl::RenameObjectStore(long long transaction_id,
                                           long long object_store_id,
                                           const blink::WebString& new_name) {
  io_runner_->PostTask(FROM_HERE,
                       base::BindOnce(&IOThreadHelper::RenameObjectStore,
                                      base::Unretained(helper_), transaction_id,
                                      object_store_id, new_name.Utf16()));
}

void WebIDBDatabaseImpl::CreateTransaction(
    long long transaction_id,
    const WebVector<long long>& object_store_ids,
    blink::WebIDBTransactionMode mode) {
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::CreateTransaction,
                                base::Unretained(helper_), transaction_id,
                                std::vector<int64_t>(object_store_ids.begin(),
                                                     object_store_ids.end()),
                                mode));
}

void WebIDBDatabaseImpl::Close() {
  io_runner_->PostTask(FROM_HERE, base::BindOnce(&IOThreadHelper::Close,
                                                 base::Unretained(helper_)));
}

void WebIDBDatabaseImpl::VersionChangeIgnored() {
  io_runner_->PostTask(FROM_HERE,
                       base::BindOnce(&IOThreadHelper::VersionChangeIgnored,
                                      base::Unretained(helper_)));
}

void WebIDBDatabaseImpl::AddObserver(
    long long transaction_id,
    int32_t observer_id,
    bool include_transaction,
    bool no_records,
    bool values,
    const std::bitset<blink::kWebIDBOperationTypeCount>& operation_types) {
  static_assert(blink::kWebIDBOperationTypeCount < sizeof(uint16_t) * CHAR_BIT,
                "WebIDBOperationType Count exceeds size of uint16_t");
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::AddObserver, base::Unretained(helper_),
                     transaction_id, observer_id, include_transaction,
                     no_records, values, operation_types.to_ulong()));
}

void WebIDBDatabaseImpl::RemoveObservers(
    const WebVector<int32_t>& observer_ids_to_remove) {
  std::vector<int32_t> remove_observer_ids(
      observer_ids_to_remove.Data(),
      observer_ids_to_remove.Data() + observer_ids_to_remove.size());

  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::RemoveObservers,
                     base::Unretained(helper_), remove_observer_ids));
}

void WebIDBDatabaseImpl::Get(long long transaction_id,
                             long long object_store_id,
                             long long index_id,
                             const WebIDBKeyRange& key_range,
                             bool key_only,
                             WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::Get, base::Unretained(helper_),
                                transaction_id, object_store_id, index_id,
                                IndexedDBKeyRangeBuilder::Build(key_range),
                                key_only, std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::GetAll(long long transaction_id,
                                long long object_store_id,
                                long long index_id,
                                const WebIDBKeyRange& key_range,
                                long long max_count,
                                bool key_only,
                                WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::GetAll, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id,
                     IndexedDBKeyRangeBuilder::Build(key_range), max_count,
                     key_only, std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::Put(long long transaction_id,
                             long long object_store_id,
                             const blink::WebData& value,
                             const WebVector<WebBlobInfo>& web_blob_info,
                             WebIDBKeyView web_primary_key,
                             blink::WebIDBPutMode put_mode,
                             WebIDBCallbacks* callbacks,
                             const WebVector<long long>& index_ids,
                             WebVector<WebIndexKeys> index_keys) {
  IndexedDBKey key = IndexedDBKeyBuilder::Build(web_primary_key);

  if (value.size() + key.size_estimate() > max_put_value_size_) {
    callbacks->OnError(blink::WebIDBDatabaseError(
        blink::kWebIDBDatabaseExceptionUnknownError,
        WebString::FromUTF8(base::StringPrintf(
            "The serialized value is too large"
            " (size=%" PRIuS " bytes, max=%" PRIuS " bytes).",
            value.size(), max_put_value_size_))));
    return;
  }

  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto mojo_value = indexed_db::mojom::Value::New();
  DCHECK(mojo_value->bits.empty());
  mojo_value->bits.reserve(value.size());
  value.ForEachSegment([&mojo_value](const char* segment, size_t segment_size,
                                     size_t segment_offset) {
    mojo_value->bits.append(segment, segment_size);
    return true;
  });
  mojo_value->blob_or_file_info.reserve(web_blob_info.size());
  for (const WebBlobInfo& info : web_blob_info) {
    auto blob_info = indexed_db::mojom::BlobInfo::New();
    if (info.IsFile()) {
      blob_info->file = indexed_db::mojom::FileInfo::New();
      blob_info->file->path = blink::WebStringToFilePath(info.FilePath());
      blob_info->file->name = info.FileName().Utf16();
      blob_info->file->last_modified =
          base::Time::FromDoubleT(info.LastModified());
    }
    blob_info->size = info.size();
    blob_info->uuid = info.Uuid().Latin1();
    DCHECK(blob_info->uuid.size());
    blob_info->mime_type = info.GetType().Utf16();
    blob_info->blob = blink::mojom::BlobPtrInfo(info.CloneBlobHandle(),
                                                blink::mojom::Blob::Version_);
    mojo_value->blob_or_file_info.push_back(std::move(blob_info));
  }

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::Put, base::Unretained(helper_),
                     transaction_id, object_store_id, std::move(mojo_value),
                     key, put_mode, std::move(callbacks_impl),
                     ConvertWebIndexKeys(index_ids, index_keys)));
}

void WebIDBDatabaseImpl::SetIndexKeys(
    long long transaction_id,
    long long object_store_id,
    WebIDBKeyView primary_key,
    const WebVector<long long>& index_ids,
    const WebVector<WebIndexKeys>& index_keys) {
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::SetIndexKeys, base::Unretained(helper_),
                     transaction_id, object_store_id,
                     IndexedDBKeyBuilder::Build(primary_key),
                     ConvertWebIndexKeys(index_ids, index_keys)));
}

void WebIDBDatabaseImpl::SetIndexesReady(
    long long transaction_id,
    long long object_store_id,
    const WebVector<long long>& web_index_ids) {
  std::vector<int64_t> index_ids(web_index_ids.Data(),
                                 web_index_ids.Data() + web_index_ids.size());
  io_runner_->PostTask(FROM_HERE,
                       base::BindOnce(&IOThreadHelper::SetIndexesReady,
                                      base::Unretained(helper_), transaction_id,
                                      object_store_id, std::move(index_ids)));
}

void WebIDBDatabaseImpl::OpenCursor(long long transaction_id,
                                    long long object_store_id,
                                    long long index_id,
                                    const WebIDBKeyRange& key_range,
                                    blink::WebIDBCursorDirection direction,
                                    bool key_only,
                                    blink::WebIDBTaskType task_type,
                                    WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::OpenCursor, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id,
                     IndexedDBKeyRangeBuilder::Build(key_range), direction,
                     key_only, task_type, std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::Count(long long transaction_id,
                               long long object_store_id,
                               long long index_id,
                               const WebIDBKeyRange& key_range,
                               WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::Count, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id,
                     IndexedDBKeyRangeBuilder::Build(key_range),
                     std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::Delete(long long transaction_id,
                                long long object_store_id,
                                WebIDBKeyView primary_key,
                                WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::DeleteRange, base::Unretained(helper_),
                     transaction_id, object_store_id,
                     IndexedDBKeyRangeBuilder::Build(primary_key),
                     std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::DeleteRange(long long transaction_id,
                                     long long object_store_id,
                                     const WebIDBKeyRange& key_range,
                                     WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::DeleteRange, base::Unretained(helper_),
                     transaction_id, object_store_id,
                     IndexedDBKeyRangeBuilder::Build(key_range),
                     std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::Clear(long long transaction_id,
                               long long object_store_id,
                               WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id, nullptr);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      callback_runner_);
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::Clear,
                                base::Unretained(helper_), transaction_id,
                                object_store_id, std::move(callbacks_impl)));
}

void WebIDBDatabaseImpl::CreateIndex(long long transaction_id,
                                     long long object_store_id,
                                     long long index_id,
                                     const WebString& name,
                                     const WebIDBKeyPath& key_path,
                                     bool unique,
                                     bool multi_entry) {
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::CreateIndex, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id, name.Utf16(),
                     IndexedDBKeyPathBuilder::Build(key_path), unique,
                     multi_entry));
}

void WebIDBDatabaseImpl::DeleteIndex(long long transaction_id,
                                     long long object_store_id,
                                     long long index_id) {
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::DeleteIndex, base::Unretained(helper_),
                     transaction_id, object_store_id, index_id));
}

void WebIDBDatabaseImpl::RenameIndex(long long transaction_id,
                                     long long object_store_id,
                                     long long index_id,
                                     const WebString& new_name) {
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::RenameIndex,
                                base::Unretained(helper_), transaction_id,
                                object_store_id, index_id, new_name.Utf16()));
}

void WebIDBDatabaseImpl::Abort(long long transaction_id) {
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::Abort,
                                base::Unretained(helper_), transaction_id));
}

void WebIDBDatabaseImpl::Commit(long long transaction_id) {
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::Commit,
                                base::Unretained(helper_), transaction_id));
}

WebIDBDatabaseImpl::IOThreadHelper::IOThreadHelper(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

WebIDBDatabaseImpl::IOThreadHelper::~IOThreadHelper() {}

void WebIDBDatabaseImpl::IOThreadHelper::Bind(
    DatabaseAssociatedPtrInfo database_info) {
  database_.Bind(std::move(database_info), task_runner_);
}

void WebIDBDatabaseImpl::IOThreadHelper::CreateObjectStore(
    int64_t transaction_id,
    int64_t object_store_id,
    const base::string16& name,
    const IndexedDBKeyPath& key_path,
    bool auto_increment) {
  database_->CreateObjectStore(transaction_id, object_store_id, name, key_path,
                               auto_increment);
}

void WebIDBDatabaseImpl::IOThreadHelper::DeleteObjectStore(
    int64_t transaction_id,
    int64_t object_store_id) {
  database_->DeleteObjectStore(transaction_id, object_store_id);
}

void WebIDBDatabaseImpl::IOThreadHelper::RenameObjectStore(
    int64_t transaction_id,
    int64_t object_store_id,
    const base::string16& new_name) {
  database_->RenameObjectStore(transaction_id, object_store_id, new_name);
}

void WebIDBDatabaseImpl::IOThreadHelper::CreateTransaction(
    int64_t transaction_id,
    const std::vector<int64_t>& object_store_ids,
    blink::WebIDBTransactionMode mode) {
  database_->CreateTransaction(transaction_id, object_store_ids, mode);
}

void WebIDBDatabaseImpl::IOThreadHelper::Close() {
  database_->Close();
}

void WebIDBDatabaseImpl::IOThreadHelper::VersionChangeIgnored() {
  database_->VersionChangeIgnored();
}

void WebIDBDatabaseImpl::IOThreadHelper::AddObserver(int64_t transaction_id,
                                                     int32_t observer_id,
                                                     bool include_transaction,
                                                     bool no_records,
                                                     bool values,
                                                     uint16_t operation_types) {
  database_->AddObserver(transaction_id, observer_id, include_transaction,
                         no_records, values, operation_types);
}

void WebIDBDatabaseImpl::IOThreadHelper::RemoveObservers(
    const std::vector<int32_t>& observers) {
  database_->RemoveObservers(observers);
}

void WebIDBDatabaseImpl::IOThreadHelper::Get(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    bool key_only,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->Get(transaction_id, object_store_id, index_id, key_range, key_only,
                 GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::GetAll(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    int64_t max_count,
    bool key_only,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->GetAll(transaction_id, object_store_id, index_id, key_range,
                    key_only, max_count,
                    GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::Put(
    int64_t transaction_id,
    int64_t object_store_id,
    indexed_db::mojom::ValuePtr value,
    const IndexedDBKey& key,
    blink::WebIDBPutMode mode,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
    const std::vector<content::IndexedDBIndexKeys>& index_keys) {
  database_->Put(transaction_id, object_store_id, std::move(value), key, mode,
                 index_keys, GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::SetIndexKeys(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKey& primary_key,
    const std::vector<content::IndexedDBIndexKeys>& index_keys) {
  database_->SetIndexKeys(transaction_id, object_store_id, primary_key,
                          index_keys);
}

void WebIDBDatabaseImpl::IOThreadHelper::SetIndexesReady(
    int64_t transaction_id,
    int64_t object_store_id,
    const std::vector<int64_t>& index_ids) {
  database_->SetIndexesReady(transaction_id, object_store_id, index_ids);
}

void WebIDBDatabaseImpl::IOThreadHelper::OpenCursor(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    blink::WebIDBCursorDirection direction,
    bool key_only,
    blink::WebIDBTaskType task_type,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->OpenCursor(transaction_id, object_store_id, index_id, key_range,
                        direction, key_only, task_type,
                        GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::Count(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const IndexedDBKeyRange& key_range,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->Count(transaction_id, object_store_id, index_id, key_range,
                   GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::DeleteRange(
    int64_t transaction_id,
    int64_t object_store_id,
    const IndexedDBKeyRange& key_range,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->DeleteRange(transaction_id, object_store_id, key_range,
                         GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::Clear(
    int64_t transaction_id,
    int64_t object_store_id,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  database_->Clear(transaction_id, object_store_id,
                   GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBDatabaseImpl::IOThreadHelper::CreateIndex(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const base::string16& name,
    const IndexedDBKeyPath& key_path,
    bool unique,
    bool multi_entry) {
  database_->CreateIndex(transaction_id, object_store_id, index_id, name,
                         key_path, unique, multi_entry);
}

void WebIDBDatabaseImpl::IOThreadHelper::DeleteIndex(int64_t transaction_id,
                                                     int64_t object_store_id,
                                                     int64_t index_id) {
  database_->DeleteIndex(transaction_id, object_store_id, index_id);
}

void WebIDBDatabaseImpl::IOThreadHelper::RenameIndex(
    int64_t transaction_id,
    int64_t object_store_id,
    int64_t index_id,
    const base::string16& new_name) {
  database_->RenameIndex(transaction_id, object_store_id, index_id, new_name);
}

void WebIDBDatabaseImpl::IOThreadHelper::Abort(int64_t transaction_id) {
  database_->Abort(transaction_id);
}

void WebIDBDatabaseImpl::IOThreadHelper::Commit(int64_t transaction_id) {
  database_->Commit(transaction_id);
}

CallbacksAssociatedPtrInfo
WebIDBDatabaseImpl::IOThreadHelper::GetCallbacksProxy(
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  CallbacksAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  mojo::MakeStrongAssociatedBinding(std::move(callbacks), std::move(request));
  return ptr_info;
}

}  // namespace content
