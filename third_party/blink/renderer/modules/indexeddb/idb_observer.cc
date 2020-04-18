// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/indexeddb/idb_observer.h"

#include <bitset>

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/modules/v8/to_v8_for_modules.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_binding_for_modules.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_idb_observer_callback.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/indexed_db_names.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_database.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_observer_changes.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_observer_init.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_transaction.h"

namespace blink {

IDBObserver* IDBObserver::Create(V8IDBObserverCallback* callback) {
  return new IDBObserver(callback);
}

IDBObserver::IDBObserver(V8IDBObserverCallback* callback)
    : callback_(callback) {}

void IDBObserver::observe(IDBDatabase* database,
                          IDBTransaction* transaction,
                          const IDBObserverInit& options,
                          ExceptionState& exception_state) {
  if (!transaction->IsActive()) {
    exception_state.ThrowDOMException(kTransactionInactiveError,
                                      transaction->InactiveErrorMessage());
    return;
  }
  if (transaction->IsVersionChange()) {
    exception_state.ThrowDOMException(
        kTransactionInactiveError,
        IDBDatabase::kCannotObserveVersionChangeTransaction);
    return;
  }
  if (!database->Backend()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      IDBDatabase::kDatabaseClosedErrorMessage);
    return;
  }
  if (!options.hasOperationTypes()) {
    exception_state.ThrowTypeError(
        "operationTypes not specified in observe options.");
    return;
  }
  if (options.operationTypes().IsEmpty()) {
    exception_state.ThrowTypeError("operationTypes must be populated.");
    return;
  }

  std::bitset<kWebIDBOperationTypeCount> types;
  for (const auto& operation_type : options.operationTypes()) {
    if (operation_type == IndexedDBNames::add) {
      types[kWebIDBAdd] = true;
    } else if (operation_type == IndexedDBNames::put) {
      types[kWebIDBPut] = true;
    } else if (operation_type == IndexedDBNames::kDelete) {
      types[kWebIDBDelete] = true;
    } else if (operation_type == IndexedDBNames::clear) {
      types[kWebIDBClear] = true;
    } else {
      exception_state.ThrowTypeError(
          "Unknown operation type in observe options: " + operation_type);
      return;
    }
  }

  int32_t observer_id =
      database->AddObserver(this, transaction->Id(), options.transaction(),
                            options.noRecords(), options.values(), types);
  observer_ids_.insert(observer_id, database);
}

void IDBObserver::unobserve(IDBDatabase* database,
                            ExceptionState& exception_state) {
  if (!database->Backend()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      IDBDatabase::kDatabaseClosedErrorMessage);
    return;
  }

  Vector<int32_t> observer_ids_to_remove;
  for (const auto& it : observer_ids_) {
    if (it.value == database)
      observer_ids_to_remove.push_back(it.key);
  }
  observer_ids_.RemoveAll(observer_ids_to_remove);

  if (!observer_ids_to_remove.IsEmpty())
    database->RemoveObservers(observer_ids_to_remove);
}

void IDBObserver::Trace(blink::Visitor* visitor) {
  visitor->Trace(callback_);
  visitor->Trace(observer_ids_);
  ScriptWrappable::Trace(visitor);
}

void IDBObserver::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(callback_);
  ScriptWrappable::TraceWrappers(visitor);
}

}  // namespace blink
