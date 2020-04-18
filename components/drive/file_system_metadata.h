// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_FILE_SYSTEM_METADATA_H_
#define COMPONENTS_DRIVE_FILE_SYSTEM_METADATA_H_

#include <stdint.h>
#include <string>

#include "base/time/time.h"
#include "components/drive/file_errors.h"

namespace drive {

// Metadata of FileSystem. Used by FileSystem::GetMetadata().
struct FileSystemMetadata {
  FileSystemMetadata();
  ~FileSystemMetadata();

  // The start_page_token that the file system holds (may be different
  // from the one on the server)
  std::string start_page_token = "";

  // True if the resource metadata is now being fetched from the server.
  bool refreshing;

  // Time of the last update check.
  base::Time last_update_check_time;

  // Error code of the last update check.
  FileError last_update_check_error;
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_FILE_SYSTEM_METADATA_H_
