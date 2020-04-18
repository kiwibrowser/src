// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_DOWNLOAD_OPERATION_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_DOWNLOAD_OPERATION_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/drive/file_errors.h"
#include "components/drive/job_list.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace google_apis {
class ResourceEntry;
}  // namespace google_apis

namespace drive {

class JobScheduler;
class ResourceEntry;
struct ClientContext;

namespace internal {
class FileCache;
class ResourceMetadata;
}  // namespace internal

namespace file_system {

class OperationDelegate;

class DownloadOperation {
 public:
  DownloadOperation(base::SequencedTaskRunner* blocking_task_runner,
                    OperationDelegate* delegate,
                    JobScheduler* scheduler,
                    internal::ResourceMetadata* metadata,
                    internal::FileCache* cache,
                    const base::FilePath& temporary_file_directory);
  ~DownloadOperation();

  // Ensures that the file content specified by |local_id| is locally
  // downloaded and returns a closure to cancel the task.
  // For hosted documents, this method may create a JSON file representing the
  // file.
  // For regular files, if the locally cached file is found, returns it.
  // If not found, start to download the file from the server.
  // When a JSON file is created, the cache file is found or downloading is
  // being started, |initialized_callback| is called with |local_file|
  // for JSON file or the cache file, or with |cancel_download_closure| for
  // downloading.
  // During the downloading |get_content_callback| will be called periodically
  // with the downloaded content.
  // Upon completion or an error is found, |completion_callback| will be called.
  // |initialized_callback| and |get_content_callback| can be null if not
  // needed.
  // |completion_callback| must not be null.
  base::Closure EnsureFileDownloadedByLocalId(
      const std::string& local_id,
      const ClientContext& context,
      const GetFileContentInitializedCallback& initialized_callback,
      const google_apis::GetContentCallback& get_content_callback,
      const GetFileCallback& completion_callback);

  // Does the same thing as EnsureFileDownloadedByLocalId for the file
  // specified by |file_path|.
  base::Closure EnsureFileDownloadedByPath(
      const base::FilePath& file_path,
      const ClientContext& context,
      const GetFileContentInitializedCallback& initialized_callback,
      const google_apis::GetContentCallback& get_content_callback,
      const GetFileCallback& completion_callback);

 private:
  // Parameters for EnsureFileDownloaded.
  class DownloadParams;

  // Part of EnsureFileDownloaded(). Called upon the completion of precondition
  // check.
  void EnsureFileDownloadedAfterCheckPreCondition(
      std::unique_ptr<DownloadParams> params,
      const ClientContext& context,
      base::FilePath* drive_file_path,
      base::FilePath* cache_file_path,
      base::FilePath* temp_download_file_path,
      FileError error);

  // Part of EnsureFileDownloaded(). Called after the actual downloading.
  void EnsureFileDownloadedAfterDownloadFile(
      const base::FilePath& drive_file_path,
      std::unique_ptr<DownloadParams> params,
      google_apis::DriveApiErrorCode gdata_error,
      const base::FilePath& downloaded_file_path);

  // Part of EnsureFileDownloaded(). Called after updating local state is
  // completed.
  void EnsureFileDownloadedAfterUpdateLocalState(
      const base::FilePath& file_path,
      std::unique_ptr<DownloadParams> params,
      std::unique_ptr<ResourceEntry> entry_after_update,
      base::FilePath* cache_file_path,
      FileError error);

  // Cancels the job with |job_id| in the scheduler.
  void CancelJob(JobID job_id);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationDelegate* delegate_;
  JobScheduler* scheduler_;
  internal::ResourceMetadata* metadata_;
  internal::FileCache* cache_;
  const base::FilePath temporary_file_directory_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<DownloadOperation> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DownloadOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_DOWNLOAD_OPERATION_H_
