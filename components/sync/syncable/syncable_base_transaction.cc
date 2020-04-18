// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_base_transaction.h"

#include "base/trace_event/trace_event.h"
#include "components/sync/syncable/directory.h"

namespace syncer {
namespace syncable {

// static
Id BaseTransaction::root_id() {
  return Id::GetRoot();
}

Directory* BaseTransaction::directory() const {
  return directory_;
}

void BaseTransaction::Lock() {
  TRACE_EVENT2("sync_lock_contention", "AcquireLock", "src_file",
               from_here_.file_name(), "src_func", from_here_.function_name());

  directory_->kernel()->transaction_mutex.Acquire();
}

void BaseTransaction::Unlock() {
  directory_->kernel()->transaction_mutex.Release();
}

void BaseTransaction::OnUnrecoverableError(const base::Location& location,
                                           const std::string& message) {
  unrecoverable_error_set_ = true;
  unrecoverable_error_location_ = location;
  unrecoverable_error_msg_ = message;

  // Note: We dont call the Directory's OnUnrecoverableError method right
  // away. Instead we wait to unwind the stack and in the destructor of the
  // transaction we would call the OnUnrecoverableError method.

  directory()->ReportUnrecoverableError();
}

bool BaseTransaction::unrecoverable_error_set() const {
  return unrecoverable_error_set_;
}

void BaseTransaction::HandleUnrecoverableErrorIfSet() {
  if (unrecoverable_error_set_) {
    directory()->OnUnrecoverableError(this, unrecoverable_error_location_,
                                      unrecoverable_error_msg_);
  }
}

BaseTransaction::BaseTransaction(const base::Location& from_here,
                                 const char* name,
                                 WriterTag writer,
                                 Directory* directory)
    : from_here_(from_here),
      name_(name),
      writer_(writer),
      directory_(directory),
      unrecoverable_error_set_(false) {
  // TODO(lipalani): Don't issue a good transaction if the directory has
  // unrecoverable error set. And the callers have to check trans.good before
  // proceeding.
  TRACE_EVENT_BEGIN2("sync", name_, "src_file", from_here_.file_name(),
                     "src_func", from_here_.function_name());
}

BaseTransaction::~BaseTransaction() {
  TRACE_EVENT_END0("sync", name_);
}

}  // namespace syncable
}  // namespace syncer
