// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_TEST_TRANSACTION_OBSERVER_H_
#define COMPONENTS_SYNC_TEST_TEST_TRANSACTION_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/base/model_type.h"
#include "components/sync/syncable/transaction_observer.h"
#include "components/sync/syncable/write_transaction_info.h"

namespace syncer {
namespace syncable {

// This class acts as a TransactionObserver for the syncable::Directory.
// It gathers information that is useful for writing test assertions.
class TestTransactionObserver
    : public base::SupportsWeakPtr<TestTransactionObserver>,
      public TransactionObserver {
 public:
  TestTransactionObserver();
  ~TestTransactionObserver() override;
  void OnTransactionWrite(
      const ImmutableWriteTransactionInfo& write_transaction_info,
      ModelTypeSet models_with_changes) override;

  // Returns the number of transactions observed.
  //
  // Transactions are generated only when meaningful changes are made.  For most
  // testing purposes, you may assume that this counts the number of syncer
  // nudges generated.
  int transactions_observed();

 private:
  int transactions_observed_;

  DISALLOW_COPY_AND_ASSIGN(TestTransactionObserver);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_TEST_TRANSACTION_OBSERVER_H_
