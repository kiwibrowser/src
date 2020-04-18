// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_
#define COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sync/model/sync_change_processor.h"
#include "components/sync/syncable/base_transaction.h"

namespace base {
class Location;
}  // namespace base

namespace syncer {

namespace syncable {
class BaseTransaction;
class WriteTransaction;
}  // namespace syncable

// Sync API's WriteTransaction is a read/write BaseTransaction.  It wraps
// a syncable::WriteTransaction.
//
// NOTE: Only a single model type can be mutated for a given
// WriteTransaction.
class WriteTransaction : public BaseTransaction {
 public:
  // Start a new read/write transaction.
  WriteTransaction(const base::Location& from_here, UserShare* share);
  // |transaction_version| stores updated model and nodes version if model
  // is changed by the transaction, or syncable::kInvalidTransaction
  // if not after transaction is closed. This constructor is used for model
  // types that support embassy data.
  WriteTransaction(const base::Location& from_here,
                   UserShare* share,
                   int64_t* transaction_version);
  ~WriteTransaction() override;

  // Provide access to the syncable transaction from the API WriteNode.
  syncable::BaseTransaction* GetWrappedTrans() const override;
  syncable::WriteTransaction* GetWrappedWriteTrans() { return transaction_; }

  // Set's a |type|'s local context. |refresh_status| controls whether
  // a datatype refresh is performed (clearing the progress marker token and
  // setting the version of all synced entities to 1).
  void SetDataTypeContext(
      ModelType type,
      SyncChangeProcessor::ContextRefreshStatus refresh_status,
      const std::string& context);

 protected:
  WriteTransaction() {}

  void SetTransaction(syncable::WriteTransaction* trans) {
    transaction_ = trans;
  }

 private:
  void* operator new(size_t size);  // Transaction is meant for stack use only.

  // The underlying syncable object which this class wraps.
  syncable::WriteTransaction* transaction_;

  DISALLOW_COPY_AND_ASSIGN(WriteTransaction);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_WRITE_TRANSACTION_H_
