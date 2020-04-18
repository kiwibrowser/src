// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "components/drive/chromeos/about_resource_loader.h"
#include "components/drive/chromeos/default_corpus_change_list_loader.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/copy_operation.h"
#include "components/drive/chromeos/file_system/create_directory_operation.h"
#include "components/drive/chromeos/file_system/create_file_operation.h"
#include "components/drive/chromeos/file_system/download_operation.h"
#include "components/drive/chromeos/file_system/get_file_for_saving_operation.h"
#include "components/drive/chromeos/file_system/move_operation.h"
#include "components/drive/chromeos/file_system/open_file_operation.h"
#include "components/drive/chromeos/file_system/remove_operation.h"
#include "components/drive/chromeos/file_system/search_operation.h"
#include "components/drive/chromeos/file_system/set_property_operation.h"
#include "components/drive/chromeos/file_system/touch_operation.h"
#include "components/drive/chromeos/file_system/truncate_operation.h"
#include "components/drive/chromeos/file_system_observer.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/remove_stale_cache_files.h"
#include "components/drive/chromeos/search_metadata.h"
#include "components/drive/chromeos/start_page_token_loader.h"
#include "components/drive/chromeos/sync_client.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_pref_names.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/resource_entry_conversion.h"
#include "components/prefs/pref_service.h"
#include "google_apis/drive/drive_api_parser.h"

