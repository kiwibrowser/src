// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_connection.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "content/browser/indexed_db/indexed_db_class_factory.h"
#include "content/browser/indexed_db/indexed_db_database_callbacks.h"
#include "content/browser/indexed_db/indexed_db_database_error.h"
#include "content/browser/indexed_db/indexed_db_observer.h"
#include "content/browser/indexed_db/indexed_db_tracing.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"

namespace content {

namespace {

static int32_t g_next_indexed_db_connection_id;

}  // namespace

IndexedDBConnection::IndexedDBConnection(
    int child_process_id,
    scoped_refptr<IndexedDBDatabase> database,
    scoped_refptr<IndexedDBDatabaseCallbacks> callbacks)
    : id_(g_next_indexed_db_connection_id++),
      child_process_id_(child_process_id),
      database_(database),
      callbacks_(callbacks),
      weak_factory_(this) {}

IndexedDBConnection::~IndexedDBConnection() {}

void IndexedDBConnection::Close() {
  if (!callbacks_.get())
    return;
  base::WeakPtr<IndexedDBConnection> this_obj = weak_factory_.GetWeakPtr();
  database_->Close(this, false /* forced */);
  if (this_obj) {
    database_ = nullptr;
    callbacks_ = nullptr;
    active_observers_.clear();
  }
}

void IndexedDBConnection::ForceClose() {
  if (!callbacks_.get())
    return;

  // IndexedDBDatabase::Close() can delete this instance.
  base::WeakPtr<IndexedDBConnection> this_obj = weak_factory_.GetWeakPtr();
  scoped_refptr<IndexedDBDatabaseCallbacks> callbacks(callbacks_);
  database_->Close(this, true /* forced */);
  if (this_obj) {
    database_ = nullptr;
    callbacks_ = nullptr;
    active_observers_.clear();
  }
  callbacks->OnForcedClose();
}

void IndexedDBConnection::VersionChangeIgnored() {
  if (!database_.get())
    return;
  database_->VersionChangeIgnored();
}

bool IndexedDBConnection::IsConnected() {
  return database_.get() != nullptr;
}

// The observers begin listening to changes only once they are activated.
void IndexedDBConnection::ActivatePendingObservers(
    std::vector<std::unique_ptr<IndexedDBObserver>> pending_observers) {
  for (auto& observer : pending_observers) {
    active_observers_.push_back(std::move(observer));
  }
  pending_observers.clear();
}

void IndexedDBConnection::RemoveObservers(
    const std::vector<int32_t>& observer_ids_to_remove) {
  std::vector<int32_t> pending_observer_ids;
  for (int32_t id_to_remove : observer_ids_to_remove) {
    const auto& it = std::find_if(
        active_observers_.begin(), active_observers_.end(),
        [&id_to_remove](const std::unique_ptr<IndexedDBObserver>& o) {
          return o->id() == id_to_remove;
        });
    if (it != active_observers_.end())
      active_observers_.erase(it);
    else
      pending_observer_ids.push_back(id_to_remove);
  }
  if (pending_observer_ids.empty())
    return;

  for (const auto& it : transactions_) {
    it.second->RemovePendingObservers(pending_observer_ids);
  }
}

IndexedDBTransaction* IndexedDBConnection::CreateTransaction(
    int64_t id,
    const std::set<int64_t>& scope,
    blink::WebIDBTransactionMode mode,
    IndexedDBBackingStore::Transaction* backing_store_transaction) {
  CHECK_EQ(GetTransaction(id), nullptr) << "Duplicate transaction id." << id;
  std::unique_ptr<IndexedDBTransaction> transaction =
      IndexedDBClassFactory::Get()->CreateIndexedDBTransaction(
          id, this, scope, mode, backing_store_transaction);
  IndexedDBTransaction* transaction_ptr = transaction.get();
  transactions_[id] = std::move(transaction);
  return transaction_ptr;
}

void IndexedDBConnection::AbortTransaction(
    IndexedDBTransaction* transaction,
    const IndexedDBDatabaseError& error) {
  IDB_TRACE1("IndexedDBDatabase::Abort(error)", "txn.id", transaction->id());
  transaction->Abort(error);
}

void IndexedDBConnection::AbortAllTransactions(
    const IndexedDBDatabaseError& error) {
  std::unordered_map<int64_t, std::unique_ptr<IndexedDBTransaction>> temp_map;
  std::swap(temp_map, transactions_);
  for (const auto& pair : temp_map) {
    IDB_TRACE1("IndexedDBDatabase::Abort(error)", "txn.id", pair.second->id());
    pair.second->Abort(error);
  }
}

IndexedDBTransaction* IndexedDBConnection::GetTransaction(int64_t id) const {
  auto it = transactions_.find(id);
  if (it == transactions_.end())
    return nullptr;
  return it->second.get();
}

base::WeakPtr<IndexedDBTransaction>
IndexedDBConnection::AddTransactionForTesting(
    std::unique_ptr<IndexedDBTransaction> transaction) {
  DCHECK(!base::ContainsKey(transactions_, transaction->id()));
  base::WeakPtr<IndexedDBTransaction> transaction_ptr =
      transaction->ptr_factory_.GetWeakPtr();
  transactions_[transaction->id()] = std::move(transaction);
  return transaction_ptr;
}

void IndexedDBConnection::RemoveTransaction(int64_t id) {
  transactions_.erase(id);
}

int64_t IndexedDBConnection::NewObserverTransactionId() {
  // When we overflow to 0, reset the ID to 1 (id of 0 is reserved for upgrade
  // transactions).
  if (next_observer_transaction_id_ == 0)
    next_observer_transaction_id_ = 1;
  return static_cast<int64_t>(next_observer_transaction_id_++) << 32;
}

}  // namespace content
