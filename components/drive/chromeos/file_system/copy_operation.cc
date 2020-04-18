// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/copy_operation.h"

#include <stdint.h>

#include <string>
#include <utility>

#include "base/task_runner_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/create_file_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/resource_entry_conversion.h"
#include "google_apis/drive/drive_api_parser.h"

namespace drive {
namespace file_system {

struct CopyOperation::CopyParams {
  base::FilePath src_file_path;
  base::FilePath dest_file_path;
  bool preserve_last_modified;
  FileOperationCallback callback;
  ResourceEntry src_entry;
  ResourceEntry parent_entry;
};

// Enum for categorizing where a gdoc represented by a JSON file exists.
enum JsonGdocLocationType {
  NOT_IN_METADATA,
  IS_ORPHAN,
  HAS_PARENT,
};

struct CopyOperation::TransferJsonGdocParams {
  TransferJsonGdocParams(const FileOperationCallback& callback,
                         const std::string& resource_id,
                         const ResourceEntry& parent_entry,
                         const std::string& new_title)
      : callback(callback),
        resource_id(resource_id),
        parent_resource_id(parent_entry.resource_id()),
        parent_local_id(parent_entry.local_id()),
        new_title(new_title),
        location_type(NOT_IN_METADATA) {
  }
  // Parameters supplied or calculated from operation arguments.
  const FileOperationCallback callback;
  const std::string resource_id;
  const std::string parent_resource_id;
  const std::string parent_local_id;
  const std::string new_title;

  // Values computed during operation.
  JsonGdocLocationType location_type;  // types where the gdoc file is located.
  std::string local_id;  // the local_id of the file (if exists in metadata.)
  base::FilePath changed_path;
};

namespace {

FileError TryToCopyLocally(internal::ResourceMetadata* metadata,
                           internal::FileCache* cache,
                           CopyOperation::CopyParams* params,
                           std::vector<std::string>* updated_local_ids,
                           bool* directory_changed,
                           bool* should_copy_on_server) {
  FileError error = metadata->GetResourceEntryByPath(params->src_file_path,
                                                     &params->src_entry);
  if (error != FILE_ERROR_OK)
    return error;

  error = metadata->GetResourceEntryByPath(params->dest_file_path.DirName(),
                                           &params->parent_entry);
  if (error != FILE_ERROR_OK)
    return error;

  if (!params->parent_entry.file_info().is_directory())
    return FILE_ERROR_NOT_A_DIRECTORY;

  // Drive File System doesn't support recursive copy.
  if (params->src_entry.file_info().is_directory())
    return FILE_ERROR_NOT_A_FILE;

  // Check destination.
  ResourceEntry dest_entry;
  error = metadata->GetResourceEntryByPath(params->dest_file_path, &dest_entry);
  switch (error) {
    case FILE_ERROR_OK:
      // File API spec says it is an error to try to "copy a file to a path
      // occupied by a directory".
      if (dest_entry.file_info().is_directory())
        return FILE_ERROR_INVALID_OPERATION;

      // Move the existing entry to the trash.
      dest_entry.set_parent_local_id(util::kDriveTrashDirLocalId);
      error = metadata->RefreshEntry(dest_entry);
      if (error != FILE_ERROR_OK)
        return error;
      updated_local_ids->push_back(dest_entry.local_id());
      *directory_changed = true;
      break;
    case FILE_ERROR_NOT_FOUND:
      break;
    default:
      return error;
  }

  // If the cache file is not present and the entry exists on the server,
  // server side copy should be used.
  if (!params->src_entry.file_specific_info().cache_state().is_present() &&
      !params->src_entry.resource_id().empty()) {
    *should_copy_on_server = true;
    return FILE_ERROR_OK;
  }

  // Copy locally.
  ResourceEntry entry;
  const int64_t now = base::Time::Now().ToInternalValue();
  const int64_t last_modified =
      params->preserve_last_modified
          ? params->src_entry.file_info().last_modified()
          : now;
  entry.set_title(params->dest_file_path.BaseName().AsUTF8Unsafe());
  entry.set_parent_local_id(params->parent_entry.local_id());
  entry.mutable_file_specific_info()->set_content_mime_type(
      params->src_entry.file_specific_info().content_mime_type());
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);
  entry.set_modification_date(base::Time::Now().ToInternalValue());
  entry.mutable_file_info()->set_last_modified(last_modified);
  // preserve_last_modified=true preserves last_modified only.
  // Regardless of preserve_last_modified's value, last_modified_by_me is
  // always set to the same value as last_modified.
  // This means that, even if preserve_last_modified=true, last_modified_by_me
  // of the new file may differ from that of the original file.
  // This behavior is due to the limitation in Drive API that we can not
  // set different timestamps to last_modified and last_modified_by_me.
  entry.set_last_modified_by_me(last_modified);
  entry.mutable_file_info()->set_last_accessed(now);

