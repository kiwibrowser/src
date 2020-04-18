// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/indexed_db_callbacks_impl.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/common/indexed_db/indexed_db_constants.h"
#include "content/renderer/indexed_db/indexed_db_dispatcher.h"
#include "content/renderer/indexed_db/indexed_db_key_builders.h"
#include "content/renderer/indexed_db/webidbcursor_impl.h"
#include "content/renderer/indexed_db/webidbdatabase_impl.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_callbacks.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_error.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_metadata.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_value.h"

using blink::WebBlobInfo;
using blink::WebData;
using blink::WebIDBCallbacks;
using blink::WebIDBDatabase;
using blink::WebIDBMetadata;
using blink::WebIDBValue;
using blink::WebString;
using blink::WebVector;
using indexed_db::mojom::DatabaseAssociatedPtrInfo;

namespace content {

namespace {

void ConvertIndexMetadata(const content::IndexedDBIndexMetadata& metadata,
                          WebIDBMetadata::Index* output) {
  output->id = metadata.id;
  output->name = WebString::FromUTF16(metadata.name);
  output->key_path = WebIDBKeyPathBuilder::Build(metadata.key_path);
  output->unique = metadata.unique;
  output->multi_entry = metadata.multi_entry;
}

void ConvertObjectStoreMetadata(
    const content::IndexedDBObjectStoreMetadata& metadata,
    WebIDBMetadata::ObjectStore* output) {
  output->id = metadata.id;
  output->name = WebString::FromUTF16(metadata.name);
  output->key_path = WebIDBKeyPathBuilder::Build(metadata.key_path);
  output->auto_increment = metadata.auto_increment;
  output->max_index_id = metadata.max_index_id;
  output->indexes = WebVector<WebIDBMetadata::Index>(metadata.indexes.size());
  size_t i = 0;
  for (const auto& iter : metadata.indexes)
    ConvertIndexMetadata(iter.second, &output->indexes[i++]);
}

void ConvertDatabaseMetadata(const content::IndexedDBDatabaseMetadata& metadata,
                             WebIDBMetadata* output) {
  output->id = metadata.id;
  output->name = WebString::FromUTF16(metadata.name);
  output->version = metadata.version;
  output->max_object_store_id = metadata.max_object_store_id;
  output->object_stores =
      WebVector<WebIDBMetadata::ObjectStore>(metadata.object_stores.size());
  size_t i = 0;
  for (const auto& iter : metadata.object_stores)
    ConvertObjectStoreMetadata(iter.second, &output->object_stores[i++]);
}

WebIDBValue ConvertReturnValue(const indexed_db::mojom::ReturnValuePtr& value) {
  if (!value)
    return WebIDBValue(WebData(), WebVector<WebBlobInfo>());

  WebIDBValue web_value = IndexedDBCallbacksImpl::ConvertValue(value->value);
  web_value.SetInjectedPrimaryKey(WebIDBKeyBuilder::Build(value->primary_key),
                                  WebIDBKeyPathBuilder::Build(value->key_path));
  return web_value;
}

}  // namespace

// static
WebIDBValue IndexedDBCallbacksImpl::ConvertValue(
    const indexed_db::mojom::ValuePtr& value) {
  if (!value || value->bits.empty())
    return WebIDBValue(WebData(), WebVector<WebBlobInfo>());

  WebVector<WebBlobInfo> local_blob_info;
  local_blob_info.reserve(value->blob_or_file_info.size());
  for (size_t i = 0; i < value->blob_or_file_info.size(); ++i) {
    const auto& info = value->blob_or_file_info[i];
    if (info->file) {
      local_blob_info.emplace_back(WebString::FromUTF8(info->uuid),
                                   blink::FilePathToWebString(info->file->path),
                                   WebString::FromUTF16(info->file->name),
                                   WebString::FromUTF16(info->mime_type),
                                   info->file->last_modified.ToDoubleT(),
                                   info->size, info->blob.PassHandle());
    } else {
      local_blob_info.emplace_back(WebString::FromUTF8(info->uuid),
                                   WebString::FromUTF16(info->mime_type),
                                   info->size, info->blob.PassHandle());
    }
  }

  return WebIDBValue(WebData(&*value->bits.begin(), value->bits.size()),
                     std::move(local_blob_info));
}

IndexedDBCallbacksImpl::IndexedDBCallbacksImpl(
    std::unique_ptr<WebIDBCallbacks> callbacks,
    int64_t transaction_id,
    const base::WeakPtr<WebIDBCursorImpl>& cursor,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    scoped_refptr<base::SingleThreadTaskRunner> callback_runner)
    : internal_state_(new InternalState(std::move(callbacks),
                                        transaction_id,
                                        cursor,
                                        std::move(io_runner),
                                        callback_runner)),
      callback_runner_(std::move(callback_runner)) {}

IndexedDBCallbacksImpl::~IndexedDBCallbacksImpl() {
  callback_runner_->DeleteSoon(FROM_HERE, internal_state_);
}

void IndexedDBCallbacksImpl::Error(int32_t code,
                                   const base::string16& message) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::Error, base::Unretained(internal_state_),
                     code, message));
}

