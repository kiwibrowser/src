// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/truncate_operation.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {
namespace {

// Truncates the local file at |local_cache_path| to the |length| bytes,
// then marks the resource is dirty on |cache|.
FileError TruncateOnBlockingPool(internal::ResourceMetadata* metadata,
                                 internal::FileCache* cache,
                                 const std::string& local_id,
                                 const base::FilePath& local_cache_path,
                                 int64_t length) {
  DCHECK(metadata);
  DCHECK(cache);

  std::unique_ptr<base::ScopedClosureRunner> file_closer;
  FileError error = cache->OpenForWrite(local_id, &file_closer);
  if (error != FILE_ERROR_OK)
    return error;

  base::File file(local_cache_path,
                  base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return FILE_ERROR_FAILED;

  if (!file.SetLength(length))
    return FILE_ERROR_FAILED;

  return FILE_ERROR_OK;
}

}  // namespace

TruncateOperation::TruncateOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    JobScheduler* scheduler,
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& temporary_file_directory)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      metadata_(metadata),
      cache_(cache),
      download_operation_(new DownloadOperation(blocking_task_runner,
                                                delegate,
                                                scheduler,
                                                metadata,
                                                cache,
                                                temporary_file_directory)),
      weak_ptr_factory_(this) {
}

TruncateOperation::~TruncateOperation() = default;

void TruncateOperation::Truncate(const base::FilePath& file_path,
                                 int64_t length,
                                 const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (length < 0) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, FILE_ERROR_INVALID_OPERATION));
    return;
  }

  // TODO(kinaba): http://crbug.com/132780.
  // Optimize the cases for small |length|, at least for |length| == 0.
  download_operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      base::Bind(&TruncateOperation::TruncateAfterEnsureFileDownloadedByPath,
                 weak_ptr_factory_.GetWeakPtr(), length, callback));
}

void TruncateOperation::TruncateAfterEnsureFileDownloadedByPath(
    int64_t length,
    const FileOperationCallback& callback,
    FileError error,
    const base::FilePath& local_file_path,
    std::unique_ptr<ResourceEntry> entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error);
    return;
  }
  DCHECK(entry);
  DCHECK(entry->has_file_specific_info());

  if (entry->file_specific_info().is_hosted_document()) {
    callback.Run(FILE_ERROR_INVALID_OPERATION);
    return;
  }

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&TruncateOnBlockingPool,
                 metadata_, cache_, entry->local_id(), local_file_path, length),
      base::Bind(
          &TruncateOperation::TruncateAfterTruncateOnBlockingPool,
          weak_ptr_factory_.GetWeakPtr(), entry->local_id(), callback));
}

void TruncateOperation::TruncateAfterTruncateOnBlockingPool(
    const std::string& local_id,
    const FileOperationCallback& callback,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED), local_id);

  callback.Run(error);
}

}  // namespace file_system
}  // namespace drive
