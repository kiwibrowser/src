// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_SYNC_ENTRY_UPDATE_PERFORMER_H_
#define COMPONENTS_DRIVE_CHROMEOS_SYNC_ENTRY_UPDATE_PERFORMER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace base {
class ScopedClosureRunner;
class SequencedTaskRunner;
}  // namespace base

namespace google_apis {
class FileResource;
}  // namespace google_apis

namespace drive {

class FileChange;
class JobScheduler;
struct ClientContext;

namespace file_system {
class OperationDelegate;
}  // namespace file_system

namespace internal {

class EntryRevertPerformer;
class FileCache;
class LoaderController;
class RemovePerformer;
class ResourceMetadata;

// This class is responsible to perform server side update of an entry.
class EntryUpdatePerformer {
 public:
  EntryUpdatePerformer(base::SequencedTaskRunner* blocking_task_runner,
                       file_system::OperationDelegate* delegate,
                       JobScheduler* scheduler,
                       ResourceMetadata* metadata,
                       FileCache* cache,
                       LoaderController* loader_controller);
  ~EntryUpdatePerformer();

  // Requests the server to update the metadata of the entry specified by
  // |local_id| with the locally stored one.
  // Invokes |callback| when finished with the result of the operation.
  // |callback| must not be null.
  void UpdateEntry(const std::string& local_id,
                   const ClientContext& context,
                   const FileOperationCallback& callback);

  struct LocalState;

 private:
  // Part of UpdateEntry(). Called after local metadata look up.
  void UpdateEntryAfterPrepare(const ClientContext& context,
                               const FileOperationCallback& callback,
                               std::unique_ptr<LocalState> local_state,
                               FileError error);

  // Part of UpdateEntry(). Called after UpdateResource is completed.
  void UpdateEntryAfterUpdateResource(
      const ClientContext& context,
      const FileOperationCallback& callback,
      std::unique_ptr<LocalState> local_state,
      std::unique_ptr<base::ScopedClosureRunner> loader_lock,
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::FileResource> entry);

  // Part of UpdateEntry(). Called after FinishUpdate is completed.
  void UpdateEntryAfterFinish(const FileOperationCallback& callback,
                              const FileChange* changed_files,
                              FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  file_system::OperationDelegate* delegate_;
  JobScheduler* scheduler_;
  ResourceMetadata* metadata_;
  FileCache* cache_;
  LoaderController* loader_controller_;
  std::unique_ptr<RemovePerformer> remove_performer_;
  std::unique_ptr<EntryRevertPerformer> entry_revert_performer_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<EntryUpdatePerformer> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(EntryUpdatePerformer);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_SYNC_ENTRY_UPDATE_PERFORMER_H_
