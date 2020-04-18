// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/indexed_db/indexed_db_database.h"

namespace content {
class IndexedDBDatabaseCallbacks;
class IndexedDBDatabaseError;
class IndexedDBObserver;
class IndexedDBTransaction;

class CONTENT_EXPORT IndexedDBConnection {
 public:
  IndexedDBConnection(int child_process_id,
                      scoped_refptr<IndexedDBDatabase> db,
                      scoped_refptr<IndexedDBDatabaseCallbacks> callbacks);
  virtual ~IndexedDBConnection();

  // These methods are virtual to allow subclassing in unit tests.
  virtual void ForceClose();
  virtual void Close();
  virtual bool IsConnected();

  void VersionChangeIgnored();

  virtual void ActivatePendingObservers(
      std::vector<std::unique_ptr<IndexedDBObserver>> pending_observers);
  // Removes observer listed in |remove_observer_ids| from active_observer of
  // connection or pending_observer of transactions associated with this
  // connection.
  virtual void RemoveObservers(const std::vector<int32_t>& remove_observer_ids);

  int32_t id() const { return id_; }
  int child_process_id() const { return child_process_id_; }

  IndexedDBDatabase* database() const { return database_.get(); }
  IndexedDBDatabaseCallbacks* callbacks() const { return callbacks_.get(); }
  const std::vector<std::unique_ptr<IndexedDBObserver>>& active_observers()
      const {
    return active_observers_;
  }
  base::WeakPtr<IndexedDBConnection> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Creates a transaction for this connection.
  IndexedDBTransaction* CreateTransaction(
      int64_t id,
      const std::set<int64_t>& scope,
      blink::WebIDBTransactionMode mode,
      IndexedDBBackingStore::Transaction* backing_store_transaction);

  void AbortTransaction(IndexedDBTransaction* transaction,
                        const IndexedDBDatabaseError& error);

  void AbortAllTransactions(const IndexedDBDatabaseError& error);

  IndexedDBTransaction* GetTransaction(int64_t id) const;

  base::WeakPtr<IndexedDBTransaction> AddTransactionForTesting(
      std::unique_ptr<IndexedDBTransaction> transaction);

  // We ignore calls where the id doesn't exist to facilitate the AbortAll call.
  // TODO(dmurph): Change that so this doesn't need to ignore unknown ids.
  void RemoveTransaction(int64_t id);

  // Returns a new transaction id for an observer transaction.
  // Unique per connection.
  int64_t NewObserverTransactionId();

 private:
  const int32_t id_;

  // The renderer-created transactions are in the right 32 bits of the
  // transaction id, and the observer (browser-created) transactions are in the
  // left 32 bits.
  // This keeps track of the left 32 bits. Unsigned for defined overflow.
  uint32_t next_observer_transaction_id_ = 1;

  // The process id of the child process this connection is associated with.
  // Tracked for IndexedDBContextImpl::GetAllOriginsDetails and debugging.
  const int child_process_id_;

  // NULL in some unit tests, and after the connection is closed.
  scoped_refptr<IndexedDBDatabase> database_;

  // The connection owns transactions created on this connection.
  std::unordered_map<int64_t, std::unique_ptr<IndexedDBTransaction>>
      transactions_;

  // The callbacks_ member is cleared when the connection is closed.
  // May be NULL in unit tests.
  scoped_refptr<IndexedDBDatabaseCallbacks> callbacks_;
  std::vector<std::unique_ptr<IndexedDBObserver>> active_observers_;
  base::WeakPtrFactory<IndexedDBConnection> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBConnection);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_
