// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_script_cached_metadata_handler.h"

#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_client.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"

namespace blink {

ServiceWorkerScriptCachedMetadataHandler::
    ServiceWorkerScriptCachedMetadataHandler(
        WorkerGlobalScope* worker_global_scope,
        const KURL& script_url,
        const Vector<char>* meta_data)
    : worker_global_scope_(worker_global_scope), script_url_(script_url) {
  if (meta_data)
    cached_metadata_ = CachedMetadata::CreateFromSerializedData(
        meta_data->data(), meta_data->size());
}

ServiceWorkerScriptCachedMetadataHandler::
    ~ServiceWorkerScriptCachedMetadataHandler() = default;

void ServiceWorkerScriptCachedMetadataHandler::Trace(blink::Visitor* visitor) {
  visitor->Trace(worker_global_scope_);
  CachedMetadataHandler::Trace(visitor);
}

void ServiceWorkerScriptCachedMetadataHandler::SetCachedMetadata(
    uint32_t data_type_id,
    const char* data,
    size_t size,
    CacheType type) {
  if (type != kSendToPlatform)
    return;
  cached_metadata_ = CachedMetadata::Create(data_type_id, data, size);
  const Vector<char>& serialized_data = cached_metadata_->SerializedData();
  ServiceWorkerGlobalScopeClient::From(worker_global_scope_)
      ->SetCachedMetadata(script_url_, serialized_data.data(),
                          serialized_data.size());
}

void ServiceWorkerScriptCachedMetadataHandler::ClearCachedMetadata(
    CacheType type) {
  if (type != kSendToPlatform)
    return;
  cached_metadata_ = nullptr;
  ServiceWorkerGlobalScopeClient::From(worker_global_scope_)
      ->ClearCachedMetadata(script_url_);
}

scoped_refptr<CachedMetadata>
ServiceWorkerScriptCachedMetadataHandler::GetCachedMetadata(
    uint32_t data_type_id) const {
  if (!cached_metadata_ || cached_metadata_->DataTypeID() != data_type_id)
    return nullptr;
  return cached_metadata_;
}

String ServiceWorkerScriptCachedMetadataHandler::Encoding() const {
  return g_empty_string;
}

bool ServiceWorkerScriptCachedMetadataHandler::IsServedFromCacheStorage()
    const {
  return false;
}

}  // namespace blink