namespace drive {
namespace {

// Gets a ResourceEntry from the metadata, and overwrites its file info when the
// cached file is dirty.
FileError GetLocallyStoredResourceEntry(
    internal::ResourceMetadata* resource_metadata,
    internal::FileCache* cache,
    const base::FilePath& file_path,
    ResourceEntry* entry) {
  std::string local_id;
  FileError error = resource_metadata->GetIdByPath(file_path, &local_id);
  if (error != FILE_ERROR_OK)
    return error;

  error = resource_metadata->GetResourceEntryById(local_id, entry);
  if (error != FILE_ERROR_OK)
    return error;

  // For entries that will never be cached, use the original resource entry
  // as is.
  if (!entry->has_file_specific_info() ||
      entry->file_specific_info().is_hosted_document())
    return FILE_ERROR_OK;

  // When cache is not found, use the original resource entry as is.
  if (!entry->file_specific_info().has_cache_state())
    return FILE_ERROR_OK;

  // When cache is non-dirty and obsolete (old hash), use the original entry.
  if (!entry->file_specific_info().cache_state().is_dirty() &&
      entry->file_specific_info().md5() !=
      entry->file_specific_info().cache_state().md5())
    return FILE_ERROR_OK;

  // If there's a valid cache, obtain the file info from the cache file itself.
  base::FilePath local_cache_path;
  error = cache->GetFile(local_id, &local_cache_path);
  if (error != FILE_ERROR_OK)
    return error;

  base::File::Info file_info;
  if (!base::GetFileInfo(local_cache_path, &file_info))
    return FILE_ERROR_NOT_FOUND;

  entry->mutable_file_info()->set_size(file_info.size);
  return FILE_ERROR_OK;
}

// Runs the callback with parameters.
void RunGetResourceEntryCallback(const GetResourceEntryCallback& callback,
                                 std::unique_ptr<ResourceEntry> entry,
                                 FileError error) {
  DCHECK(callback);

  if (error != FILE_ERROR_OK)
    entry.reset();
  callback.Run(error, std::move(entry));
}

// Used to implement Pin().
FileError PinInternal(internal::ResourceMetadata* resource_metadata,
                      internal::FileCache* cache,
                      const base::FilePath& file_path,
                      std::string* local_id) {
  FileError error = resource_metadata->GetIdByPath(file_path, local_id);
  if (error != FILE_ERROR_OK)
    return error;

  ResourceEntry entry;
  error = resource_metadata->GetResourceEntryById(*local_id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  // TODO(hashimoto): Support pinning directories. crbug.com/127831
  if (entry.file_info().is_directory())
    return FILE_ERROR_NOT_A_FILE;

  return cache->Pin(*local_id);
}

// Used to implement Unpin().
FileError UnpinInternal(internal::ResourceMetadata* resource_metadata,
                        internal::FileCache* cache,
                        const base::FilePath& file_path,
                        std::string* local_id) {
  FileError error = resource_metadata->GetIdByPath(file_path, local_id);
  if (error != FILE_ERROR_OK)
    return error;

  return cache->Unpin(*local_id);
}

// Used to implement MarkCacheFileAsMounted().
FileError MarkCacheFileAsMountedInternal(
    internal::ResourceMetadata* resource_metadata,
    internal::FileCache* cache,
    const base::FilePath& drive_file_path,
    base::FilePath* cache_file_path) {
  std::string local_id;
  FileError error = resource_metadata->GetIdByPath(drive_file_path, &local_id);
  if (error != FILE_ERROR_OK)
    return error;

  return cache->MarkAsMounted(local_id, cache_file_path);
}

// Used to implement IsCacheFileMarkedAsMounted().
FileError IsCacheFileMarkedAsMountedInternal(
    internal::ResourceMetadata* resource_metadata,
    internal::FileCache* cache,
    const base::FilePath& drive_file_path,
    bool* result) {
  std::string local_id;
  FileError error = resource_metadata->GetIdByPath(drive_file_path, &local_id);
  if (error != FILE_ERROR_OK)
    return error;

  *result = cache->IsMarkedAsMounted(local_id);
  return FILE_ERROR_OK;
}

// Runs the callback with arguments.
void RunMarkMountedCallback(const MarkMountedCallback& callback,
                            base::FilePath* cache_file_path,
                            FileError error) {
  DCHECK(callback);
  callback.Run(error, *cache_file_path);
}

// Runs the callback with arguments.
void RunIsMountedCallback(const IsMountedCallback& callback,
                          bool* result,
                          FileError error) {
  DCHECK(callback);
  callback.Run(error, *result);
}

// Callback for ResourceMetadata::GetLargestChangestamp.
// |callback| must not be null.
void OnGetStartPageToken(FileSystemMetadata metadata,  // Will be modified.
                         const GetFilesystemMetadataCallback& callback,
                         const std::string* start_page_token,
                         FileError error) {
  DCHECK(callback);

  metadata.start_page_token = *start_page_token;
  callback.Run(metadata);
}

// Thin adapter to map GetFileCallback to FileOperationCallback.
void GetFileCallbackToFileOperationCallbackAdapter(
    const FileOperationCallback& callback,
    FileError error,
    const base::FilePath& unused_file_path,
    std::unique_ptr<ResourceEntry> unused_entry) {
  callback.Run(error);
}

// Clears |resource_metadata| and |cache|.
FileError ResetOnBlockingPool(internal::ResourceMetadata* resource_metadata,
                              internal::FileCache* cache) {
  FileError error = resource_metadata->Reset();
  if (error != FILE_ERROR_OK)
    return error;
  return cache->ClearAll() ? FILE_ERROR_OK : FILE_ERROR_FAILED;
}

// Part of GetPathFromResourceId().
// Obtains |file_path| from |resource_id|. The function should be run on the
// blocking pool.
FileError GetPathFromResourceIdOnBlockingPool(
    internal::ResourceMetadata* resource_metadata,
    const std::string& resource_id,
    base::FilePath* file_path) {
  std::string local_id;
  const FileError error =
      resource_metadata->GetIdByResourceId(resource_id, &local_id);
  if (error != FILE_ERROR_OK)
    return error;
  return resource_metadata->GetFilePath(local_id, file_path);
}

// Part of GetPathFromResourceId().
// Called when GetPathFromResourceIdInBlockingPool is complete.
void GetPathFromResourceIdAfterGetPath(base::FilePath* file_path,
                                       const GetFilePathCallback& callback,
                                       FileError error) {
  callback.Run(error, *file_path);
}

bool FreeDiskSpaceIfNeededForOnBlockingPool(internal::FileCache* cache,
                                            int64_t num_bytes) {
  return cache->FreeDiskSpaceIfNeededFor(num_bytes);
}

int64_t CalculateCacheSizeOnBlockingPool(internal::FileCache* cache) {
  return cache->CalculateCacheSize();
}

int64_t CalculateEvictableCacheSizeOnBlockingPool(internal::FileCache* cache) {
  return cache->CalculateEvictableCacheSize();
}

// Excludes hosted documents from the given entries.
// Used to implement ReadDirectory().
void FilterHostedDocuments(const ReadDirectoryEntriesCallback& callback,
                           std::unique_ptr<ResourceEntryVector> entries) {
  DCHECK(callback);

  if (entries) {
    // TODO(kinaba): Stop handling hide_hosted_docs here. crbug.com/256520.
    std::unique_ptr<ResourceEntryVector> filtered(new ResourceEntryVector);
    for (size_t i = 0; i < entries->size(); ++i) {
      if (entries->at(i).file_specific_info().is_hosted_document()) {
        continue;
      }
      filtered->push_back(entries->at(i));
    }
    entries.swap(filtered);
  }
  callback.Run(std::move(entries));
}

// Adapter for using FileOperationCallback as google_apis::EntryActionCallback.
void RunFileOperationCallbackAsEntryActionCallback(
    const FileOperationCallback& callback,
    google_apis::DriveApiErrorCode error) {
  callback.Run(GDataToFileError(error));
}

// Checks if the |entry|'s hash is included in |hashes|.
bool CheckHashes(const std::set<std::string>& hashes,
                 const ResourceEntry& entry) {
  return hashes.find(entry.file_specific_info().md5()) != hashes.end();
}

// Runs |callback| with |error| and the list of HashAndFilePath obtained from
// |original_result|.
void RunSearchByHashesCallback(
    const SearchByHashesCallback& callback,
    FileError error,
    std::unique_ptr<MetadataSearchResultVector> original_result) {
  std::vector<HashAndFilePath> result;
  if (error != FILE_ERROR_OK) {
    callback.Run(error, result);
    return;
  }
  for (const auto& search_result : *original_result) {
    HashAndFilePath hash_and_path;
    hash_and_path.hash = search_result.md5;
    hash_and_path.path = search_result.path;
    result.push_back(hash_and_path);
  }
  callback.Run(FILE_ERROR_OK, result);
}

}  // namespace

struct FileSystem::CreateDirectoryParams {
  base::FilePath directory_path;
  bool is_exclusive;
  bool is_recursive;
  FileOperationCallback callback;
};

FileSystem::FileSystem(PrefService* pref_service,
                       EventLogger* logger,
                       internal::FileCache* cache,
                       JobScheduler* scheduler,
                       internal::ResourceMetadata* resource_metadata,
                       base::SequencedTaskRunner* blocking_task_runner,
                       const base::FilePath& temporary_file_directory)
    : pref_service_(pref_service),
      logger_(logger),
      cache_(cache),
      scheduler_(scheduler),
      resource_metadata_(resource_metadata),
      last_update_check_error_(FILE_ERROR_OK),
      blocking_task_runner_(blocking_task_runner),
      temporary_file_directory_(temporary_file_directory),
      weak_ptr_factory_(this) {
  ResetComponents();
}

FileSystem::~FileSystem() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  default_corpus_change_list_loader_->RemoveObserver(this);
}

void FileSystem::Reset(const FileOperationCallback& callback) {
  // Discard the current loader and operation objects and renew them. This is to
  // avoid that changes initiated before the metadata reset is applied after the
  // reset, which may cause an inconsistent state.
  // TODO(kinaba): callbacks held in the subcomponents are discarded. We might
  // want to have a way to abort and flush callbacks in in-flight operations.
  ResetComponents();

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&ResetOnBlockingPool, resource_metadata_, cache_),
      callback);
}

