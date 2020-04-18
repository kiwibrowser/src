// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TRUNCATE_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TRUNCATE_OPERATION_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace drive {

class JobScheduler;
class ResourceEntry;

namespace internal {
class FileCache;
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class OperationDelegate;
class DownloadOperation;

// This class encapsulates the drive Truncate function. It is responsible for
// fetching the content from the Drive server if necessary, truncating the
// file content actually, and then notifying the file is locally modified and
// that it is necessary to upload the file to the server.
class TruncateOperation {
 public:
  TruncateOperation(base::SequencedTaskRunner* blocking_task_runner,
                    OperationDelegate* delegate,
                    JobScheduler* scheduler,
                    internal::ResourceMetadata* metadata,
                    internal::FileCache* cache,
                    const base::FilePath& temporary_file_directory);
  ~TruncateOperation();

  // Performs the truncate operation on the file at drive path |file_path| to
  // |length| bytes. Invokes |callback| when finished with the result of the
  // operation. |callback| must not be null.
  void Truncate(const base::FilePath& file_path,
                int64_t length,
                const FileOperationCallback& callback);

 private:
  // Part of Truncate(). Called after EnsureFileDownloadedByPath() is complete.
  void TruncateAfterEnsureFileDownloadedByPath(
      int64_t length,
      const FileOperationCallback& callback,
      FileError error,
      const base::FilePath& local_file_path,
      std::unique_ptr<ResourceEntry> resource_entry);

  // Part of Truncate(). Called after TruncateOnBlockingPool() is complete.
  void TruncateAfterTruncateOnBlockingPool(
      const std::string& local_id,
      const FileOperationCallback& callback,
      FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;
  internal::FileCache* cache_;

  std::unique_ptr<DownloadOperation> download_operation_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<TruncateOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(TruncateOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TRUNCATE_OPERATION_H_
