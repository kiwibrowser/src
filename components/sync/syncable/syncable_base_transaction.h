// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_TRANSACTION_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_TRANSACTION_H_

#include <string>

#include "base/location.h"
#include "base/macros.h"
#include "components/sync/syncable/syncable_id.h"

namespace syncer {
namespace syncable {

class Directory;

// A WriteTransaction has a writer tag describing which body of code is doing
// the write. This is defined up here since WriteTransactionInfo also contains
// one.
enum WriterTag {
  INVALID,
  SYNCER,
  AUTHWATCHER,
  UNITTEST,
  VACUUM_AFTER_SAVE,
  HANDLE_SAVE_FAILURE,
  PURGE_ENTRIES,
  SYNCAPI,
};

// Make sure to update this if you update WriterTag.
std::string WriterTagToString(WriterTag writer_tag);

class BaseTransaction {
 public:
  static Id root_id();

  Directory* directory() const;

  virtual ~BaseTransaction();

  // This should be called when a database corruption is detected and there is
  // no way for us to recover short of wiping the database clean. When this is
  // called we set a bool in the transaction. The caller has to unwind the
  // stack. When the destructor for the transaction is called it acts upon the
  // bool and calls the Directory to handle the unrecoverable error.
  void OnUnrecoverableError(const base::Location& location,
                            const std::string& message);

  bool unrecoverable_error_set() const;

 protected:
  BaseTransaction(const base::Location& from_here,
                  const char* name,
                  WriterTag writer,
                  Directory* directory);

  void Lock();
  void Unlock();

  // This should be called before unlocking because it calls the Direcotry's
  // OnUnrecoverableError method which is not protected by locks and could
  // be called from any thread. Holding the transaction lock ensures only one
  // thread could call the method at a time.
  void HandleUnrecoverableErrorIfSet();

  const base::Location from_here_;
  const char* const name_;
  WriterTag writer_;
  Directory* const directory_;

  // Error information.
  bool unrecoverable_error_set_;
  base::Location unrecoverable_error_location_;
  std::string unrecoverable_error_msg_;

 private:
  friend class Entry;
  DISALLOW_COPY_AND_ASSIGN(BaseTransaction);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_BASE_TRANSACTION_H_