  std::string local_id;
  error = metadata->AddEntry(entry, &local_id);
  if (error != FILE_ERROR_OK)
    return error;
  updated_local_ids->push_back(local_id);
  *directory_changed = true;

  if (!params->src_entry.file_specific_info().cache_state().is_present()) {
    DCHECK(params->src_entry.resource_id().empty());
    // Locally created empty file may have no cache file.
    return FILE_ERROR_OK;
  }

  base::FilePath cache_file_path;
  error = cache->GetFile(params->src_entry.local_id(), &cache_file_path);
  if (error != FILE_ERROR_OK)
    return error;

  return cache->Store(local_id, std::string(), cache_file_path,
                      internal::FileCache::FILE_OPERATION_COPY);
}

// Stores the entry returned from the server and returns its path.
FileError UpdateLocalStateForServerSideOperation(
    internal::ResourceMetadata* metadata,
    std::unique_ptr<google_apis::FileResource> file_resource,
    ResourceEntry* entry,
    base::FilePath* file_path) {
  DCHECK(file_resource);

  std::string parent_resource_id;
  if (!ConvertFileResourceToResourceEntry(
          *file_resource, entry, &parent_resource_id) ||
      parent_resource_id.empty())
    return FILE_ERROR_NOT_A_FILE;

  std::string parent_local_id;
  FileError error = metadata->GetIdByResourceId(parent_resource_id,
                                                &parent_local_id);
  if (error != FILE_ERROR_OK)
    return error;
  entry->set_parent_local_id(parent_local_id);

  std::string local_id;
  error = metadata->AddEntry(*entry, &local_id);
  // Depending on timing, the metadata may have inserted via change list
  // already. So, FILE_ERROR_EXISTS is not an error.
  if (error == FILE_ERROR_EXISTS)
    error = metadata->GetIdByResourceId(entry->resource_id(), &local_id);

  if (error != FILE_ERROR_OK)
    return error;

  return metadata->GetFilePath(local_id, file_path);
}

// Stores the file at |local_file_path| to the cache as a content of entry at
// |remote_dest_path|, and marks it dirty.
FileError UpdateLocalStateForScheduleTransfer(
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache,
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    ResourceEntry* entry,
    std::string* local_id) {
  FileError error = metadata->GetIdByPath(remote_dest_path, local_id);
  if (error != FILE_ERROR_OK)
    return error;

  error = metadata->GetResourceEntryById(*local_id, entry);
  if (error != FILE_ERROR_OK)
    return error;

  return cache->Store(*local_id, std::string(), local_src_path,
                      internal::FileCache::FILE_OPERATION_COPY);
}

// Gets the file size of the |local_path|, and the ResourceEntry for the parent
// of |remote_path| to prepare the necessary information for transfer.
FileError PrepareTransferFileFromLocalToRemote(
    internal::ResourceMetadata* metadata,
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    std::string* gdoc_resource_id,
    ResourceEntry* parent_entry) {
  FileError error = metadata->GetResourceEntryByPath(
      remote_dest_path.DirName(), parent_entry);
  if (error != FILE_ERROR_OK)
    return error;

  // The destination's parent must be a directory.
  if (!parent_entry->file_info().is_directory())
    return FILE_ERROR_NOT_A_DIRECTORY;

  // Try to parse GDoc File and extract the resource id, if necessary.
  // Failing isn't problem. It'd be handled as a regular file, then.
  if (util::HasHostedDocumentExtension(local_src_path))
    *gdoc_resource_id = util::ReadResourceIdFromGDocFile(local_src_path);
  return FILE_ERROR_OK;
}

// Performs local work before server-side work for transferring JSON-represented
// gdoc files.
FileError LocalWorkForTransferJsonGdocFile(
    internal::ResourceMetadata* metadata,
    CopyOperation::TransferJsonGdocParams* params) {
  std::string local_id;
  FileError error = metadata->GetIdByResourceId(params->resource_id, &local_id);
  if (error != FILE_ERROR_OK) {
    params->location_type = NOT_IN_METADATA;
    return error == FILE_ERROR_NOT_FOUND ? FILE_ERROR_OK : error;
  }

  ResourceEntry entry;
  error = metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  params->local_id = entry.local_id();

  if (entry.parent_local_id() == util::kDriveOtherDirLocalId) {
    params->location_type = IS_ORPHAN;
    entry.set_title(params->new_title);
    entry.set_parent_local_id(params->parent_local_id);
    entry.set_metadata_edit_state(ResourceEntry::DIRTY);
    entry.set_modification_date(base::Time::Now().ToInternalValue());
    error = metadata->RefreshEntry(entry);
    if (error != FILE_ERROR_OK)
      return error;
    return metadata->GetFilePath(local_id, &params->changed_path);
  }

  params->location_type = HAS_PARENT;
  return FILE_ERROR_OK;
}

}  // namespace

CopyOperation::CopyOperation(base::SequencedTaskRunner* blocking_task_runner,
                             OperationDelegate* delegate,
                             JobScheduler* scheduler,
                             internal::ResourceMetadata* metadata,
                             internal::FileCache* cache)
  : blocking_task_runner_(blocking_task_runner),
    delegate_(delegate),
    scheduler_(scheduler),
    metadata_(metadata),
    cache_(cache),
    create_file_operation_(new CreateFileOperation(blocking_task_runner,
                                                   delegate,
                                                   metadata)),
    weak_ptr_factory_(this) {
}

CopyOperation::~CopyOperation() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void CopyOperation::Copy(const base::FilePath& src_file_path,
                         const base::FilePath& dest_file_path,
                         bool preserve_last_modified,
                         const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  CopyParams* params = new CopyParams;
  params->src_file_path = src_file_path;
  params->dest_file_path = dest_file_path;
  params->preserve_last_modified = preserve_last_modified;
  params->callback = callback;

  std::vector<std::string>* updated_local_ids = new std::vector<std::string>;
  bool* directory_changed = new bool(false);
  bool* should_copy_on_server = new bool(false);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&TryToCopyLocally, metadata_, cache_, params,
                 updated_local_ids, directory_changed, should_copy_on_server),
      base::Bind(&CopyOperation::CopyAfterTryToCopyLocally,
                 weak_ptr_factory_.GetWeakPtr(), base::Owned(params),
                 base::Owned(updated_local_ids), base::Owned(directory_changed),
                 base::Owned(should_copy_on_server)));
}

