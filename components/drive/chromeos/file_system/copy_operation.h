// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_COPY_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_COPY_OPERATION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
class Time;
}  // namespace base

namespace google_apis {
class FileResource;
}  // namespace google_apis

namespace drive {

class JobScheduler;
class ResourceEntry;

namespace internal {
class FileCache;
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class CreateFileOperation;
class OperationDelegate;

// This class encapsulates the drive Copy function.  It is responsible for
// sending the request to the drive API, then updating the local state and
// metadata to reflect the new state.
class CopyOperation {
 public:
  CopyOperation(base::SequencedTaskRunner* blocking_task_runner,
                OperationDelegate* delegate,
                JobScheduler* scheduler,
                internal::ResourceMetadata* metadata,
                internal::FileCache* cache);
  ~CopyOperation();

  // Performs the copy operation on the file at drive path |src_file_path|
  // with a target of |dest_file_path|.
  // If |preserve_last_modified| is set to true, this tries to preserve
  // last modified time stamp. This is supported only on Drive API v2.
  // Regardless of preserve_last_modified's value, last_modified_by_me timestamp
  // will always be set to the same timestamp as last_modified.
  // Invokes |callback| when finished with the result of the operation.
  // |callback| must not be null.
  void Copy(const base::FilePath& src_file_path,
            const base::FilePath& dest_file_path,
            bool preserve_last_modified,
            const FileOperationCallback& callback);

  // Initiates transfer of |local_src_file_path| to |remote_dest_file_path|.
  // |local_src_file_path| must be a file from the local file system.
  // |remote_dest_file_path| is the virtual destination path within Drive file
  // system.
  //
  // |callback| must not be null.
  void TransferFileFromLocalToRemote(
      const base::FilePath& local_src_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback);

  // Params for Copy().
  struct CopyParams;

  // Params for TransferJsonGdocFileAfterLocalWork.
  struct TransferJsonGdocParams;

 private:
  // Part of Copy(). Called after trying to copy locally.
  void CopyAfterTryToCopyLocally(
      const CopyParams* params,
      const std::vector<std::string>* updated_local_ids,
      const bool* directory_changed,
      const bool* should_copy_on_server,
      FileError error);

  // Part of Copy(). Called after the parent entry gets synced.
  void CopyAfterParentSync(const CopyParams& params, FileError error);

  // Part of Copy(). Called after the parent resource ID is resolved.
  void CopyAfterGetParentResourceId(const CopyParams& params,
                                    const ResourceEntry* parent,
                                    FileError error);

  // Part of TransferFileFromLocalToRemote(). Called after preparation is done.
  // |gdoc_resource_id| and |parent_resource_id| is available only if the file
  // is JSON GDoc file.
  void TransferFileFromLocalToRemoteAfterPrepare(
      const base::FilePath& local_src_path,
      const base::FilePath& remote_dest_path,
      const FileOperationCallback& callback,
      std::string* gdoc_resource_id,
      ResourceEntry* parent_entry,
      FileError error);

  // Part of TransferFileFromLocalToRemote().
  void TransferJsonGdocFileAfterLocalWork(TransferJsonGdocParams* params,
                                          FileError error);

  // Copies resource with |resource_id| into the directory |parent_resource_id|
  // with renaming it to |new_title|.
  void CopyResourceOnServer(const std::string& resource_id,
                            const std::string& parent_resource_id,
                            const std::string& new_title,
                            const base::Time& last_modified,
                            const FileOperationCallback& callback);

  // Part of CopyResourceOnServer and TransferFileFromLocalToRemote.
  // Called after server side operation is done.
  void UpdateAfterServerSideOperation(
      const FileOperationCallback& callback,
      google_apis::DriveApiErrorCode status,
      std::unique_ptr<google_apis::FileResource> entry);

  // Part of CopyResourceOnServer and TransferFileFromLocalToRemote.
  // Called after local state update is done.
  void UpdateAfterLocalStateUpdate(const FileOperationCallback& callback,
                                   base::FilePath* file_path,
                                   const ResourceEntry* entry,
                                   FileError error);

  // Creates an empty file on the server at |remote_dest_path| to ensure
  // the location, stores a file at |local_file_path| in cache and marks it
  // dirty, so that SyncClient will upload the data later.
  void ScheduleTransferRegularFile(const base::FilePath& local_src_path,
                                   const base::FilePath& remote_dest_path,
                                   const FileOperationCallback& callback);

  // Part of ScheduleTransferRegularFile(). Called after file creation.
  void ScheduleTransferRegularFileAfterCreate(
      const base::FilePath& local_src_path,
      const base::FilePath& remote_dest_path,
      const FileOperationCallback& callback,
      FileError error);

  // Part of ScheduleTransferRegularFile(). Called after updating local state
  // is completed.
  void ScheduleTransferRegularFileAfterUpdateLocalState(
      const FileOperationCallback& callback,
      const base::FilePath& remote_dest_path,
      const ResourceEntry* entry,
      std::string* local_id,
      FileError error);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  JobScheduler* scheduler_;
  internal::ResourceMetadata* metadata_;
  internal::FileCache* cache_;

  // Uploading a new file is internally implemented by creating a dirty file.
  std::unique_ptr<CreateFileOperation> create_file_operation_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<CopyOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CopyOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_COPY_OPERATION_H_
