// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/get_file_for_saving_operation.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/sequenced_task_runner.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/create_file_operation.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/event_logger.h"
#include "components/drive/file_write_watcher.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {

namespace {

FileError OpenCacheFileForWrite(
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const std::string& local_id,
    std::unique_ptr<base::ScopedClosureRunner>* file_closer,
    ResourceEntry* entry) {
  FileError error = cache->OpenForWrite(local_id, file_closer);
  if (error != FILE_ERROR_OK)
    return error;
  return metadata->GetResourceEntryById(local_id, entry);
}

}  // namespace

GetFileForSavingOperation::GetFileForSavingOperation(
    EventLogger* logger,
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    JobScheduler* scheduler,
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& temporary_file_directory)
    : logger_(logger),
      create_file_operation_(
          new CreateFileOperation(blocking_task_runner, delegate, metadata)),
      download_operation_(new DownloadOperation(blocking_task_runner,
                                                delegate,
                                                scheduler,
                                                metadata,
                                                cache,
                                                temporary_file_directory)),
      file_write_watcher_(new internal::FileWriteWatcher(blocking_task_runner)),
      blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      metadata_(metadata),
      cache_(cache),
      weak_ptr_factory_(this) {}

GetFileForSavingOperation::~GetFileForSavingOperation() = default;

void GetFileForSavingOperation::GetFileForSaving(
    const base::FilePath& file_path,
    const GetFileCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  create_file_operation_->CreateFile(
      file_path,
      false,  // error_if_already_exists
      std::string(),  // no specific mime type
      base::Bind(&GetFileForSavingOperation::GetFileForSavingAfterCreate,
                 weak_ptr_factory_.GetWeakPtr(),
                 file_path,
                 callback));
}

void GetFileForSavingOperation::GetFileForSavingAfterCreate(
    const base::FilePath& file_path,
    const GetFileCallback& callback,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), std::unique_ptr<ResourceEntry>());
    return;
  }

  download_operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      base::Bind(&GetFileForSavingOperation::GetFileForSavingAfterDownload,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

void GetFileForSavingOperation::GetFileForSavingAfterDownload(
    const GetFileCallback& callback,
    FileError error,
    const base::FilePath& cache_path,
    std::unique_ptr<ResourceEntry> entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), std::unique_ptr<ResourceEntry>());
    return;
  }

  const std::string& local_id = entry->local_id();
  ResourceEntry* entry_ptr = entry.get();
  std::unique_ptr<base::ScopedClosureRunner>* file_closer =
      new std::unique_ptr<base::ScopedClosureRunner>;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&OpenCacheFileForWrite, metadata_, cache_, local_id,
                     file_closer, entry_ptr),
      base::BindOnce(
          &GetFileForSavingOperation::GetFileForSavingAfterOpenForWrite,
          weak_ptr_factory_.GetWeakPtr(), callback, cache_path,
          std::move(entry), base::Owned(file_closer)));
}

void GetFileForSavingOperation::GetFileForSavingAfterOpenForWrite(
    const GetFileCallback& callback,
    const base::FilePath& cache_path,
    std::unique_ptr<ResourceEntry> entry,
    std::unique_ptr<base::ScopedClosureRunner>* file_closer,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::FilePath(), std::unique_ptr<ResourceEntry>());
    return;
  }

  const std::string& local_id = entry->local_id();
  file_write_watcher_->StartWatch(
      cache_path,
      base::Bind(&GetFileForSavingOperation::GetFileForSavingAfterWatch,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback,
                 cache_path,
                 base::Passed(&entry)),
      base::Bind(&GetFileForSavingOperation::OnWriteEvent,
                 weak_ptr_factory_.GetWeakPtr(),
                 local_id,
                 base::Passed(file_closer)));
}

void GetFileForSavingOperation::GetFileForSavingAfterWatch(
    const GetFileCallback& callback,
    const base::FilePath& cache_path,
    std::unique_ptr<ResourceEntry> entry,
    bool success) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  logger_->Log(logging::LOG_INFO, "Started watching modification to %s [%s].",
               entry->local_id().c_str(),
               success ? "ok" : "fail");

  if (!success) {
    callback.Run(FILE_ERROR_FAILED, base::FilePath(),
                 std::unique_ptr<ResourceEntry>());
    return;
  }

  callback.Run(FILE_ERROR_OK, cache_path, std::move(entry));
}

void GetFileForSavingOperation::OnWriteEvent(
    const std::string& local_id,
    std::unique_ptr<base::ScopedClosureRunner> file_closer) {
  logger_->Log(logging::LOG_INFO, "Detected modification to %s.",
               local_id.c_str());

  delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED), local_id);

  // Clients may have enlarged the file. By FreeDiskpSpaceIfNeededFor(0),
  // we try to ensure (0 + the-minimum-safe-margin = 512MB as of now) space.
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(
          base::Bind(&internal::FileCache::FreeDiskSpaceIfNeededFor,
                     base::Unretained(cache_),
                     0))));
}

}  // namespace file_system
}  // namespace drive
