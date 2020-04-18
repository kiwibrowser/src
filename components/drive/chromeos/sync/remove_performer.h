// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_SYNC_REMOVE_PERFORMER_H_
#define COMPONENTS_DRIVE_CHROMEOS_SYNC_REMOVE_PERFORMER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace base {
class SequencedTaskRunner;
}

namespace google_apis {
class FileResource;
}

namespace drive {

class JobScheduler;
class ResourceEntry;
struct ClientContext;

namespace file_system {
class OperationDelegate;
}  // namespace file_system

namespace internal {

class EntryRevertPerformer;
class ResourceMetadata;

// This class encapsulates the drive Remove function.  It is responsible for
// sending the request to the drive API, then updating the local state and
// metadata to reflect the new state.
class RemovePerformer {
 public:
  RemovePerformer(base::SequencedTaskRunner* blocking_task_runner,
                  file_system::OperationDelegate* delegate,
                  JobScheduler* scheduler,
                  ResourceMetadata* metadata);
  ~RemovePerformer();

  // Removes the resource.
  //
  // |callback| must not be null.
  void Remove(const std::string& local_id,
              const ClientContext& context,
              const FileOperationCallback& callback);

 private:
  // Part of Remove(). Called after GetResourceEntry() completion.
  void RemoveAfterGetResourceEntry(const ClientContext& context,
                                   const FileOperationCallback& callback,
                                   const ResourceEntry* entry,
                                   FileError error);

  // Requests the server to move the specified resource to the trash.
  void TrashResource(const ClientContext& context,
                     const FileOperationCallback& callback,
                     const std::string& resource_id,
                     const std::string& local_id);

  // Part of TrashResource(). Called after server-side removal is done.
  void TrashResourceAfterUpdateRemoteState(
      const ClientContext& context,
      const FileOperationCallback& callback,
      const std::string& local_id,
      google_apis::DriveApiErrorCode status);

  // Requests the server to detach the specified resource from its parent.
  void UnparentResource(const ClientContext& context,
                        const FileOperationCallback& callback,
                        const std::string& resource_id,
                        const std::string& local_id);

  // Part of UnparentResource().
  void UnparentResourceAfterGetFileResource(
      const ClientContext& context,
      const FileOperationCallback& callback,
      const std::string& local_id,
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::FileResource> file_resource);

  // Part of UnparentResource().
  void UnparentResourceAfterUpdateRemoteState(
      const FileOperationCallback& callback,
      const std::string& local_id,
      google_apis::DriveApiErrorCode status);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  file_system::OperationDelegate* delegate_;
  JobScheduler* scheduler_;
  ResourceMetadata* metadata_;
  std::unique_ptr<EntryRevertPerformer> entry_revert_performer_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<RemovePerformer> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(RemovePerformer);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_SYNC_REMOVE_PERFORMER_H_