void CopyOperation::CopyAfterTryToCopyLocally(
    const CopyParams* params,
    const std::vector<std::string>* updated_local_ids,
    const bool* directory_changed,
    const bool* should_copy_on_server,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(params->callback);

  for (const auto& id : *updated_local_ids) {
    // Syncing for copy should be done in background, so pass the BACKGROUND
    // context. See: crbug.com/420278.
    delegate_->OnEntryUpdatedByOperation(ClientContext(BACKGROUND), id);
  }

  if (*directory_changed) {
    FileChange changed_file;
    DCHECK(!params->src_entry.file_info().is_directory());
    changed_file.Update(params->dest_file_path, FileChange::FILE_TYPE_FILE,
                        FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
    delegate_->OnFileChangedByOperation(changed_file);
  }

  if (error != FILE_ERROR_OK || !*should_copy_on_server) {
    params->callback.Run(error);
    return;
  }

  if (params->parent_entry.resource_id().empty()) {
    // Parent entry may be being synced.
    const bool waiting = delegate_->WaitForSyncComplete(
        params->parent_entry.local_id(),
        base::Bind(&CopyOperation::CopyAfterParentSync,
                   weak_ptr_factory_.GetWeakPtr(), *params));
    if (!waiting)
      params->callback.Run(FILE_ERROR_NOT_FOUND);
  } else {
    CopyAfterGetParentResourceId(*params, &params->parent_entry, FILE_ERROR_OK);
  }
}

void CopyOperation::CopyAfterParentSync(const CopyParams& params,
                                        FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(params.callback);

  if (error != FILE_ERROR_OK) {
    params.callback.Run(error);
    return;
  }

  ResourceEntry* parent = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&internal::ResourceMetadata::GetResourceEntryById,
                 base::Unretained(metadata_),
                 params.parent_entry.local_id(),
                 parent),
      base::Bind(&CopyOperation::CopyAfterGetParentResourceId,
                 weak_ptr_factory_.GetWeakPtr(),
                 params,
                 base::Owned(parent)));
}

