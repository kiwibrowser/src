// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/set_property_operation.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace file_system {

namespace {

// Adds the property to resource entry. Overwrites existing property if exists.
// If no change has been made (same key, visibility and value is already added)
// then FILE_ERROR_EXISTS is returned.
FileError UpdateLocalState(internal::ResourceMetadata* metadata,
                           const base::FilePath& file_path,
                           google_apis::drive::Property::Visibility visibility,
                           const std::string& key,
                           const std::string& value,
                           ResourceEntry* entry) {
  using google_apis::drive::Property;
  FileError error = metadata->GetResourceEntryByPath(file_path, entry);
  if (error != FILE_ERROR_OK)
    return error;

  Property_Visibility proto_visibility = Property_Visibility_PRIVATE;
  switch (visibility) {
    case Property::VISIBILITY_PRIVATE:
      proto_visibility = Property_Visibility_PRIVATE;
      break;
    case Property::VISIBILITY_PUBLIC:
      proto_visibility = Property_Visibility_PUBLIC;
      break;
  }

  ::drive::Property* property_to_update = nullptr;
  for (auto& property : *entry->mutable_new_properties()) {
    if (property.visibility() == proto_visibility && property.key() == key) {
      // Exactly the same property exists, so don't update the local state.
      if (property.value() == value)
        return FILE_ERROR_EXISTS;
      property_to_update = &property;
      break;
    }
  }

  // If no property to update has been found, then add a new one.
  if (!property_to_update)
    property_to_update = entry->mutable_new_properties()->Add();

  property_to_update->set_visibility(proto_visibility);
  property_to_update->set_key(key);
  property_to_update->set_value(value);
  entry->set_metadata_edit_state(ResourceEntry::DIRTY);
  entry->set_modification_date(base::Time::Now().ToInternalValue());

  return metadata->RefreshEntry(*entry);
}

}  // namespace

SetPropertyOperation::SetPropertyOperation(
    base::SequencedTaskRunner* blocking_task_runner,
    OperationDelegate* delegate,
    internal::ResourceMetadata* metadata)
    : blocking_task_runner_(blocking_task_runner),
      delegate_(delegate),
      metadata_(metadata),
      weak_ptr_factory_(this) {
}

SetPropertyOperation::~SetPropertyOperation() = default;

void SetPropertyOperation::SetProperty(
    const base::FilePath& file_path,
    google_apis::drive::Property::Visibility visibility,
    const std::string& key,
    const std::string& value,
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  ResourceEntry* entry = new ResourceEntry;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&UpdateLocalState, metadata_, file_path, visibility, key,
                 value, entry),
      base::Bind(&SetPropertyOperation::SetPropertyAfterUpdateLocalState,
                 weak_ptr_factory_.GetWeakPtr(), callback, base::Owned(entry)));
}

void SetPropertyOperation::SetPropertyAfterUpdateLocalState(
    const FileOperationCallback& callback,
    const ResourceEntry* entry,
    FileError result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  if (result == FILE_ERROR_OK) {
    // Do not notify about the file change, as properties are write only and
    // cannot be read, so there is no visible change.
    delegate_->OnEntryUpdatedByOperation(ClientContext(USER_INITIATED),
                                         entry->local_id());
  }

  // Even if exists, return success, as the set property operation always
  // overwrites existing values.
  callback.Run(result == FILE_ERROR_EXISTS ? FILE_ERROR_OK : result);
}

}  // namespace file_system
}  // namespace drive
