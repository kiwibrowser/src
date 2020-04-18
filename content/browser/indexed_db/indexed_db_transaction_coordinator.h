// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_COORDINATOR_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_COORDINATOR_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "content/browser/indexed_db/list_set.h"
#include "content/common/content_export.h"

namespace content {

class IndexedDBTransaction;

// Transactions are executed in the order the were created.
class CONTENT_EXPORT IndexedDBTransactionCoordinator {
 public:
  IndexedDBTransactionCoordinator();
  ~IndexedDBTransactionCoordinator();

  // Called by transactions as they start and finish.
  void DidCreateTransaction(IndexedDBTransaction* transaction);
  void DidCreateObserverTransaction(IndexedDBTransaction* transaction);
  void DidFinishTransaction(IndexedDBTransaction* transaction);

  bool IsRunningVersionChangeTransaction() const;

#ifndef NDEBUG
  bool IsActive(IndexedDBTransaction* transaction);
#endif

  // Makes a snapshot of the transaction queue. For diagnostics only.
  std::vector<const IndexedDBTransaction*> GetTransactions() const;

 private:
  friend class IndexedDBTransactionCoordinatorTest;

  void RecordMetrics() const;

  void ProcessQueuedTransactions();
  bool CanStartTransaction(IndexedDBTransaction* const transaction,
                           const std::set<int64_t>& locked_scope) const;

  // Transactions in different states are grouped below.
  // list_set is used to provide stable ordering; required by spec
  // for the queue, convenience for diagnostics for the rest.
  list_set<IndexedDBTransaction*> queued_transactions_;
  list_set<IndexedDBTransaction*> started_transactions_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBTransactionCoordinator);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_COORDINATOR_H_