void CopyOperation::CopyAfterGetParentResourceId(const CopyParams& params,
                                                 const ResourceEntry* parent,
                                                 FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(params.callback);

  if (error != FILE_ERROR_OK) {
    params.callback.Run(error);
    return;
  }

  base::FilePath new_title = params.dest_file_path.BaseName();
  if (params.src_entry.file_specific_info().is_hosted_document()) {
    // Drop the document extension, which should not be in the title.
    // TODO(yoshiki): Remove this code with crbug.com/223304.
    new_title = new_title.RemoveExtension();
  }

  base::Time last_modified =
      params.preserve_last_modified ?
      base::Time::FromInternalValue(
          params.src_entry.file_info().last_modified()) : base::Time();

  CopyResourceOnServer(
      params.src_entry.resource_id(), parent->resource_id(),
      new_title.AsUTF8Unsafe(), last_modified, params.callback);
}

void CopyOperation::TransferFileFromLocalToRemote(
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  std::string* gdoc_resource_id = new std::string;
  ResourceEntry* parent_entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(
          &PrepareTransferFileFromLocalToRemote,
          metadata_, local_src_path, remote_dest_path,
          gdoc_resource_id, parent_entry),
      base::Bind(
          &CopyOperation::TransferFileFromLocalToRemoteAfterPrepare,
          weak_ptr_factory_.GetWeakPtr(),
          local_src_path, remote_dest_path, callback,
          base::Owned(gdoc_resource_id), base::Owned(parent_entry)));
}

void CopyOperation::TransferFileFromLocalToRemoteAfterPrepare(
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    const FileOperationCallback& callback,
    std::string* gdoc_resource_id,
    ResourceEntry* parent_entry,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error);
    return;
  }

  // For regular files, schedule the transfer.
  if (gdoc_resource_id->empty()) {
    ScheduleTransferRegularFile(local_src_path, remote_dest_path, callback);
    return;
  }

  // GDoc file may contain a resource ID in the old format.
  const std::string canonicalized_resource_id =
      util::CanonicalizeResourceId(*gdoc_resource_id);

  // Drop the document extension, which should not be in the title.
  // TODO(yoshiki): Remove this code with crbug.com/223304.
  const std::string new_title =
      remote_dest_path.BaseName().RemoveExtension().AsUTF8Unsafe();

  // This is uploading a JSON file representing a hosted document.
  TransferJsonGdocParams* params = new TransferJsonGdocParams(
      callback, canonicalized_resource_id, *parent_entry, new_title);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&LocalWorkForTransferJsonGdocFile, metadata_, params),
      base::Bind(&CopyOperation::TransferJsonGdocFileAfterLocalWork,
                 weak_ptr_factory_.GetWeakPtr(), base::Owned(params)));
}

void CopyOperation::TransferJsonGdocFileAfterLocalWork(
    TransferJsonGdocParams* params,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != FILE_ERROR_OK) {
    params->callback.Run(error);
    return;
  }

  switch (params->location_type) {
    // When |resource_id| is found in the local metadata and it has a specific
    // parent folder, we assume the user's intention is to copy the document and
    // thus perform the server-side copy operation.
    case HAS_PARENT:
      CopyResourceOnServer(params->resource_id,
                           params->parent_resource_id,
                           params->new_title,
                           base::Time(),
                           params->callback);
      break;
    // When |resource_id| has no parent, we just set the new destination folder
    // as the parent, for sharing the document between the original source.
    // This reparenting is already done in LocalWorkForTransferJsonGdocFile().
    case IS_ORPHAN: {
      DCHECK(!params->changed_path.empty());
      // Syncing for copy should be done in background, so pass the BACKGROUND
      // context. See: crbug.com/420278.
      delegate_->OnEntryUpdatedByOperation(ClientContext(BACKGROUND),
                                           params->local_id);

      FileChange changed_file;
      changed_file.Update(
          params->changed_path,
          FileChange::FILE_TYPE_FILE,  // This must be a hosted document.
          FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
      delegate_->OnFileChangedByOperation(changed_file);
      params->callback.Run(error);
      break;
    }
    // When the |resource_id| is not in the local metadata, assume it to be a
    // document just now shared on the server but not synced locally.
    // Same as the IS_ORPHAN case, we want to deal the case by setting parent,
    // but this time we need to resort to server side operation.
    case NOT_IN_METADATA:
      scheduler_->UpdateResource(
          params->resource_id, params->parent_resource_id, params->new_title,
          base::Time(), base::Time(), google_apis::drive::Properties(),
          ClientContext(USER_INITIATED),
          base::Bind(&CopyOperation::UpdateAfterServerSideOperation,
                     weak_ptr_factory_.GetWeakPtr(), params->callback));
      break;
  }
}

