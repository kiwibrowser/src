// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/download_operation.h"

#include <stdint.h>

#include <utility>

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_change.h"
#include "components/drive/file_errors.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace drive {
namespace file_system {
namespace {

// Generates an unused file path with |extension| to |out_path|, as a descendant
// of |dir|, with its parent directory created.
bool GeneratesUniquePathWithExtension(
    const base::FilePath& dir,
    const base::FilePath::StringType& extension,
    base::FilePath* out_path) {
  base::FilePath subdir;
  if (!base::CreateTemporaryDirInDir(dir, base::FilePath::StringType(),
                                     &subdir)) {
    return false;
  }
  *out_path = subdir.Append(FILE_PATH_LITERAL("tmp") + extension);
  return true;
}

// Prepares for downloading the file. Allocates the enough space for the file
// in the cache.
// If succeeded, returns FILE_ERROR_OK with |temp_download_file| storing the
// path to the file in the cache.
FileError PrepareForDownloadFile(internal::FileCache* cache,
                                 int64_t expected_file_size,
                                 const base::FilePath& temporary_file_directory,
                                 base::FilePath* temp_download_file) {
  DCHECK(cache);
  DCHECK(temp_download_file);

  // Ensure enough space in the cache.
  if (!cache->FreeDiskSpaceIfNeededFor(expected_file_size))
    return FILE_ERROR_NO_LOCAL_SPACE;

  return base::CreateTemporaryFileInDir(
      temporary_file_directory,
      temp_download_file) ? FILE_ERROR_OK : FILE_ERROR_FAILED;
}

// If the resource is a hosted document, creates a JSON file representing the
// resource locally, and returns FILE_ERROR_OK with |cache_file_path| storing
// the path to the JSON file.
// If the resource is a regular file and its local cache is available,
// returns FILE_ERROR_OK with |cache_file_path| storing the path to the
// cache file.
// If the resource is a regular file but its local cache is NOT available,
// returns FILE_ERROR_OK, but |cache_file_path| is kept empty.
// Otherwise returns error code.
FileError CheckPreConditionForEnsureFileDownloaded(
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& temporary_file_directory,
    const std::string& local_id,
    ResourceEntry* entry,
    base::FilePath* cache_file_path,
    base::FilePath* temp_download_file_path) {
  DCHECK(metadata);
  DCHECK(cache);
  DCHECK(cache_file_path);

  FileError error = metadata->GetResourceEntryById(local_id, entry);
  if (error != FILE_ERROR_OK)
    return error;

  if (entry->file_info().is_directory())
    return FILE_ERROR_NOT_A_FILE;

  // For a hosted document, we create a special JSON file to represent the
  // document instead of fetching the document content in one of the exported
  // formats. The JSON file contains the edit URL and resource ID of the
  // document.
  if (entry->file_specific_info().is_hosted_document()) {
    base::FilePath::StringType extension = base::FilePath::FromUTF8Unsafe(
        entry->file_specific_info().document_extension()).value();
    base::FilePath gdoc_file_path;
    base::File::Info file_info;
    // We add the gdoc file extension in the temporary file, so that in cross
    // profile drag-and-drop between Drive folders, the destination profiles's
    // CopyOperation can detect the special JSON file only by the path.
    if (!GeneratesUniquePathWithExtension(temporary_file_directory, extension,
                                          &gdoc_file_path) ||
        !util::CreateGDocFile(gdoc_file_path, GURL(entry->alternate_url()),
                              entry->resource_id()) ||
        !base::GetFileInfo(gdoc_file_path,
                           reinterpret_cast<base::File::Info*>(&file_info)))
      return FILE_ERROR_FAILED;

    *cache_file_path = gdoc_file_path;
    entry->mutable_file_info()->set_size(file_info.size);
    return FILE_ERROR_OK;
  }

  if (!entry->file_specific_info().cache_state().is_present()) {
    // This file has no cache file.
    if (!entry->resource_id().empty()) {
      // This entry exists on the server, leave |cache_file_path| empty to
      // start download.
      return PrepareForDownloadFile(cache, entry->file_info().size(),
                                    temporary_file_directory,
                                    temp_download_file_path);
    }

    // This entry does not exist on the server, store an empty file and mark it
    // as dirty.
    base::FilePath empty_file;
    if (!base::CreateTemporaryFileInDir(temporary_file_directory, &empty_file))
      return FILE_ERROR_FAILED;
    error = cache->Store(local_id, std::string(), empty_file,
                         internal::FileCache::FILE_OPERATION_MOVE);
    if (error != FILE_ERROR_OK)
      return error;

    error = metadata->GetResourceEntryById(local_id, entry);
    if (error != FILE_ERROR_OK)
      return error;
  }

  // Leave |cache_file_path| empty when the stored file is obsolete and has no
  // local modification.
  if (!entry->file_specific_info().cache_state().is_dirty() &&
      entry->file_specific_info().md5() !=
      entry->file_specific_info().cache_state().md5()) {
    return PrepareForDownloadFile(cache, entry->file_info().size(),
                                  temporary_file_directory,
                                  temp_download_file_path);
  }

  // Fill |cache_file_path| with the path to the cached file.
  error = cache->GetFile(local_id, cache_file_path);
  if (error != FILE_ERROR_OK)
    return error;

  // If the cache file is to be returned as the download result, the file info
  // of the cache needs to be returned via |entry|.
  // TODO(kinaba): crbug.com/246469. The logic below is similar to that in
  // drive::FileSystem::CheckLocalModificationAndRun. We should merge them.
  base::File::Info file_info;
  if (base::GetFileInfo(*cache_file_path, &file_info))
    entry->mutable_file_info()->set_size(file_info.size);

  return FILE_ERROR_OK;
}

struct CheckPreconditionForEnsureFileDownloadedParams {
  internal::ResourceMetadata* metadata;
  internal::FileCache* cache;
  base::FilePath temporary_file_directory;
};

// Calls CheckPreConditionForEnsureFileDownloaded() with the entry specified by
// the given ID. Also fills |drive_file_path| with the path of the entry.
FileError CheckPreConditionForEnsureFileDownloadedByLocalId(
    const CheckPreconditionForEnsureFileDownloadedParams& params,
    const std::string& local_id,
    base::FilePath* drive_file_path,
    base::FilePath* cache_file_path,
    base::FilePath* temp_download_file_path,
    ResourceEntry* entry) {
  FileError error = params.metadata->GetFilePath(local_id, drive_file_path);
  if (error != FILE_ERROR_OK)
    return error;
  return CheckPreConditionForEnsureFileDownloaded(
      params.metadata, params.cache, params.temporary_file_directory, local_id,
      entry, cache_file_path, temp_download_file_path);
}

// Calls CheckPreConditionForEnsureFileDownloaded() with the entry specified by
// the given file path.
FileError CheckPreConditionForEnsureFileDownloadedByPath(
    const CheckPreconditionForEnsureFileDownloadedParams& params,
    const base::FilePath& file_path,
    base::FilePath* cache_file_path,
    base::FilePath* temp_download_file_path,
    ResourceEntry* entry) {
  std::string local_id;
  FileError error = params.metadata->GetIdByPath(file_path, &local_id);
  if (error != FILE_ERROR_OK)
    return error;
  return CheckPreConditionForEnsureFileDownloaded(
      params.metadata, params.cache, params.temporary_file_directory, local_id,
      entry, cache_file_path, temp_download_file_path);
}

// Stores the downloaded file at |downloaded_file_path| into |cache|.
// If succeeded, returns FILE_ERROR_OK with |cache_file_path| storing the
// path to the cache file.
// If failed, returns an error code with deleting |downloaded_file_path|.
FileError UpdateLocalStateForDownloadFile(
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const ResourceEntry& entry_before_download,
    google_apis::DriveApiErrorCode gdata_error,
    const base::FilePath& downloaded_file_path,
    ResourceEntry* entry_after_update,
    base::FilePath* cache_file_path) {
  DCHECK(cache);

  // Downloaded file should be deleted on errors.
  base::ScopedClosureRunner file_deleter(base::Bind(
      base::IgnoreResult(&base::DeleteFile),
      downloaded_file_path, false /* recursive */));

  FileError error = GDataToFileError(gdata_error);
  if (error != FILE_ERROR_OK)
    return error;

  const std::string& local_id = entry_before_download.local_id();

  // Do not overwrite locally edited file with server side contents.
  ResourceEntry entry;
  error = metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (entry.file_specific_info().cache_state().is_dirty())
    return FILE_ERROR_IN_USE;

  // Here the download is completed successfully, so store it into the cache.
  error = cache->Store(local_id,
                       entry_before_download.file_specific_info().md5(),
                       downloaded_file_path,
                       internal::FileCache::FILE_OPERATION_MOVE);
  if (error != FILE_ERROR_OK)
    return error;
  base::OnceClosure unused_file_deleter_closure = file_deleter.Release();

  error = metadata->GetResourceEntryById(local_id, entry_after_update);
  if (error != FILE_ERROR_OK)
    return error;

  return cache->GetFile(local_id, cache_file_path);
}

}  // namespace

class DownloadOperation::DownloadParams {
 public:
  DownloadParams(const GetFileContentInitializedCallback initialized_callback,
                 const google_apis::GetContentCallback get_content_callback,
                 const GetFileCallback completion_callback,
                 std::unique_ptr<ResourceEntry> entry)
      : initialized_callback_(initialized_callback),
        get_content_callback_(get_content_callback),
        completion_callback_(completion_callback),
        entry_(std::move(entry)),
        was_cancelled_(false),
        weak_ptr_factory_(this) {
    DCHECK(!completion_callback_.is_null());
    DCHECK(entry_);
  }