void FileSystem::ResetComponents() {
  file_system::OperationDelegate* delegate = this;

  about_resource_loader_ =
      std::make_unique<internal::AboutResourceLoader>(scheduler_);
  loader_controller_ = std::make_unique<internal::LoaderController>();

  default_corpus_change_list_loader_ =
      std::make_unique<internal::DefaultCorpusChangeListLoader>(
          logger_, blocking_task_runner_.get(), resource_metadata_, scheduler_,
          about_resource_loader_.get(), loader_controller_.get());

  default_corpus_change_list_loader_->AddObserver(this);

  sync_client_ = std::make_unique<internal::SyncClient>(
      blocking_task_runner_.get(), delegate, scheduler_, resource_metadata_,
      cache_, loader_controller_.get(), temporary_file_directory_);

  copy_operation_ = std::make_unique<file_system::CopyOperation>(
      blocking_task_runner_.get(), delegate, scheduler_, resource_metadata_,
      cache_);
  create_directory_operation_ =
      std::make_unique<file_system::CreateDirectoryOperation>(
          blocking_task_runner_.get(), delegate, resource_metadata_);
  create_file_operation_ = std::make_unique<file_system::CreateFileOperation>(
      blocking_task_runner_.get(), delegate, resource_metadata_);
  move_operation_ = std::make_unique<file_system::MoveOperation>(
      blocking_task_runner_.get(), delegate, resource_metadata_);
  open_file_operation_ = std::make_unique<file_system::OpenFileOperation>(
      blocking_task_runner_.get(), delegate, scheduler_, resource_metadata_,
      cache_, temporary_file_directory_);
  remove_operation_ = std::make_unique<file_system::RemoveOperation>(
      blocking_task_runner_.get(), delegate, resource_metadata_, cache_);
  touch_operation_ = std::make_unique<file_system::TouchOperation>(
      blocking_task_runner_.get(), delegate, resource_metadata_);
  truncate_operation_ = std::make_unique<file_system::TruncateOperation>(
      blocking_task_runner_.get(), delegate, scheduler_, resource_metadata_,
      cache_, temporary_file_directory_);
  download_operation_ = std::make_unique<file_system::DownloadOperation>(
      blocking_task_runner_.get(), delegate, scheduler_, resource_metadata_,
      cache_, temporary_file_directory_);
  search_operation_ = std::make_unique<file_system::SearchOperation>(
      blocking_task_runner_.get(), scheduler_, resource_metadata_,
      loader_controller_.get());
  get_file_for_saving_operation_ =
      std::make_unique<file_system::GetFileForSavingOperation>(

          logger_, blocking_task_runner_.get(), delegate, scheduler_,
          resource_metadata_, cache_, temporary_file_directory_);
  set_property_operation_ = std::make_unique<file_system::SetPropertyOperation>(
      blocking_task_runner_.get(), delegate, resource_metadata_);
}