void IndexedDBCallbacksImpl::SuccessStringList(
    const std::vector<base::string16>& value) {
  callback_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InternalState::SuccessStringList,
                                base::Unretained(internal_state_), value));
}

void IndexedDBCallbacksImpl::Blocked(int64_t existing_version) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::Blocked, base::Unretained(internal_state_),
                     existing_version));
}

void IndexedDBCallbacksImpl::UpgradeNeeded(
    DatabaseAssociatedPtrInfo database,
    int64_t old_version,
    blink::WebIDBDataLoss data_loss,
    const std::string& data_loss_message,
    const content::IndexedDBDatabaseMetadata& metadata) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::UpgradeNeeded,
                     base::Unretained(internal_state_), std::move(database),
                     old_version, data_loss, data_loss_message, metadata));
}

void IndexedDBCallbacksImpl::SuccessDatabase(
    DatabaseAssociatedPtrInfo database,
    const content::IndexedDBDatabaseMetadata& metadata) {
  callback_runner_->PostTask(FROM_HERE,
                             base::BindOnce(&InternalState::SuccessDatabase,
                                            base::Unretained(internal_state_),
                                            std::move(database), metadata));
}

void IndexedDBCallbacksImpl::SuccessCursor(
    indexed_db::mojom::CursorAssociatedPtrInfo cursor,
    const IndexedDBKey& key,
    const IndexedDBKey& primary_key,
    indexed_db::mojom::ValuePtr value) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::SuccessCursor,
                     base::Unretained(internal_state_), std::move(cursor), key,
                     primary_key, std::move(value)));
}

void IndexedDBCallbacksImpl::SuccessValue(
    indexed_db::mojom::ReturnValuePtr value) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::SuccessValue,
                     base::Unretained(internal_state_), std::move(value)));
}

void IndexedDBCallbacksImpl::SuccessCursorContinue(
    const IndexedDBKey& key,
    const IndexedDBKey& primary_key,
    indexed_db::mojom::ValuePtr value) {
  callback_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InternalState::SuccessCursorContinue,
                                base::Unretained(internal_state_), key,
                                primary_key, std::move(value)));
}

void IndexedDBCallbacksImpl::SuccessCursorPrefetch(
    const std::vector<IndexedDBKey>& keys,
    const std::vector<IndexedDBKey>& primary_keys,
    std::vector<indexed_db::mojom::ValuePtr> values) {
  callback_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InternalState::SuccessCursorPrefetch,
                                base::Unretained(internal_state_), keys,
                                primary_keys, std::move(values)));
}

void IndexedDBCallbacksImpl::SuccessArray(
    std::vector<indexed_db::mojom::ReturnValuePtr> values) {
  callback_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&InternalState::SuccessArray,
                     base::Unretained(internal_state_), std::move(values)));
}

void IndexedDBCallbacksImpl::SuccessKey(const IndexedDBKey& key) {
  callback_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InternalState::SuccessKey,
                                base::Unretained(internal_state_), key));
}

void IndexedDBCallbacksImpl::SuccessInteger(int64_t value) {
  callback_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InternalState::SuccessInteger,
                                base::Unretained(internal_state_), value));
}

void IndexedDBCallbacksImpl::Success() {
  callback_runner_->PostTask(FROM_HERE,
                             base::BindOnce(&InternalState::Success,
                                            base::Unretained(internal_state_)));
}

IndexedDBCallbacksImpl::InternalState::InternalState(
    std::unique_ptr<blink::WebIDBCallbacks> callbacks,
    int64_t transaction_id,
    const base::WeakPtr<WebIDBCursorImpl>& cursor,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    scoped_refptr<base::SingleThreadTaskRunner> callback_runner)
    : callbacks_(std::move(callbacks)),
      transaction_id_(transaction_id),
      cursor_(cursor),
      io_runner_(std::move(io_runner)),
      callback_runner_(std::move(callback_runner)) {
  IndexedDBDispatcher::ThreadSpecificInstance()->RegisterMojoOwnedCallbacks(
      this);
}

