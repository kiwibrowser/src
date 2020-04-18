/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/indexeddb/idb_open_db_request.h"

#include <memory>
#include "base/optional.h"
#include "third_party/blink/renderer/bindings/modules/v8/idb_object_store_or_idb_index_or_idb_cursor.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_database.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_database_callbacks.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_tracing.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_version_change_event.h"

using blink::WebIDBDatabase;

namespace blink {

IDBOpenDBRequest* IDBOpenDBRequest::Create(
    ScriptState* script_state,
    IDBDatabaseCallbacks* callbacks,
    int64_t transaction_id,
    int64_t version,
    IDBRequest::AsyncTraceState metrics) {
  IDBOpenDBRequest* request = new IDBOpenDBRequest(
      script_state, callbacks, transaction_id, version, std::move(metrics));
  request->PauseIfNeeded();
  return request;
}

IDBOpenDBRequest::IDBOpenDBRequest(ScriptState* script_state,
                                   IDBDatabaseCallbacks* callbacks,
                                   int64_t transaction_id,
                                   int64_t version,
                                   IDBRequest::AsyncTraceState metrics)
    : IDBRequest(script_state,
                 IDBRequest::Source(),
                 nullptr,
                 std::move(metrics)),
      database_callbacks_(callbacks),
      transaction_id_(transaction_id),
      version_(version) {
  DCHECK(!ResultAsAny());
}

IDBOpenDBRequest::~IDBOpenDBRequest() = default;

void IDBOpenDBRequest::Trace(blink::Visitor* visitor) {
  visitor->Trace(database_callbacks_);
  IDBRequest::Trace(visitor);
}

void IDBOpenDBRequest::ContextDestroyed(ExecutionContext* destroyed_context) {
  IDBRequest::ContextDestroyed(destroyed_context);
  if (database_callbacks_)
    database_callbacks_->DetachWebCallbacks();
}

const AtomicString& IDBOpenDBRequest::InterfaceName() const {
  return EventTargetNames::IDBOpenDBRequest;
}

void IDBOpenDBRequest::EnqueueBlocked(int64_t old_version) {
  IDB_TRACE("IDBOpenDBRequest::onBlocked()");
  if (!ShouldEnqueueEvent())
    return;
  base::Optional<unsigned long long> new_version_nullable;
  if (version_ != IDBDatabaseMetadata::kDefaultVersion) {
    new_version_nullable = version_;
  }
  EnqueueEvent(IDBVersionChangeEvent::Create(
      EventTypeNames::blocked, old_version, new_version_nullable));
}

void IDBOpenDBRequest::EnqueueUpgradeNeeded(
    int64_t old_version,
    std::unique_ptr<WebIDBDatabase> backend,
    const IDBDatabaseMetadata& metadata,
    WebIDBDataLoss data_loss,
    String data_loss_message) {
  IDB_TRACE("IDBOpenDBRequest::onUpgradeNeeded()");
  if (!ShouldEnqueueEvent()) {
    metrics_.RecordAndReset();
    return;
  }

  DCHECK(database_callbacks_);

  IDBDatabase* idb_database =
      IDBDatabase::Create(GetExecutionContext(), std::move(backend),
                          database_callbacks_.Release(), isolate_);
  idb_database->SetMetadata(metadata);

  if (old_version == IDBDatabaseMetadata::kNoVersion) {
    // This database hasn't had a version before.
    old_version = IDBDatabaseMetadata::kDefaultVersion;
  }
  IDBDatabaseMetadata old_database_metadata(
      metadata.name, metadata.id, old_version, metadata.max_object_store_id);

  transaction_ = IDBTransaction::CreateVersionChange(
      GetExecutionContext(), transaction_id_, idb_database, this,
      old_database_metadata);
  SetResult(IDBAny::Create(idb_database));

  if (version_ == IDBDatabaseMetadata::kNoVersion)
    version_ = 1;
  EnqueueEvent(IDBVersionChangeEvent::Create(EventTypeNames::upgradeneeded,
                                             old_version, version_, data_loss,
                                             data_loss_message));
}

void IDBOpenDBRequest::EnqueueResponse(std::unique_ptr<WebIDBDatabase> backend,
                                       const IDBDatabaseMetadata& metadata) {
  IDB_TRACE("IDBOpenDBRequest::onSuccess()");
  if (!ShouldEnqueueEvent()) {
    metrics_.RecordAndReset();
    return;
  }

  IDBDatabase* idb_database = nullptr;
  if (ResultAsAny()) {
    // Previous OnUpgradeNeeded call delivered the backend.
    DCHECK(!backend.get());
    idb_database = ResultAsAny()->IdbDatabase();
    DCHECK(idb_database);
    DCHECK(!database_callbacks_);
  } else {
    DCHECK(backend.get());
    DCHECK(database_callbacks_);
    idb_database =
        IDBDatabase::Create(GetExecutionContext(), std::move(backend),
                            database_callbacks_.Release(), isolate_);
    SetResult(IDBAny::Create(idb_database));
  }
  idb_database->SetMetadata(metadata);
  EnqueueEvent(Event::Create(EventTypeNames::success));
  metrics_.RecordAndReset();
}

void IDBOpenDBRequest::EnqueueResponse(int64_t old_version) {
  IDB_TRACE("IDBOpenDBRequest::onSuccess()");
  if (!ShouldEnqueueEvent()) {
    metrics_.RecordAndReset();
    return;
  }
  if (old_version == IDBDatabaseMetadata::kNoVersion) {
    // This database hasn't had an integer version before.
    old_version = IDBDatabaseMetadata::kDefaultVersion;
  }
  SetResult(IDBAny::CreateUndefined());
  EnqueueEvent(IDBVersionChangeEvent::Create(EventTypeNames::success,
                                             old_version, base::nullopt));
  metrics_.RecordAndReset();
}

bool IDBOpenDBRequest::ShouldEnqueueEvent() const {
  if (!GetExecutionContext())
    return false;
  DCHECK(ready_state_ == PENDING || ready_state_ == DONE);
  if (request_aborted_)
    return false;
  return true;
}

DispatchEventResult IDBOpenDBRequest::DispatchEventInternal(Event* event) {
  // If the connection closed between onUpgradeNeeded and the delivery of the
  // "success" event, an "error" event should be fired instead.
  if (event->type() == EventTypeNames::success &&
      ResultAsAny()->GetType() == IDBAny::kIDBDatabaseType &&
      ResultAsAny()->IdbDatabase()->IsClosePending()) {
    DequeueEvent(event);
    SetResult(nullptr);
    HandleResponse(
        DOMException::Create(kAbortError, "The connection was closed."));
    return DispatchEventResult::kCanceledBeforeDispatch;
  }

  return IDBRequest::DispatchEventInternal(event);
}

}  // namespace blink