void FileSystem::CheckForUpdates() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(1) << "CheckForUpdates";

  default_corpus_change_list_loader_->CheckForUpdates(
      base::Bind(&FileSystem::OnUpdateChecked, weak_ptr_factory_.GetWeakPtr()));
}

void FileSystem::OnUpdateChecked(FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(1) << "CheckForUpdates finished: " << FileErrorToString(error);
  last_update_check_time_ = base::Time::Now();
  last_update_check_error_ = error;
}

void FileSystem::AddObserver(FileSystemObserver* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observers_.AddObserver(observer);
}

void FileSystem::RemoveObserver(FileSystemObserver* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observers_.RemoveObserver(observer);
}

void FileSystem::TransferFileFromLocalToRemote(
    const base::FilePath& local_src_file_path,
    const base::FilePath& remote_dest_file_path,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  copy_operation_->TransferFileFromLocalToRemote(local_src_file_path,
                                                 remote_dest_file_path,
                                                 callback);
}

void FileSystem::Copy(const base::FilePath& src_file_path,
                      const base::FilePath& dest_file_path,
                      bool preserve_last_modified,
                      const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  copy_operation_->Copy(
      src_file_path, dest_file_path, preserve_last_modified, callback);
}

void FileSystem::Move(const base::FilePath& src_file_path,
                      const base::FilePath& dest_file_path,
                      const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  move_operation_->Move(src_file_path, dest_file_path, callback);
}

