// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides task related API functions.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MOUNT_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MOUNT_H_

#include <vector>

#include "base/files/file_path.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_base.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"

namespace extensions {

// Implements chrome.fileManagerPrivate.addMount method.
// Mounts removable devices and archive files.
class FileManagerPrivateAddMountFunction : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.addMount",
                             FILEMANAGERPRIVATE_ADDMOUNT)

 protected:
  ~FileManagerPrivateAddMountFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  // Part of Run(). Called after GetFile for Drive File System.
  void RunAfterGetDriveFile(const base::FilePath& drive_path,
                            drive::FileError error,
                            const base::FilePath& cache_path,
                            std::unique_ptr<drive::ResourceEntry> entry);

  // Part of Run(). Called after IsCacheMarkedAsMounted for Drive File System.
  void RunAfterIsCacheFileMarkedAsMounted(const base::FilePath& display_name,
                                          const base::FilePath& cache_path,
                                          drive::FileError error,
                                          bool is_marked_as_mounted);

  // Part of Run(). Called after MarkCacheFielAsMounted for Drive File System.
  // (or directly called from RunAsync() for other file system, or when the
  // file is already marked as mounted).
  void RunAfterMarkCacheFileAsMounted(const base::FilePath& display_name,
                                      drive::FileError error,
                                      const base::FilePath& file_path);
};

// Implements chrome.fileManagerPrivate.removeMount method.
// Unmounts selected volume. Expects volume id as an argument.
class FileManagerPrivateRemoveMountFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.removeMount",
                             FILEMANAGERPRIVATE_REMOVEMOUNT)

 protected:
  ~FileManagerPrivateRemoveMountFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;
};

// Implements chrome.fileManagerPrivate.markCacheAsMounted method.
// Marks a cached file as mounted or unmounted.
class FileManagerPrivateMarkCacheAsMountedFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.markCacheAsMounted",
                             FILEMANAGERPRIVATE_MARKCACHEASMOUNTED)

 protected:
  ~FileManagerPrivateMarkCacheAsMountedFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  // Part of Run(). Called after GetFile for Drive File System.
  void RunAfterGetDriveFile(const base::FilePath& drive_path,
                            bool is_mounted,
                            drive::FileError error,
                            const base::FilePath& cache_path,
                            std::unique_ptr<drive::ResourceEntry> entry);

  // Part of Run(). Called after MarkCacheFielAsMounted for Drive File System.
  void RunAfterMarkCacheFileAsMounted(drive::FileError error,
                                      const base::FilePath& file_path);

  // Part of Run(). Called after MarkCacheFielAsUnmounted for Drive File System.
  void RunAfterMarkCacheFileAsUnmounted(drive::FileError error);
};

// Implements chrome.fileManagerPrivate.getVolumeMetadataList method.
class FileManagerPrivateGetVolumeMetadataListFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.getVolumeMetadataList",
                             FILEMANAGERPRIVATE_GETVOLUMEMETADATALIST)

 protected:
  ~FileManagerPrivateGetVolumeMetadataListFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MOUNT_H_
