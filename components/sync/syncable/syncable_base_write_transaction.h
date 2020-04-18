// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_WRITE_TRANSACTION_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_WRITE_TRANSACTION_H_

#include "base/macros.h"
#include "components/sync/syncable/syncable_base_transaction.h"

namespace syncer {
namespace syncable {

// A base class shared by both ModelNeutralWriteTransaction and
// WriteTransaction.
class BaseWriteTransaction : public BaseTransaction {
 public:
  virtual void TrackChangesTo(const EntryKernel* entry) = 0;

 protected:
  BaseWriteTransaction(const base::Location location,
                       const char* name,
                       WriterTag writer,
                       Directory* directory);
  ~BaseWriteTransaction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseWriteTransaction);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_WRITE_TRANSACTION_H_