void FileSystem::Remove(const base::FilePath& file_path,
                        bool is_recursive,
                        const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  remove_operation_->Remove(file_path, is_recursive, callback);
}

void FileSystem::CreateDirectory(
    const base::FilePath& directory_path,
    bool is_exclusive,
    bool is_recursive,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  CreateDirectoryParams params;
  params.directory_path = directory_path;
  params.is_exclusive = is_exclusive;
  params.is_recursive = is_recursive;
  params.callback = callback;

  // Ensure its parent directory is loaded to the local metadata.
  ReadDirectory(directory_path.DirName(),
                ReadDirectoryEntriesCallback(),
                base::Bind(&FileSystem::CreateDirectoryAfterRead,
                           weak_ptr_factory_.GetWeakPtr(), params));
}

void FileSystem::CreateDirectoryAfterRead(const CreateDirectoryParams& params,
                                          FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(params.callback);

  DVLOG_IF(1, error != FILE_ERROR_OK) << "ReadDirectory failed. "
                                      << FileErrorToString(error);

  create_directory_operation_->CreateDirectory(
      params.directory_path, params.is_exclusive, params.is_recursive,
      params.callback);
}

void FileSystem::CreateFile(const base::FilePath& file_path,
                            bool is_exclusive,
                            const std::string& mime_type,
                            const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  create_file_operation_->CreateFile(
      file_path, is_exclusive, mime_type, callback);
}

void FileSystem::TouchFile(const base::FilePath& file_path,
                           const base::Time& last_access_time,
                           const base::Time& last_modified_time,
                           const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  touch_operation_->TouchFile(
      file_path, last_access_time, last_modified_time, callback);
}

void FileSystem::TruncateFile(const base::FilePath& file_path,
                              int64_t length,
                              const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  truncate_operation_->Truncate(file_path, length, callback);
}

void FileSystem::Pin(const base::FilePath& file_path,
                     const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  std::string* local_id = new std::string;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&PinInternal, resource_metadata_, cache_, file_path, local_id),
      base::Bind(&FileSystem::FinishPin,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback,
                 base::Owned(local_id)));
}

void FileSystem::FinishPin(const FileOperationCallback& callback,
                           const std::string* local_id,
                           FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error == FILE_ERROR_OK)
    sync_client_->AddFetchTask(*local_id);
  callback.Run(error);
}

void FileSystem::Unpin(const base::FilePath& file_path,
                       const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  std::string* local_id = new std::string;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(
          &UnpinInternal, resource_metadata_, cache_, file_path, local_id),
      base::Bind(&FileSystem::FinishUnpin,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback,
                 base::Owned(local_id)));
}

void FileSystem::FinishUnpin(const FileOperationCallback& callback,
                             const std::string* local_id,
                             FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error == FILE_ERROR_OK)
    sync_client_->RemoveFetchTask(*local_id);
  callback.Run(error);
}

void FileSystem::GetFile(const base::FilePath& file_path,
                         const GetFileCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  download_operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      GetFileContentInitializedCallback(),
      google_apis::GetContentCallback(),
      callback);
}

void FileSystem::GetFileForSaving(const base::FilePath& file_path,
                                  const GetFileCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  get_file_for_saving_operation_->GetFileForSaving(file_path, callback);
}