  base::Closure GetCancelClosure() {
    return base::Bind(&DownloadParams::Cancel, weak_ptr_factory_.GetWeakPtr());
  }

  void OnCacheFileFound(const base::FilePath& cache_file_path) {
    if (!initialized_callback_.is_null()) {
      initialized_callback_.Run(FILE_ERROR_OK, cache_file_path,
                                std::make_unique<ResourceEntry>(*entry_));
    }
    completion_callback_.Run(FILE_ERROR_OK, cache_file_path, std::move(entry_));
  }

  void OnStartDownloading(const base::Closure& cancel_download_closure) {
    cancel_download_closure_ = cancel_download_closure;
    if (initialized_callback_.is_null()) {
      return;
    }

    DCHECK(entry_);
    initialized_callback_.Run(FILE_ERROR_OK, base::FilePath(),
                              std::make_unique<ResourceEntry>(*entry_));
  }

  void OnError(FileError error) const {
    completion_callback_.Run(error, base::FilePath(),
                             std::unique_ptr<ResourceEntry>());
  }

  void OnDownloadCompleted(const base::FilePath& cache_file_path,
                           std::unique_ptr<ResourceEntry> entry) const {
    completion_callback_.Run(FILE_ERROR_OK, cache_file_path, std::move(entry));
  }

