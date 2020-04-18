// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/create_directory_operation.h"

#include <stddef.h>

#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {

namespace {

FileError CreateDirectoryRecursively(internal::ResourceMetadata* metadata,
                                     const std::string& parent_local_id,
                                     const base::FilePath& relative_file_path,
                                     std::set<std::string>* updated_local_ids,
                                     FileChange* changed_files) {
  // Split the first component and remaining ones of |relative_file_path|.
  std::vector<base::FilePath::StringType> components;
  relative_file_path.GetComponents(&components);
  DCHECK(!components.empty());
  base::FilePath title(components[0]);
  base::FilePath remaining_path;
  title.AppendRelativePath(relative_file_path, &remaining_path);

  ResourceEntry entry;
  const int64_t now = base::Time::Now().ToInternalValue();
  entry.set_title(title.AsUTF8Unsafe());
  entry.mutable_file_info()->set_is_directory(true);
  entry.mutable_file_info()->set_last_modified(now);
  entry.set_last_modified_by_me(now);
  entry.mutable_file_info()->set_last_accessed(now);
  entry.set_parent_local_id(parent_local_id);
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);
  entry.set_modification_date(base::Time::Now().ToInternalValue());

  std::string local_id;
  FileError error = metadata->AddEntry(entry, &local_id);
  if (error != FILE_ERROR_OK)
    return error;

  base::FilePath path;
  error = metadata->GetFilePath(local_id, &path);
  if (error != FILE_ERROR_OK)
    return error;

  updated_local_ids->insert(local_id);
  DCHECK(changed_files);
  changed_files->Update(path, FileChange::FILE_TYPE_DIRECTORY,
                        FileChange::CHANGE_TYPE_ADD_OR_UPDATE);

  if (remaining_path.empty())  // All directories are created successfully.
    return FILE_ERROR_OK;

  // Create descendant directories.
  return CreateDirectoryRecursively(
      metadata, local_id, remaining_path, updated_local_ids, changed_files);
}

FileError UpdateLocalState(internal::ResourceMetadata* metadata,
                           const base::FilePath& directory_path,
                           bool is_exclusive,
                           bool is_recursive,
                           std::set<std::string>* updated_local_ids,
                           FileChange* changed_files) {
  // Get the existing deepest entry.
  std::vector<base::FilePath::StringType> components;
  directory_path.GetComponents(&components);

  if (components.empty() ||
      components[0] != util::GetDriveGrandRootPath().value())
    return FILE_ERROR_NOT_FOUND;

  base::FilePath existing_deepest_path(components[0]);
  std::string local_id = util::kDriveGrandRootLocalId;
  for (size_t i = 1; i < components.size(); ++i) {
    const std::string component = base::FilePath(components[i]).AsUTF8Unsafe();
    std::string child_local_id;
    FileError error =
        metadata->GetChildId(local_id, component, &child_local_id);
    if (error == FILE_ERROR_NOT_FOUND)
      break;
    if (error != FILE_ERROR_OK)
      return error;
    existing_deepest_path = existing_deepest_path.Append(components[i]);
    local_id = child_local_id;
  }

  ResourceEntry entry;
  FileError error = metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  if (!entry.file_info().is_directory())
    return FILE_ERROR_NOT_A_DIRECTORY;

  if (directory_path == existing_deepest_path)
    return is_exclusive ? FILE_ERROR_EXISTS : FILE_ERROR_OK;

  // If it is not recursive creation, the found directory must be the direct
  // parent of |directory_path| to ensure creating exact one directory.
  if (!is_recursive && existing_deepest_path != directory_path.DirName())
    return FILE_ERROR_NOT_FOUND;

  // Create directories under the found directory.
  base::FilePath remaining_path;
  existing_deepest_path.AppendRelativePath(directory_path, &remaining_path);
  return CreateDirectoryRecursively(metadata,
                                    entry.local_id(),
                                    remaining_path,
                                    updated_local_ids,
                                    changed_files);
}

}  // namespace

CreateDirectoryOperation::CreateDirectoryOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    internal::ResourceMetadata* metadata)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      metadata_(metadata),
      weak_ptr_factory_(this) {
}

CreateDirectoryOperation::~CreateDirectoryOperation() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void CreateDirectoryOperation::CreateDirectory(
    const base::FilePath& directory_path,
    bool is_exclusive,
    bool is_recursive,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  std::set<std::string>* updated_local_ids = new std::set<std::string>;
  FileChange* changed_files(new FileChange);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&UpdateLocalState,
                 metadata_,
                 directory_path,
                 is_exclusive,
                 is_recursive,
                 updated_local_ids,
                 changed_files),
      base::Bind(
          &CreateDirectoryOperation::CreateDirectoryAfterUpdateLocalState,
          weak_ptr_factory_.GetWeakPtr(),
          callback,
          base::Owned(updated_local_ids),
          base::Owned(changed_files)));
}

void CreateDirectoryOperation::CreateDirectoryAfterUpdateLocalState(
    const FileOperationCallback& callback,
    const std::set<std::string>* updated_local_ids,
    const FileChange* changed_files,
    FileError error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  for (const auto& id : *updated_local_ids) {
    delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED), id);
  }

  delegate_->OnFileChangedByOperation(*changed_files);

  callback.Run(error);
}

}  // namespace file_system
}  // namespace drive