base::Closure FileSystem::GetFileContent(
    const base::FilePath& file_path,
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const FileOperationCallback& completion_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(initialized_callback);
  DCHECK(get_content_callback);
  DCHECK(completion_callback);

  return download_operation_->EnsureFileDownloadedByPath(
      file_path,
      ClientContext(USER_INITIATED),
      initialized_callback,
      get_content_callback,
      base::Bind(&GetFileCallbackToFileOperationCallbackAdapter,
                 completion_callback));
}

void FileSystem::GetResourceEntry(
    const base::FilePath& file_path,
    const GetResourceEntryCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  ReadDirectory(file_path.DirName(),
                ReadDirectoryEntriesCallback(),
                base::Bind(&FileSystem::GetResourceEntryAfterRead,
                           weak_ptr_factory_.GetWeakPtr(),
                           file_path,
                           callback));
}

void FileSystem::GetResourceEntryAfterRead(
    const base::FilePath& file_path,
    const GetResourceEntryCallback& callback,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  DVLOG_IF(1, error != FILE_ERROR_OK) << "ReadDirectory failed. "
                                      << FileErrorToString(error);

  std::unique_ptr<ResourceEntry> entry(new ResourceEntry);
  ResourceEntry* entry_ptr = entry.get();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&GetLocallyStoredResourceEntry, resource_metadata_, cache_,
                     file_path, entry_ptr),
      base::BindOnce(&RunGetResourceEntryCallback, callback, std::move(entry)));
}

void FileSystem::ReadDirectory(
    const base::FilePath& directory_path,
    const ReadDirectoryEntriesCallback& entries_callback_in,
    const FileOperationCallback& completion_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(completion_callback);

  const bool hide_hosted_docs =
      pref_service_->GetBoolean(prefs::kDisableDriveHostedFiles);
  ReadDirectoryEntriesCallback entries_callback = entries_callback_in;
  if (entries_callback && hide_hosted_docs)
    entries_callback = base::Bind(&FilterHostedDocuments, entries_callback);

  default_corpus_change_list_loader_->ReadDirectory(
      directory_path, entries_callback, completion_callback);
}

void FileSystem::GetAvailableSpace(
    const GetAvailableSpaceCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  about_resource_loader_->GetAboutResource(
      base::Bind(&FileSystem::OnGetAboutResource,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

void FileSystem::OnGetAboutResource(
    const GetAvailableSpaceCallback& callback,
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::AboutResource> about_resource) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  FileError error = GDataToFileError(status);
  if (error != FILE_ERROR_OK) {
    callback.Run(error, -1, -1);
    return;
  }
  DCHECK(about_resource);

  callback.Run(FILE_ERROR_OK, about_resource->quota_bytes_total(),
               about_resource->quota_bytes_used_aggregate());
}

void FileSystem::GetShareUrl(const base::FilePath& file_path,
                             const GURL& embed_origin,
                             const GetShareUrlCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  // Resolve the resource id.
  ResourceEntry* entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetResourceEntryByPath,
                 base::Unretained(resource_metadata_),
                 file_path,
                 entry),
      base::Bind(&FileSystem::GetShareUrlAfterGetResourceEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 file_path,
                 embed_origin,
                 callback,
                 base::Owned(entry)));
}

