// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/remove_operation.h"

#include "base/sequenced_task_runner.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {

namespace {

// Removes cache file and moves the metadata entry to the trash.
FileError UpdateLocalState(internal::ResourceMetadata* metadata,
                           internal::FileCache* cache,
                           const base::FilePath& path,
                           bool is_recursive,
                           std::string* local_id,
                           ResourceEntry* entry,
                           base::FilePath* changed_path) {
  FileError error = metadata->GetIdByPath(path, local_id);
  if (error != FILE_ERROR_OK)
    return error;

  error = metadata->GetResourceEntryById(*local_id, entry);
  if (error != FILE_ERROR_OK)
    return error;

  if (entry->file_info().is_directory() && !is_recursive) {
    // Check emptiness of the directory.
    ResourceEntryVector entries;
    error = metadata->ReadDirectoryByPath(path, &entries);
    if (error != FILE_ERROR_OK)
      return error;
    if (!entries.empty())
      return FILE_ERROR_NOT_EMPTY;
  }

  error = cache->Remove(*local_id);
  if (error != FILE_ERROR_OK)
    return error;

  *changed_path = path;

  // Move to the trash.
  entry->set_parent_local_id(util::kDriveTrashDirLocalId);
  return metadata->RefreshEntry(*entry);
}

}  // namespace

RemoveOperation::RemoveOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    internal::ResourceMetadata* metadata,
    internal::FileCache* cache)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      metadata_(metadata),
      cache_(cache),
      weak_ptr_factory_(this) {
}

RemoveOperation::~RemoveOperation() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void RemoveOperation::Remove(const base::FilePath& path,
                             bool is_recursive,
                             const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  std::string* local_id = new std::string;
  base::FilePath* changed_path = new base::FilePath;
  ResourceEntry* entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&UpdateLocalState,
                 metadata_,
                 cache_,
                 path,
                 is_recursive,
                 local_id,
                 entry,
                 changed_path),
      base::Bind(&RemoveOperation::RemoveAfterUpdateLocalState,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback,
                 base::Owned(local_id),
                 base::Owned(entry),
                 base::Owned(changed_path)));
}

void RemoveOperation::RemoveAfterUpdateLocalState(
    const FileOperationCallback& callback,
    const std::string* local_id,
    const ResourceEntry* entry,
    const base::FilePath* changed_path,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (!changed_path->empty()) {
    FileChange changed_file;
    changed_file.Update(*changed_path, *entry, FileChange::CHANGE_TYPE_DELETE);
    if (error == FILE_ERROR_OK) {
      delegate_->OnFileChangedByOperation(changed_file);
      delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED),
                                           *local_id);
    }
  }

  callback.Run(error);
}

}  // namespace file_system
}  // namespace drive