IndexedDBCallbacksImpl::InternalState::~InternalState() {
  IndexedDBDispatcher::ThreadSpecificInstance()->UnregisterMojoOwnedCallbacks(
      this);
}

void IndexedDBCallbacksImpl::InternalState::Error(
    int32_t code,
    const base::string16& message) {
  callbacks_->OnError(
      blink::WebIDBDatabaseError(code, WebString::FromUTF16(message)));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessStringList(
    const std::vector<base::string16>& value) {
  WebVector<WebString> web_value(value.size());
  std::transform(
      value.begin(), value.end(), web_value.begin(),
      [](const base::string16& s) { return WebString::FromUTF16(s); });
  callbacks_->OnSuccess(web_value);
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::Blocked(int64_t existing_version) {
  callbacks_->OnBlocked(existing_version);
  // Not resetting |callbacks_|.
}

void IndexedDBCallbacksImpl::InternalState::UpgradeNeeded(
    DatabaseAssociatedPtrInfo database_info,
    int64_t old_version,
    blink::WebIDBDataLoss data_loss,
    const std::string& data_loss_message,
    const content::IndexedDBDatabaseMetadata& metadata) {
  WebIDBDatabase* database = new WebIDBDatabaseImpl(
      std::move(database_info), io_runner_, callback_runner_);
  WebIDBMetadata web_metadata;
  ConvertDatabaseMetadata(metadata, &web_metadata);
  callbacks_->OnUpgradeNeeded(old_version, database, web_metadata, data_loss,
                              WebString::FromUTF8(data_loss_message));
  // Not resetting |callbacks_|.
}

void IndexedDBCallbacksImpl::InternalState::SuccessDatabase(
    DatabaseAssociatedPtrInfo database_info,
    const content::IndexedDBDatabaseMetadata& metadata) {
  WebIDBDatabase* database = nullptr;
  if (database_info.is_valid()) {
    database = new WebIDBDatabaseImpl(std::move(database_info), io_runner_,
                                      callback_runner_);
  }

  WebIDBMetadata web_metadata;
  ConvertDatabaseMetadata(metadata, &web_metadata);
  callbacks_->OnSuccess(database, web_metadata);
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessCursor(
    indexed_db::mojom::CursorAssociatedPtrInfo cursor_info,
    const IndexedDBKey& key,
    const IndexedDBKey& primary_key,
    indexed_db::mojom::ValuePtr value) {
  WebIDBCursorImpl* cursor = new WebIDBCursorImpl(
      std::move(cursor_info), transaction_id_, io_runner_, callback_runner_);
  callbacks_->OnSuccess(cursor, WebIDBKeyBuilder::Build(key),
                        WebIDBKeyBuilder::Build(primary_key),
                        ConvertValue(value));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessKey(
    const IndexedDBKey& key) {
  callbacks_->OnSuccess(WebIDBKeyBuilder::Build(key));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessValue(
    indexed_db::mojom::ReturnValuePtr value) {
  callbacks_->OnSuccess(ConvertReturnValue(value));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessCursorContinue(
    const IndexedDBKey& key,
    const IndexedDBKey& primary_key,
    indexed_db::mojom::ValuePtr value) {
  callbacks_->OnSuccess(WebIDBKeyBuilder::Build(key),
                        WebIDBKeyBuilder::Build(primary_key),
                        ConvertValue(value));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessCursorPrefetch(
    const std::vector<IndexedDBKey>& keys,
    const std::vector<IndexedDBKey>& primary_keys,
    std::vector<indexed_db::mojom::ValuePtr> values) {
  std::vector<WebIDBValue> web_values;
  web_values.reserve(values.size());
  for (const indexed_db::mojom::ValuePtr& value : values)
    web_values.emplace_back(ConvertValue(value));

  if (cursor_) {
    cursor_->SetPrefetchData(keys, primary_keys, std::move(web_values));
    cursor_->CachedContinue(callbacks_.get());
  }
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessArray(
    std::vector<indexed_db::mojom::ReturnValuePtr> values) {
  WebVector<WebIDBValue> web_values;
  web_values.reserve(values.size());
  for (const indexed_db::mojom::ReturnValuePtr& value : values)
    web_values.emplace_back(ConvertReturnValue(value));
  callbacks_->OnSuccess(std::move(web_values));
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::SuccessInteger(int64_t value) {
  callbacks_->OnSuccess(value);
  callbacks_.reset();
}

void IndexedDBCallbacksImpl::InternalState::Success() {
  callbacks_->OnSuccess();
  callbacks_.reset();
}

}  // namespace content
