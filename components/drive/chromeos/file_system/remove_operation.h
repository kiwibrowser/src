// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_REMOVE_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_REMOVE_OPERATION_H_

#include <memory>

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

namespace drive {

class ResourceEntry;

namespace internal {
class FileCache;
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class OperationDelegate;

// This class encapsulates the drive Remove function.  It is responsible for
// moving the removed entry to the trash.
class RemoveOperation {
 public:
  RemoveOperation(base::SequencedTaskRunner* blocking_task_runner,
                  OperationDelegate* delegate,
                  internal::ResourceMetadata* metadata,
                  internal::FileCache* cache);
  ~RemoveOperation();

  // Removes the resource at |path|. If |path| is a directory and |is_recursive|
  // is set, it recursively removes all the descendants. If |is_recursive| is
  // not set, it succeeds only when the directory is empty.
  //
  // |callback| must not be null.
  void Remove(const base::FilePath& path,
              bool is_recursive,
              const FileOperationCallback& callback);

 private:
  // Part of Remove(). Called after UpdateLocalState() completion.
  void RemoveAfterUpdateLocalState(const FileOperationCallback& callback,
                                   const std::string* local_id,
                                   const ResourceEntry* entry,
                                   const base::FilePath* changed_directory_path,
                                   FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;
  internal::FileCache* cache_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<RemoveOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(RemoveOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_REMOVE_OPERATION_H_
