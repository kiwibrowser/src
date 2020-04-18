// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_READ_TRANSACTION_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_READ_TRANSACTION_H_

#include <stddef.h>

#include "base/macros.h"
#include "components/sync/syncable/syncable_base_transaction.h"

namespace syncer {

class ReadTransaction;

namespace syncable {

// Locks db in constructor, unlocks in destructor.
class ReadTransaction : public BaseTransaction {
 public:
  ReadTransaction(const base::Location& from_here, Directory* directory);

  ~ReadTransaction() override;

 protected:  // Don't allow creation on heap, except by sync API wrapper.
  void* operator new(size_t size) { return (::operator new)(size); }

 private:
  friend class syncer::ReadTransaction;

  DISALLOW_COPY_AND_ASSIGN(ReadTransaction);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_READ_TRANSACTION_H_
