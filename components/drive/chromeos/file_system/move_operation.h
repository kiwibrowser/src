// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_MOVE_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_MOVE_OPERATION_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace google_apis {
class ResourceEntry;
}  // namespace google_apis

namespace drive {

class FileChange;
class ResourceEntry;

namespace internal {
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class OperationDelegate;

// This class encapsulates the drive Move function.  It is responsible for
// updating the local metadata entry.
class MoveOperation {
 public:
  MoveOperation(base::SequencedTaskRunner* blocking_task_runner,
                OperationDelegate* delegate,
                internal::ResourceMetadata* metadata);
  ~MoveOperation();

  // Performs the move operation on the file at drive path |src_file_path|
  // with a target of |dest_file_path|.
  // Invokes |callback| when finished with the result of the operation.
  // |callback| must not be null.
  void Move(const base::FilePath& src_file_path,
            const base::FilePath& dest_file_path,
            const FileOperationCallback& callback);

 private:
  // Part of Move(). Called after updating the local state.
  void MoveAfterUpdateLocalState(const FileOperationCallback& callback,
                                 const FileChange* changed_file,
                                 const std::string* local_id,
                                 FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<MoveOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(MoveOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_MOVE_OPERATION_H_
