// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/file_system_metadata.h"

namespace drive {

FileSystemMetadata::FileSystemMetadata()
    : refreshing(false), last_update_check_error(FILE_ERROR_OK) {}

FileSystemMetadata::~FileSystemMetadata() {
}

}  // namespace drive
