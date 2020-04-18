// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/change_processor.h"

#include <utility>

namespace syncer {

ChangeProcessor::ChangeProcessor(
    std::unique_ptr<DataTypeErrorHandler> error_handler)
    : error_handler_(std::move(error_handler)), share_handle_(nullptr) {}

ChangeProcessor::~ChangeProcessor() {}

void ChangeProcessor::Start(UserShare* share_handle) {
  DCHECK(!share_handle_);
  share_handle_ = share_handle;
  StartImpl();
}

// Not implemented by default.
void ChangeProcessor::CommitChangesFromSyncModel() {}

DataTypeErrorHandler* ChangeProcessor::error_handler() const {
  return error_handler_.get();
}

UserShare* ChangeProcessor::share_handle() const {
  return share_handle_;
}

}  // namespace syncer