  const google_apis::GetContentCallback& get_content_callback() const {
    return get_content_callback_;
  }

  const ResourceEntry& entry() const { return *entry_; }

  bool was_cancelled() const { return was_cancelled_; }

 private:
  void Cancel() {
    was_cancelled_ = true;
    if (!cancel_download_closure_.is_null())
      cancel_download_closure_.Run();
  }

  const GetFileContentInitializedCallback initialized_callback_;
  const google_apis::GetContentCallback get_content_callback_;
  const GetFileCallback completion_callback_;

  std::unique_ptr<ResourceEntry> entry_;
  base::Closure cancel_download_closure_;
  bool was_cancelled_;

  base::WeakPtrFactory<DownloadParams> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DownloadParams);
};

DownloadOperation::DownloadOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    JobScheduler* scheduler,
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& temporary_file_directory)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      scheduler_(scheduler),
      metadata_(metadata),
      cache_(cache),
      temporary_file_directory_(temporary_file_directory),
      weak_ptr_factory_(this) {
}

DownloadOperation::~DownloadOperation() = default;

base::Closure DownloadOperation::EnsureFileDownloadedByLocalId(
    const std::string& local_id,
    const ClientContext& context,
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const GetFileCallback& completion_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!completion_callback.is_null());

  CheckPreconditionForEnsureFileDownloadedParams params;
  params.metadata = metadata_;
  params.cache = cache_;
  params.temporary_file_directory = temporary_file_directory_;
  base::FilePath* drive_file_path = new base::FilePath;
  base::FilePath* cache_file_path = new base::FilePath;
  base::FilePath* temp_download_file_path = new base::FilePath;
  ResourceEntry* entry = new ResourceEntry;
  std::unique_ptr<DownloadParams> download_params(
      new DownloadParams(initialized_callback, get_content_callback,
                         completion_callback, base::WrapUnique(entry)));
  base::Closure cancel_closure = download_params->GetCancelClosure();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CheckPreConditionForEnsureFileDownloadedByLocalId, params,
                     local_id, drive_file_path, cache_file_path,
                     temp_download_file_path, entry),
      base::BindOnce(
          &DownloadOperation::EnsureFileDownloadedAfterCheckPreCondition,
          weak_ptr_factory_.GetWeakPtr(), std::move(download_params), context,
          base::Owned(drive_file_path), base::Owned(cache_file_path),
          base::Owned(temp_download_file_path)));
  return cancel_closure;
}