void FileSystem::GetShareUrlAfterGetResourceEntry(
    const base::FilePath& file_path,
    const GURL& embed_origin,
    const GetShareUrlCallback& callback,
    ResourceEntry* entry,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, GURL());
    return;
  }
  if (entry->resource_id().empty()) {
    // This entry does not exist on the server. Just return.
    callback.Run(FILE_ERROR_FAILED, GURL());
    return;
  }

  scheduler_->GetShareUrl(
      entry->resource_id(),
      embed_origin,
      ClientContext(USER_INITIATED),
      base::Bind(&FileSystem::OnGetResourceEntryForGetShareUrl,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

void FileSystem::OnGetResourceEntryForGetShareUrl(
    const GetShareUrlCallback& callback,
    google_apis::DriveApiErrorCode status,
    const GURL& share_url) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  FileError error = GDataToFileError(status);
  if (error != FILE_ERROR_OK) {
    callback.Run(error, GURL());
    return;
  }

  if (share_url.is_empty()) {
    callback.Run(FILE_ERROR_FAILED, GURL());
    return;
  }

  callback.Run(FILE_ERROR_OK, share_url);
}

void FileSystem::Search(const std::string& search_query,
                        const GURL& next_link,
                        const SearchCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  search_operation_->Search(search_query, next_link, callback);
}

void FileSystem::SearchMetadata(const std::string& query,
                                int options,
                                int at_most_num_matches,
                                MetadataSearchOrder order,
                                const SearchMetadataCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // TODO(satorux): Stop handling hide_hosted_docs here. crbug.com/256520.
  if (pref_service_->GetBoolean(prefs::kDisableDriveHostedFiles))
    options |= SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS;

  drive::internal::SearchMetadata(
      blocking_task_runner_, resource_metadata_, query,
      base::Bind(&drive::internal::MatchesType, options), at_most_num_matches,
      order, callback);
}

void FileSystem::SearchByHashes(const std::set<std::string>& hashes,
                                const SearchByHashesCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  drive::internal::SearchMetadata(
      blocking_task_runner_, resource_metadata_,
      /* any file name */ "", base::Bind(&CheckHashes, hashes),
      std::numeric_limits<size_t>::max(),
      drive::MetadataSearchOrder::LAST_ACCESSED,
      base::Bind(&RunSearchByHashesCallback, callback));
}

void FileSystem::OnFileChangedByOperation(const FileChange& changed_files) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (auto& observer : observers_)
    observer.OnFileChanged(changed_files);
}

void FileSystem::OnEntryUpdatedByOperation(const ClientContext& context,
                                           const std::string& local_id) {
  sync_client_->AddUpdateTask(context, local_id);
}

void FileSystem::OnDriveSyncError(file_system::DriveSyncErrorType type,
                                  const std::string& local_id) {
  base::FilePath* file_path = new base::FilePath;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetFilePath,
                 base::Unretained(resource_metadata_),
                 local_id,
                 file_path),
      base::Bind(&FileSystem::OnDriveSyncErrorAfterGetFilePath,
                 weak_ptr_factory_.GetWeakPtr(),
                 type,
                 base::Owned(file_path)));
}

void FileSystem::OnDriveSyncErrorAfterGetFilePath(
    file_system::DriveSyncErrorType type,
    const base::FilePath* file_path,
    FileError error) {
  if (error != FILE_ERROR_OK)
    return;
  for (auto& observer : observers_)
    observer.OnDriveSyncError(type, *file_path);
}

bool FileSystem::WaitForSyncComplete(const std::string& local_id,
                                     const FileOperationCallback& callback) {
  return sync_client_->WaitForUpdateTaskToComplete(local_id, callback);
}

void FileSystem::OnDirectoryReloaded(const base::FilePath& directory_path) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (auto& observer : observers_)
    observer.OnDirectoryChanged(directory_path);
}

void FileSystem::OnFileChanged(const FileChange& changed_files) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (auto& observer : observers_)
    observer.OnFileChanged(changed_files);
}

void FileSystem::OnLoadFromServerComplete() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  sync_client_->StartCheckingExistingPinnedFiles();
}

void FileSystem::OnInitialLoadComplete() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  blocking_task_runner_->PostTask(FROM_HERE,
                                  base::Bind(&internal::RemoveStaleCacheFiles,
                                             cache_,
                                             resource_metadata_));
  sync_client_->StartProcessingBacklog();
}

void FileSystem::GetMetadata(
    const GetFilesystemMetadataCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  FileSystemMetadata metadata;
  metadata.refreshing = default_corpus_change_list_loader_->IsRefreshing();

  // Metadata related to delta update.
  metadata.last_update_check_time = last_update_check_time_;
  metadata.last_update_check_error = last_update_check_error_;

  std::string* start_page_token = new std::string();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetStartPageToken,
                 base::Unretained(resource_metadata_), start_page_token),
      base::Bind(&OnGetStartPageToken, metadata, callback,
                 base::Owned(start_page_token)));
}

