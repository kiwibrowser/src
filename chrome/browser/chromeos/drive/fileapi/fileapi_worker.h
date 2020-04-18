// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides the core implementation of fileapi methods.
// The functions should be called on UI thread.
// Note that most method invocation of fileapi is done on IO thread. The gap is
// filled by FileSystemProxy.
// Also, the order of arguments for the functions which take FileSystemInterface
// at the last is intentional. The instance of FileSystemInterface should be
// accessible only on UI thread, but arguments are passed on IO thread.
// So, here is an intended use case:
//   1) Bind arguments on IO thread. Then a callback instance whose type is
//      Callback<void(FileSysstemInterface*)> is created.
//   2) Post the task to the UI thread.
//   3) On UI thread, check if the instance of FileSystemInterface is alive or
//      not. If yes, Run the callback with it.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_FILEAPI_WORKER_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_FILEAPI_WORKER_H_

#include <stdint.h>

#include <vector>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "components/drive/file_errors.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "storage/browser/blob/scoped_file.h"

namespace base {
class FilePath;
}  // namespace base

namespace storage {
struct DirectoryEntry;
class FileSystemURL;
}  // namespace storage

namespace drive {

class FileSystemInterface;

namespace fileapi_internal {

typedef base::Callback<FileSystemInterface*()> FileSystemGetter;

typedef base::Callback<
    void(base::File::Error result)> StatusCallback;
typedef base::Callback<
    void(base::File::Error result,
         const base::File::Info& file_info)> GetFileInfoCallback;
typedef base::RepeatingCallback<void(
    base::File::Error result,
    std::vector<filesystem::mojom::DirectoryEntry> file_list,
    bool has_more)>
    ReadDirectoryCallback;
typedef base::Callback<void(base::File::Error result,
                            const base::File::Info& file_info,
                            const base::FilePath& snapshot_file_path,
                            storage::ScopedFile::ScopeOutPolicy
                                scope_out_policy)> CreateSnapshotFileCallback;
typedef base::Callback<
    void(base::File::Error result,
         const base::FilePath& snapshot_file_path,
         const base::Closure& close_callback)>
    CreateWritableSnapshotFileCallback;
typedef base::Callback<
    void(base::File file,
         const base::Closure& close_callback)> OpenFileCallback;

// Gets the profile of the Drive entry pointed by |url|. Used as
// FileSystemGetter callback by binding an URL on the IO thread and passing to
// the UI thread.
FileSystemInterface* GetFileSystemFromUrl(const storage::FileSystemURL& url);

// Runs |file_system_getter| to obtain the instance of FileSystemInstance,
// and then runs |callback| with it.
// If |file_system_getter| returns NULL, runs |error_callback| instead.
// This function must be called on UI thread.
// |file_system_getter| and |callback| must not be null, but
// |error_callback| can be null (if no operation is necessary for error
// case).
void RunFileSystemCallback(
    const FileSystemGetter& file_system_getter,
    const base::Callback<void(FileSystemInterface*)>& callback,
    const base::Closure& error_callback);

// Returns the metadata info of the file at |file_path|.
// Called from FileSystemProxy::GetFileInfo().
void GetFileInfo(const base::FilePath& file_path,
                 const GetFileInfoCallback& callback,
                 FileSystemInterface* file_system);

// Copies a file from |src_file_path| to |dest_file_path|.
// Called from FileSystemProxy::Copy().
void Copy(const base::FilePath& src_file_path,
          const base::FilePath& dest_file_path,
          bool preserve_last_modified,
          const StatusCallback& callback,
          FileSystemInterface* file_system);

// Moves a file from |src_file_path| to |dest_file_path|.
// Called from FileSystemProxy::Move().
void Move(const base::FilePath& src_file_path,
          const base::FilePath& dest_file_path,
          const StatusCallback& callback,
          FileSystemInterface* file_system);


// Copies a file at |src_foreign_file_path|, which is not managed by Drive File
// System, to |dest_file_path|.
void CopyInForeignFile(const base::FilePath& src_foreign_file_path,
                       const base::FilePath& dest_file_path,
                       const StatusCallback& callback,
                       FileSystemInterface* file_system);

// Reads the contents of the directory at |file_path|.
// Called from FileSystemProxy::ReadDirectory().
void ReadDirectory(const base::FilePath& file_path,
                   const ReadDirectoryCallback& callback,
                   FileSystemInterface* file_system);

// Removes a file at |file_path|. Called from FileSystemProxy::Remove().
void Remove(const base::FilePath& file_path,
            bool is_recursive,
            const StatusCallback& callback,
            FileSystemInterface* file_system);

// Creates a new directory at |file_path|.
// Called from FileSystemProxy::CreateDirectory().
void CreateDirectory(const base::FilePath& file_path,
                     bool is_exclusive,
                     bool is_recursive,
                     const StatusCallback& callback,
                     FileSystemInterface* file_system);

// Creates a new file at |file_path|.
// Called from FileSystemProxy::CreateFile().
void CreateFile(const base::FilePath& file_path,
                bool is_exclusive,
                const StatusCallback& callback,
                FileSystemInterface* file_system);

// Truncates the file at |file_path| to |length| bytes.
// Called from FileSystemProxy::Truncate().
void Truncate(const base::FilePath& file_path,
              int64_t length,
              const StatusCallback& callback,
              FileSystemInterface* file_system);

// Creates a snapshot for the file at |file_path|.
// Called from FileSystemProxy::CreateSnapshotFile().
void CreateSnapshotFile(const base::FilePath& file_path,
                        const CreateSnapshotFileCallback& callback,
                        FileSystemInterface* file_system);

// Creates a writable snapshot for the file at |file_path|.
// After writing operation is done, |close_callback| must be called.
void CreateWritableSnapshotFile(
    const base::FilePath& file_path,
    const CreateWritableSnapshotFileCallback& callback,
    FileSystemInterface* file_system);

// Opens the file at |file_path| with options |file_flags|.
// Called from FileSystemProxy::OpenFile.
void OpenFile(const base::FilePath& file_path,
              int file_flags,
              const OpenFileCallback& callback,
              FileSystemInterface* file_system);

// Changes timestamp of the file at |file_path| to |last_access_time| and
// |last_modified_time|. Called from FileSystemProxy::TouchFile().
void TouchFile(const base::FilePath& file_path,
               const base::Time& last_access_time,
               const base::Time& last_modified_time,
               const StatusCallback& callback,
               FileSystemInterface* file_system);

}  // namespace fileapi_internal
}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_FILEAPI_WORKER_H_
