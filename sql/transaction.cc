// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/transaction.h"

#include "base/logging.h"
#include "sql/connection.h"

namespace sql {

Transaction::Transaction(Connection* connection)
    : connection_(connection),
      is_open_(false) {
}

Transaction::~Transaction() {
  if (is_open_)
    connection_->RollbackTransaction();
}

bool Transaction::Begin() {
  DCHECK(!is_open_) << "Beginning a transaction twice!";
  is_open_ = connection_->BeginTransaction();
  return is_open_;
}

void Transaction::Rollback() {
  DCHECK(is_open_) << "Attempting to roll back a nonexistent transaction. "
                   << "Did you remember to call Begin() and check its return?";
  is_open_ = false;
  connection_->RollbackTransaction();
}

bool Transaction::Commit() {
  DCHECK(is_open_) << "Attempting to commit a nonexistent transaction. "
                   << "Did you remember to call Begin() and check its return?";
  is_open_ = false;
  return connection_->CommitTransaction();
}

}  // namespace sql