void CopyOperation::CopyResourceOnServer(
    const std::string& resource_id,
    const std::string& parent_resource_id,
    const std::string& new_title,
    const base::Time& last_modified,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  scheduler_->CopyResource(
      resource_id, parent_resource_id, new_title, last_modified,
      base::Bind(&CopyOperation::UpdateAfterServerSideOperation,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

void CopyOperation::UpdateAfterServerSideOperation(
    const FileOperationCallback& callback,
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::FileResource> entry) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  FileError error = GDataToFileError(status);
  if (error != FILE_ERROR_OK) {
    callback.Run(error);
    return;
  }

  ResourceEntry* resource_entry = new ResourceEntry;

  // The copy on the server side is completed successfully. Update the local
  // metadata.
  base::FilePath* file_path = new base::FilePath;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&UpdateLocalStateForServerSideOperation, metadata_,
                     std::move(entry), resource_entry, file_path),
      base::BindOnce(&CopyOperation::UpdateAfterLocalStateUpdate,
                     weak_ptr_factory_.GetWeakPtr(), callback,
                     base::Owned(file_path), base::Owned(resource_entry)));
}

void CopyOperation::UpdateAfterLocalStateUpdate(
    const FileOperationCallback& callback,
    base::FilePath* file_path,
    const ResourceEntry* entry,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error == FILE_ERROR_OK) {
    FileChange changed_file;
    changed_file.Update(*file_path, *entry,
                        FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
    delegate_->OnFileChangedByOperation(changed_file);
  }
  callback.Run(error);
}

void CopyOperation::ScheduleTransferRegularFile(
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  create_file_operation_->CreateFile(
      remote_dest_path,
      false,  // Not exclusive (OK even if a file already exists).
      std::string(),  // no specific mime type; CreateFile should guess it.
      base::Bind(&CopyOperation::ScheduleTransferRegularFileAfterCreate,
                 weak_ptr_factory_.GetWeakPtr(),
                 local_src_path, remote_dest_path, callback));
}

void CopyOperation::ScheduleTransferRegularFileAfterCreate(
    const base::FilePath& local_src_path,
    const base::FilePath& remote_dest_path,
    const FileOperationCallback& callback,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error != FILE_ERROR_OK) {
    callback.Run(error);
    return;
  }

  std::string* local_id = new std::string;
  ResourceEntry* entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&UpdateLocalStateForScheduleTransfer,
                 metadata_,
                 cache_,
                 local_src_path,
                 remote_dest_path,
                 entry,
                 local_id),
      base::Bind(
          &CopyOperation::ScheduleTransferRegularFileAfterUpdateLocalState,
          weak_ptr_factory_.GetWeakPtr(),
          callback,
          remote_dest_path,
          base::Owned(entry),
          base::Owned(local_id)));
}

void CopyOperation::ScheduleTransferRegularFileAfterUpdateLocalState(
    const FileOperationCallback& callback,
    const base::FilePath& remote_dest_path,
    const ResourceEntry* entry,
    std::string* local_id,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (error == FILE_ERROR_OK) {
    FileChange changed_file;
    changed_file.Update(remote_dest_path, *entry,
                        FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
    delegate_->OnFileChangedByOperation(changed_file);
    // Syncing for copy should be done in background, so pass the BACKGROUND
    // context. See: crbug.com/420278.
    delegate_->OnEntryUpdatedByOperation(ClientContext(BACKGROUND), *local_id);
  }
  callback.Run(error);
}

}  // namespace file_system
}  // namespace drive