void FileSystem::MarkCacheFileAsMounted(
    const base::FilePath& drive_file_path,
    const MarkMountedCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  base::FilePath* cache_file_path = new base::FilePath;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&MarkCacheFileAsMountedInternal,
                 resource_metadata_,
                 cache_,
                 drive_file_path,
                 cache_file_path),
      base::Bind(
          &RunMarkMountedCallback, callback, base::Owned(cache_file_path)));
}

void FileSystem::IsCacheFileMarkedAsMounted(
    const base::FilePath& drive_file_path,
    const IsMountedCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  bool* is_mounted = new bool(false);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&IsCacheFileMarkedAsMountedInternal, resource_metadata_,
                 cache_, drive_file_path, is_mounted),
      base::Bind(&RunIsMountedCallback, callback, base::Owned(is_mounted)));
}

void FileSystem::MarkCacheFileAsUnmounted(
    const base::FilePath& cache_file_path,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (!cache_->IsUnderFileCacheDirectory(cache_file_path)) {
    callback.Run(FILE_ERROR_FAILED);
    return;
  }

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::FileCache::MarkAsUnmounted,
                 base::Unretained(cache_),
                 cache_file_path),
      callback);
}

void FileSystem::AddPermission(const base::FilePath& drive_file_path,
                               const std::string& email,
                               google_apis::drive::PermissionRole role,
                               const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  // Resolve the resource id.
  ResourceEntry* const entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetResourceEntryByPath,
                 base::Unretained(resource_metadata_),
                 drive_file_path,
                 entry),
      base::Bind(&FileSystem::AddPermissionAfterGetResourceEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 email,
                 role,
                 callback,
                 base::Owned(entry)));
}

void FileSystem::AddPermissionAfterGetResourceEntry(
    const std::string& email,
    google_apis::drive::PermissionRole role,
    const FileOperationCallback& callback,
    ResourceEntry* entry,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != FILE_ERROR_OK) {
    callback.Run(error);
    return;
  }

  scheduler_->AddPermission(
      entry->resource_id(),
      email,
      role,
      base::Bind(&RunFileOperationCallbackAsEntryActionCallback, callback));
}

void FileSystem::SetProperty(
    const base::FilePath& drive_file_path,
    google_apis::drive::Property::Visibility visibility,
    const std::string& key,
    const std::string& value,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  set_property_operation_->SetProperty(drive_file_path, visibility, key, value,
                                       callback);
}

void FileSystem::OpenFile(const base::FilePath& file_path,
                          OpenMode open_mode,
                          const std::string& mime_type,
                          const OpenFileCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  open_file_operation_->OpenFile(file_path, open_mode, mime_type, callback);
}

void FileSystem::GetPathFromResourceId(const std::string& resource_id,
                                       const GetFilePathCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  base::FilePath* const file_path = new base::FilePath();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&GetPathFromResourceIdOnBlockingPool,
                 resource_metadata_,
                 resource_id,
                 file_path),
      base::Bind(&GetPathFromResourceIdAfterGetPath,
                 base::Owned(file_path),
                 callback));
}

void FileSystem::FreeDiskSpaceIfNeededFor(
    int64_t num_bytes,
    const FreeDiskSpaceCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&FreeDiskSpaceIfNeededForOnBlockingPool, cache_, num_bytes),
      callback);
}

void FileSystem::CalculateCacheSize(const CacheSizeCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&CalculateCacheSizeOnBlockingPool, cache_), callback);
}

void FileSystem::CalculateEvictableCacheSize(
    const CacheSizeCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&CalculateEvictableCacheSizeOnBlockingPool, cache_), callback);
}
}  // namespace drive
