// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_NULL_TRANSACTION_OBSERVER_H_
#define COMPONENTS_SYNC_TEST_NULL_TRANSACTION_OBSERVER_H_

#include "components/sync/base/weak_handle.h"

namespace syncer {
namespace syncable {

class TransactionObserver;

// Returns an initialized weak handle to a transaction observer that
// does nothing.
WeakHandle<TransactionObserver> NullTransactionObserver();

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_NULL_TRANSACTION_OBSERVER_H_