base::Closure DownloadOperation::EnsureFileDownloadedByPath(
    const base::FilePath& file_path,
    const ClientContext& context,
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const GetFileCallback& completion_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!completion_callback.is_null());

  CheckPreconditionForEnsureFileDownloadedParams params;
  params.metadata = metadata_;
  params.cache = cache_;
  params.temporary_file_directory = temporary_file_directory_;
  base::FilePath* drive_file_path = new base::FilePath(file_path);
  base::FilePath* cache_file_path = new base::FilePath;
  base::FilePath* temp_download_file_path = new base::FilePath;
  ResourceEntry* entry = new ResourceEntry;
  std::unique_ptr<DownloadParams> download_params(
      new DownloadParams(initialized_callback, get_content_callback,
                         completion_callback, base::WrapUnique(entry)));
  base::Closure cancel_closure = download_params->GetCancelClosure();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CheckPreConditionForEnsureFileDownloadedByPath, params,
                     file_path, cache_file_path, temp_download_file_path,
                     entry),
      base::BindOnce(
          &DownloadOperation::EnsureFileDownloadedAfterCheckPreCondition,
          weak_ptr_factory_.GetWeakPtr(), std::move(download_params), context,
          base::Owned(drive_file_path), base::Owned(cache_file_path),
          base::Owned(temp_download_file_path)));
  return cancel_closure;
}

void DownloadOperation::EnsureFileDownloadedAfterCheckPreCondition(
    std::unique_ptr<DownloadParams> params,
    const ClientContext& context,
    base::FilePath* drive_file_path,
    base::FilePath* cache_file_path,
    base::FilePath* temp_download_file_path,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(params);
  DCHECK(drive_file_path);
  DCHECK(cache_file_path);

  if (error != FILE_ERROR_OK) {
    // During precondition check, an error is found.
    params->OnError(error);
    return;
  }

  if (!cache_file_path->empty()) {
    // The cache file is found.
    params->OnCacheFileFound(*cache_file_path);
    return;
  }

  if (params->was_cancelled()) {
    params->OnError(FILE_ERROR_ABORT);
    return;
  }

  DCHECK(!params->entry().resource_id().empty());
  DownloadParams* params_ptr = params.get();
  JobID id = scheduler_->DownloadFile(
      *drive_file_path,
      params_ptr->entry().file_info().size(),
      *temp_download_file_path,
      params_ptr->entry().resource_id(),
      context,
      base::Bind(&DownloadOperation::EnsureFileDownloadedAfterDownloadFile,
                 weak_ptr_factory_.GetWeakPtr(),
                 *drive_file_path,
                 base::Passed(&params)),
      params_ptr->get_content_callback());

  // Notify via |initialized_callback| if necessary.
  params_ptr->OnStartDownloading(
      base::Bind(&DownloadOperation::CancelJob,
                 weak_ptr_factory_.GetWeakPtr(), id));
}

void DownloadOperation::EnsureFileDownloadedAfterDownloadFile(
    const base::FilePath& drive_file_path,
    std::unique_ptr<DownloadParams> params,
    google_apis::DriveApiErrorCode gdata_error,
    const base::FilePath& downloaded_file_path) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  DownloadParams* params_ptr = params.get();
  ResourceEntry* entry_after_update = new ResourceEntry;
  base::FilePath* cache_file_path = new base::FilePath;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&UpdateLocalStateForDownloadFile, metadata_, cache_,
                     params_ptr->entry(), gdata_error, downloaded_file_path,
                     entry_after_update, cache_file_path),
      base::BindOnce(
          &DownloadOperation::EnsureFileDownloadedAfterUpdateLocalState,
          weak_ptr_factory_.GetWeakPtr(), drive_file_path, std::move(params),
          base::WrapUnique(entry_after_update), base::Owned(cache_file_path)));
}

void DownloadOperation::EnsureFileDownloadedAfterUpdateLocalState(
    const base::FilePath& file_path,
    std::unique_ptr<DownloadParams> params,
    std::unique_ptr<ResourceEntry> entry_after_update,
    base::FilePath* cache_file_path,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != FILE_ERROR_OK) {
    params->OnError(error);
    return;
  }
  DCHECK(!entry_after_update->file_info().is_directory());

  FileChange changed_files;
  changed_files.Update(file_path, FileChange::FILE_TYPE_FILE,
                       FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
  // Storing to cache changes the "offline available" status, hence notify.
  delegate_->OnFileChangedByOperation(changed_files);
  params->OnDownloadCompleted(*cache_file_path, std::move(entry_after_update));
}

void DownloadOperation::CancelJob(JobID job_id) {
  scheduler_->CancelJob(job_id);
}

}  // namespace file_system
}  // namespace drive
