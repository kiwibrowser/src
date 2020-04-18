// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_read_transaction.h"

namespace syncer {
namespace syncable {

ReadTransaction::ReadTransaction(const base::Location& location,
                                 Directory* directory)
    : BaseTransaction(location, "ReadTransaction", INVALID, directory) {
  Lock();
}

ReadTransaction::~ReadTransaction() {
  HandleUnrecoverableErrorIfSet();
  Unlock();
}

}  // namespace syncable
}  // namespace syncer
