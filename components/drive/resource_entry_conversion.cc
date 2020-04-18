// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/resource_entry_conversion.h"

#include <stdint.h>

#include <string>

#include "base/logging.h"
#include "base/time/time.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "google_apis/drive/drive_api_parser.h"

namespace drive {

bool ConvertChangeResourceToResourceEntry(
    const google_apis::ChangeResource& input,
    ResourceEntry* out_entry,
    std::string* out_parent_resource_id) {
  DCHECK(out_entry);
  DCHECK(out_parent_resource_id);

  ResourceEntry converted;
  std::string parent_resource_id;
  if (input.type() == google_apis::ChangeResource::TEAM_DRIVE) {
    if (input.team_drive()) {
      ConvertTeamDriveResourceToResourceEntry(*input.team_drive(), &converted);
    } else {
      // resource_id is same as the Team Drive ID.
      // Both team_drive_id() and team_drive().id() holds the same ID, but the
      // latter doesn't exist for deleted items.
      converted.set_resource_id(input.team_drive_id());
      converted.mutable_file_info()->set_is_directory(true);
      converted.set_parent_local_id(util::kDriveTeamDrivesDirLocalId);
    }
  } else {
    if (input.file() && !ConvertFileResourceToResourceEntry(
                            *input.file(), &converted, &parent_resource_id))
      return false;
    converted.set_resource_id(input.file_id());
  }
  converted.set_deleted(converted.deleted() || input.is_deleted());
  converted.set_modification_date(input.modification_date().ToInternalValue());

  out_entry->Swap(&converted);
  swap(*out_parent_resource_id, parent_resource_id);
  return true;
}

bool ConvertFileResourceToResourceEntry(
    const google_apis::FileResource& input,
    ResourceEntry* out_entry,
    std::string* out_parent_resource_id) {
  DCHECK(out_entry);
  DCHECK(out_parent_resource_id);
  ResourceEntry converted;

  // For regular files, the 'filename' and 'title' attribute in the metadata
  // may be different (e.g. due to rename). To be consistent with the web
  // interface and other client to use the 'title' attribute, instead of
  // 'filename', as the file name in the local snapshot.
  converted.set_title(input.title());
  converted.set_base_name(util::NormalizeFileName(converted.title()));
  converted.set_resource_id(input.file_id());

  // Gets parent Resource ID. On drive.google.com, a file can have multiple
  // parents or no parent, but we are forcing a tree-shaped structure (i.e. no
  // multi-parent or zero-parent entries). Therefore the first found "parent" is
  // used for the entry. Tracked in http://crbug.com/158904.
  std::string parent_resource_id;
  if (!input.parents().empty())
    parent_resource_id = input.parents()[0].file_id();

  converted.set_deleted(input.labels().is_trashed());
  converted.set_starred(input.labels().is_starred());
  converted.set_shared_with_me(!input.shared_with_me_date().is_null());
  converted.set_shared(input.shared());
  if (!input.alternate_link().is_empty())
    converted.set_alternate_url(input.alternate_link().spec());

  PlatformFileInfoProto* file_info = converted.mutable_file_info();

  file_info->set_last_modified(input.modified_date().ToInternalValue());
  converted.set_last_modified_by_me(
      input.modified_by_me_date().ToInternalValue());
  // If the file has never been viewed (last_viewed_by_me_date().is_null() ==
  // true), then we will set the last_accessed field in the protocol buffer to
  // 0.
  file_info->set_last_accessed(
      input.last_viewed_by_me_date().ToInternalValue());
  file_info->set_creation_time(input.created_date().ToInternalValue());

  // Set the capabilities.
  const google_apis::FileResourceCapabilities& capabilities =
      input.capabilities();
  converted.mutable_capabilities_info()->set_can_copy(capabilities.can_copy());
  converted.mutable_capabilities_info()->set_can_delete(
      capabilities.can_delete());
  converted.mutable_capabilities_info()->set_can_rename(
      capabilities.can_rename());
  converted.mutable_capabilities_info()->set_can_add_children(
      capabilities.can_add_children());
  converted.mutable_capabilities_info()->set_can_share(
      capabilities.can_share());

  if (input.IsDirectory()) {
    file_info->set_is_directory(true);
  } else {
    FileSpecificInfo* file_specific_info =
        converted.mutable_file_specific_info();
    if (!input.IsHostedDocument()) {
      file_info->set_size(input.file_size());
      file_specific_info->set_md5(input.md5_checksum());
      file_specific_info->set_is_hosted_document(false);
    } else {
      // Attach .g<something> extension to hosted documents so we can special
      // case their handling in UI.
      // TODO(satorux): Figure out better way how to pass input info like kind
      // to UI through the File API stack.
      const std::string document_extension =
          drive::util::GetHostedDocumentExtension(input.mime_type());
      file_specific_info->set_document_extension(document_extension);
      converted.set_base_name(
          util::NormalizeFileName(converted.title() + document_extension));

      // We don't know the size of hosted docs and it does not matter since
      // it has no effect on the quota.
      file_info->set_size(0);
      file_specific_info->set_is_hosted_document(true);
    }
    file_info->set_is_directory(false);
    file_specific_info->set_content_mime_type(input.mime_type());

    const int64_t image_width = input.image_media_metadata().width();
    if (image_width != -1)
      file_specific_info->set_image_width(image_width);

    const int64_t image_height = input.image_media_metadata().height();
    if (image_height != -1)
      file_specific_info->set_image_height(image_height);

    const int64_t image_rotation = input.image_media_metadata().rotation();
    if (image_rotation != -1)
      file_specific_info->set_image_rotation(image_rotation);
  }

  out_entry->Swap(&converted);
  swap(*out_parent_resource_id, parent_resource_id);
  return true;
}

void ConvertTeamDriveResourceToResourceEntry(
    const google_apis::TeamDriveResource& input,
    ResourceEntry* out_entry) {
  DCHECK(out_entry);
  out_entry->mutable_file_info()->set_is_directory(true);
  out_entry->set_title(input.name());
  out_entry->set_base_name(input.name());
  out_entry->set_resource_id(input.id());
  out_entry->set_parent_local_id(util::kDriveTeamDrivesDirLocalId);

  // Set all the capabilities.
  const google_apis::TeamDriveCapabilities& capabilities = input.capabilities();
  out_entry->mutable_capabilities_info()->set_can_copy(capabilities.can_copy());
  out_entry->mutable_capabilities_info()->set_can_delete(
      capabilities.can_delete_team_drive());
  out_entry->mutable_capabilities_info()->set_can_rename(
      capabilities.can_rename_team_drive());
  out_entry->mutable_capabilities_info()->set_can_add_children(
      capabilities.can_add_children());
  out_entry->mutable_capabilities_info()->set_can_share(
      capabilities.can_share());
}

void ConvertResourceEntryToFileInfo(const ResourceEntry& entry,
                                    base::File::Info* file_info) {
  file_info->size = entry.file_info().size();
  file_info->is_directory = entry.file_info().is_directory();
  file_info->is_symbolic_link = entry.file_info().is_symbolic_link();
  file_info->last_modified = base::Time::FromInternalValue(
      entry.file_info().last_modified());
  file_info->last_accessed = base::Time::FromInternalValue(
      entry.file_info().last_accessed());
  file_info->creation_time = base::Time::FromInternalValue(
      entry.file_info().creation_time());
}

}  // namespace drive
