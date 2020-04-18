// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/test_transaction_observer.h"

namespace syncer {

namespace syncable {

TestTransactionObserver::TestTransactionObserver()
    : transactions_observed_(0) {}

TestTransactionObserver::~TestTransactionObserver() {}

int TestTransactionObserver::transactions_observed() {
  return transactions_observed_;
}

void TestTransactionObserver::OnTransactionWrite(
    const ImmutableWriteTransactionInfo& write_transaction_info,
    ModelTypeSet models_with_changes) {
  transactions_observed_++;
}

}  // namespace syncable

}  // namespace syncer
