// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_SET_PROPERTY_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_SET_PROPERTY_OPERATION_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_requests.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace drive {
namespace internal {
class ResourceMetadata;
}  // namespace internal

class ResourceEntry;

namespace file_system {

class OperationDelegate;

class SetPropertyOperation {
 public:
  SetPropertyOperation(base::SequencedTaskRunner* blocking_task_runner,
                       OperationDelegate* delegate,
                       internal::ResourceMetadata* metadata);
  ~SetPropertyOperation();

  // Sets the |key| property on the entry at |drive_file_path| with the
  // specified |visibility|. If the property already exists, it will be
  // overwritten.
  void SetProperty(const base::FilePath& drive_file_path,
                   google_apis::drive::Property::Visibility visibility,
                   const std::string& key,
                   const std::string& value,
                   const FileOperationCallback& callback);

 private:
  // Part of SetProperty(). Runs after updating the local state.
  void SetPropertyAfterUpdateLocalState(const FileOperationCallback& callback,
                                        const ResourceEntry* entry,
                                        FileError result);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  internal::ResourceMetadata* metadata_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SetPropertyOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SetPropertyOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_SET_PROPERTY_OPERATION_H_
