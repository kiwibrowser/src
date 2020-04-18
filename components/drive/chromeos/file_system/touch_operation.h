// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TOUCH_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TOUCH_OPERATION_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
class Time;
}  // namespace base

namespace drive {
namespace internal {
class ResourceMetadata;
}  // namespace internal

class ResourceEntry;

namespace file_system {

class OperationDelegate;

class TouchOperation {
 public:
  TouchOperation(base::SequencedTaskRunner* blocking_task_runner,
                 OperationDelegate* delegate,
                 internal::ResourceMetadata* metadata);
  ~TouchOperation();

  // Touches the file by updating last access time and last modified time.
  // Upon completion, invokes |callback|.
  // |last_access_time|, |last_modified_time| and |callback| must not be null.
  void TouchFile(const base::FilePath& file_path,
                 const base::Time& last_access_time,
                 const base::Time& last_modified_time,
                 const FileOperationCallback& callback);

 private:
  // Part of TouchFile(). Runs after updating the local state.
  void TouchFileAfterUpdateLocalState(const base::FilePath& file_path,
                                      const FileOperationCallback& callback,
                                      const ResourceEntry* entry,
                                      FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<TouchOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(TouchOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_TOUCH_OPERATION_H_
