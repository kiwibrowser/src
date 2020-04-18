// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_ROOT_FOLDER_ID_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_ROOT_FOLDER_ID_LOADER_H_

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "components/drive/file_errors.h"

namespace drive {
namespace internal {

using RootFolderIdCallback =
    base::RepeatingCallback<void(FileError, base::Optional<std::string>)>;

// RootFolderIdLoader is an interface that will load the root_folder_id for
// change list loader and directory loader.
class RootFolderIdLoader {
 public:
  virtual ~RootFolderIdLoader() = default;

  // Retrieve the root folder id, which may be obtained from the server or
  // potentially could be a constant value.
  virtual void GetRootFolderId(const RootFolderIdCallback& callback) = 0;
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_ROOT_FOLDER_ID_LOADER_H_
