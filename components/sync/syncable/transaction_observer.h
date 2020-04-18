// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_TRANSACTION_OBSERVER_H_
#define COMPONENTS_SYNC_SYNCABLE_TRANSACTION_OBSERVER_H_

#include "components/sync/base/model_type.h"
#include "components/sync/syncable/write_transaction_info.h"

namespace syncer {
namespace syncable {

class TransactionObserver {
 public:
  virtual void OnTransactionWrite(
      const ImmutableWriteTransactionInfo& write_transaction_info,
      ModelTypeSet models_with_changes) = 0;

 protected:
  virtual ~TransactionObserver() {}
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_TRANSACTION_OBSERVER_H_
