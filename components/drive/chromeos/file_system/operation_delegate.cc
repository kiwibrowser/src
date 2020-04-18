// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/operation_delegate.h"

namespace drive {
namespace file_system {

bool OperationDelegate::WaitForSyncComplete(
    const std::string& local_id,
    const FileOperationCallback& callback) {
  return false;
}

}  // namespace file_system
}  // namespace drive
