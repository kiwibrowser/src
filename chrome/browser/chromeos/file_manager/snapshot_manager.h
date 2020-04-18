// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_MANAGER_SNAPSHOT_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_FILE_MANAGER_SNAPSHOT_MANAGER_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "base/containers/circular_deque.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

class Profile;

namespace base {
class FilePath;
}  // namespace base

namespace storage {
class FileSystemURL;
}  // namespace storage

namespace storage {
class ShareableFileReference;
}  // namespace storage

namespace file_manager {

// Utility class for creating a snapshot of a file system file on local disk.
// The class wraps the underlying implementation of fileapi's CreateSnapshotFile
// and prolongs the lifetime of snapshot files so that the client code that just
// accepts file paths works without problems.
class SnapshotManager {
 public:
  // The callback type for CreateManagedSnapshot.
  typedef base::Callback<void(const base::FilePath&)> LocalPathCallback;

  explicit SnapshotManager(Profile* profile);
  ~SnapshotManager();

  // Creates a snapshot file copy of a file system file |absolute_file_path| and
  // returns back to |callback|. Returns empty path for failure.
  void CreateManagedSnapshot(const base::FilePath& absolute_file_path,
                             const LocalPathCallback& callback);

  // Struct for keeping the snapshot file reference with its file size used for
  // computing the necessity of clean up.
  struct FileReferenceWithSizeInfo {
    FileReferenceWithSizeInfo(
        scoped_refptr<storage::ShareableFileReference> ref,
        int64_t size);
    FileReferenceWithSizeInfo(const FileReferenceWithSizeInfo& other);
    ~FileReferenceWithSizeInfo();
    scoped_refptr<storage::ShareableFileReference> file_ref;
    int64_t file_size;
  };

 private:
  // Part of CreateManagedSnapshot.
  void CreateManagedSnapshotAfterSpaceComputed(
      const storage::FileSystemURL& filesystem_url,
      const LocalPathCallback& callback,
      int64_t needed_space);

  // Part of CreateManagedSnapshot.
  void OnCreateSnapshotFile(
      const LocalPathCallback& callback,
      base::File::Error result,
      const base::File::Info& file_info,
      const base::FilePath& platform_path,
      scoped_refptr<storage::ShareableFileReference> file_ref);

  Profile* profile_;
  base::circular_deque<FileReferenceWithSizeInfo> file_refs_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SnapshotManager> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SnapshotManager);
};

}  // namespace file_manager

#endif  // CHROME_BROWSER_CHROMEOS_FILE_MANAGER_SNAPSHOT_MANAGER_H_
