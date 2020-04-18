// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_CREATE_DIRECTORY_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_CREATE_DIRECTORY_OPERATION_H_

#include <set>

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

class FileChange;

namespace internal {
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class OperationDelegate;

// This class encapsulates the drive Create Directory function.  It is
// responsible for sending the request to the drive API, then updating the
// local state and metadata to reflect the new state.
class CreateDirectoryOperation {
 public:
  CreateDirectoryOperation(base::SequencedTaskRunner* blocking_task_runner,
                           OperationDelegate* delegate,
                           internal::ResourceMetadata* metadata);
  ~CreateDirectoryOperation();

  // Creates a new directory at |directory_path|.
  // If |is_exclusive| is true, an error is raised in case a directory exists
  // already at the |directory_path|.
  // If |is_recursive| is true, the invocation creates parent directories as
  // needed just like mkdir -p does.
  // Invokes |callback| when finished with the result of the operation.
  // |callback| must not be null.
  void CreateDirectory(const base::FilePath& directory_path,
                       bool is_exclusive,
                       bool is_recursive,
                       const FileOperationCallback& callback);

 private:
  // Part of CreateDirectory(). Called after UpdateLocalState().
  void CreateDirectoryAfterUpdateLocalState(
      const FileOperationCallback& callback,
      const std::set<std::string>* updated_local_ids,
      const FileChange* changed_directories,
      FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<CreateDirectoryOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CreateDirectoryOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_CREATE_DIRECTORY_OPERATION_H_
