// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_DIRECTORY_CHANGE_DELEGATE_H_
#define COMPONENTS_SYNC_SYNCABLE_DIRECTORY_CHANGE_DELEGATE_H_

#include <stdint.h>

#include <vector>

#include "components/sync/base/model_type.h"
#include "components/sync/syncable/write_transaction_info.h"

namespace syncer {
namespace syncable {

// This is an interface for listening to directory change events, triggered by
// the releasing of the syncable transaction. The delegate performs work to
// 1. Calculate changes, depending on the source of the transaction
//    (HandleCalculateChangesChangeEventFromSyncer/Syncapi).
// 2. Perform final work while the transaction is held
//    (HandleTransactionEndingChangeEvent).
// 3. Perform any work that should be done after the transaction is released.
//    (HandleTransactionCompleteChangeEvent).
//
// Note that these methods may be called on *any* thread.
class DirectoryChangeDelegate {
 public:
  // Returns the handles of changed entries in |entry_changed|.
  virtual void HandleCalculateChangesChangeEventFromSyncApi(
      const ImmutableWriteTransactionInfo& write_transaction_info,
      BaseTransaction* trans,
      std::vector<int64_t>* entries_changed) = 0;
  // Returns the handles of changed entries in |entry_changed|.
  virtual void HandleCalculateChangesChangeEventFromSyncer(
      const ImmutableWriteTransactionInfo& write_transaction_info,
      BaseTransaction* trans,
      std::vector<int64_t>* entries_changed) = 0;
  // Must return the set of all ModelTypes that were modified in the
  // transaction.
  virtual ModelTypeSet HandleTransactionEndingChangeEvent(
      const ImmutableWriteTransactionInfo& write_transaction_info,
      BaseTransaction* trans) = 0;
  virtual void HandleTransactionCompleteChangeEvent(
      ModelTypeSet models_with_changes) = 0;

 protected:
  virtual ~DirectoryChangeDelegate() {}
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_DIRECTORY_CHANGE_DELEGATE_H_
