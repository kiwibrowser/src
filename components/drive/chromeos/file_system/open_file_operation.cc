// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/open_file_operation.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/create_file_operation.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {

OpenFileOperation::OpenFileOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    JobScheduler* scheduler,
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& temporary_file_directory)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      cache_(cache),
      create_file_operation_(new CreateFileOperation(
          blocking_task_runner, delegate, metadata)),
      download_operation_(new DownloadOperation(
          blocking_task_runner, delegate, scheduler,
          metadata, cache, temporary_file_directory)),
      weak_ptr_factory_(this) {
}

OpenFileOperation::~OpenFileOperation() = default;

void OpenFileOperation::OpenFile(const base::FilePath& file_path,
                                 OpenMode open_mode,
                                 const std::string& mime_type,
                                 const OpenFileCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  switch (open_mode) {
    case OPEN_FILE:
      // It is not necessary to create a new file even if not exists.
      // So call OpenFileAfterCreateFile directly with FILE_ERROR_OK
      // to skip file creation.
      OpenFileAfterCreateFile(file_path, callback, FILE_ERROR_OK);
      break;
    case CREATE_FILE:
      create_file_operation_->CreateFile(
          file_path,
          true,  // exclusive: fail if already exists
          mime_type,
          base::Bind(&OpenFileOperation::OpenFileAfterCreateFile,
                     weak_ptr_factory_.GetWeakPtr(), file_path, callback));
      break;
    case OPEN_OR_CREATE_FILE:
      create_file_operation_->CreateFile(
          file_path,
          false,  // not-exclusive
          mime_type,
          base::Bind(&OpenFileOperation::OpenFileAfterCreateFile,
                     weak_ptr_factory_.GetWeakPtr(), file_path, callback));
      break;
  }
}

void OpenFileOperation::OpenFileAfterCreateFile(
    const base::FilePath& file_path,
    const OpenFileCallback& callback,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), base::Closure());
    return;
  }

  download_operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      base::Bind(
          &OpenFileOperation::OpenFileAfterFileDownloaded,
          weak_ptr_factory_.GetWeakPtr(), callback));
}

void OpenFileOperation::OpenFileAfterFileDownloaded(
    const OpenFileCallback& callback,
    FileError error,
    const base::FilePath& local_file_path,
    std::unique_ptr<ResourceEntry> entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error == FILE_ERROR_OK) {
    DCHECK(entry);
    DCHECK(entry->has_file_specific_info());
    if (entry->file_specific_info().is_hosted_document())
      // No support for opening a hosted document.
      error = FILE_ERROR_INVALID_OPERATION;
  }

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), base::Closure());
    return;
  }

  std::unique_ptr<base::ScopedClosureRunner>* file_closer =
      new std::unique_ptr<base::ScopedClosureRunner>;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::FileCache::OpenForWrite,
                 base::Unretained(cache_),
                 entry->local_id(),
                 file_closer),
      base::Bind(&OpenFileOperation::OpenFileAfterOpenForWrite,
                 weak_ptr_factory_.GetWeakPtr(),
                 local_file_path,
                 entry->local_id(),
                 callback,
                 base::Owned(file_closer)));
}

void OpenFileOperation::OpenFileAfterOpenForWrite(
    const base::FilePath& local_file_path,
    const std::string& local_id,
    const OpenFileCallback& callback,
    std::unique_ptr<base::ScopedClosureRunner>* file_closer,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), base::Closure());
    return;
  }

  ++open_files_[local_id];
  callback.Run(error, local_file_path,
               base::Bind(&OpenFileOperation::CloseFile,
                          weak_ptr_factory_.GetWeakPtr(),
                          local_id,
                          base::Passed(file_closer)));
}

void OpenFileOperation::CloseFile(
    const std::string& local_id,
    std::unique_ptr<base::ScopedClosureRunner> file_closer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_GT(open_files_[local_id], 0);

  if (--open_files_[local_id] == 0) {
    // All clients closes this file, so notify to upload the file.
    open_files_.erase(local_id);
    delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED),
                                         local_id);

    // Clients may have enlarged the file. By FreeDiskpSpaceIfNeededFor(0),
    // we try to ensure (0 + the-minimum-safe-margin = 512MB as of now) space.
    blocking_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(base::IgnoreResult(
            base::Bind(&internal::FileCache::FreeDiskSpaceIfNeededFor,
                       base::Unretained(cache_),
                       0))));
  }
}

}  // namespace file_system
}  // namespace drive
