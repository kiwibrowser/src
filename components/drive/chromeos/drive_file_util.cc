// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/drive_file_util.h"

#include <string>

#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_system_core_util.h"

namespace drive {
namespace internal {

FileError GetStartPageToken(internal::ResourceMetadata* resource_metadata,
                            const std::string& team_drive_id,
                            std::string* out_value) {
  DCHECK(resource_metadata);
  DCHECK(out_value);

  if (team_drive_id == util::kTeamDriveIdDefaultCorpus) {
    return resource_metadata->GetStartPageToken(out_value);
  }

  std::string local_id;
  FileError error =
      resource_metadata->GetIdByResourceId(team_drive_id, &local_id);
  if (error != FILE_ERROR_OK) {
    DLOG(ERROR) << "Failed to get team drive local id.";
    return error;
  }

  ResourceEntry entry;
  error = resource_metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK) {
    DLOG(ERROR) << "Filed to get the team drive resource.";
    return error;
  }

  DCHECK(entry.file_info().is_directory());
  DCHECK_EQ(entry.parent_local_id(), util::kDriveTeamDrivesDirLocalId);
  out_value->assign(entry.team_drive_root_specific_info().start_page_token());
  return FILE_ERROR_OK;
}

FileError SetStartPageToken(internal::ResourceMetadata* resource_metadata,
                            const std::string& team_drive_id,
                            const std::string& value) {
  DCHECK(resource_metadata);

  if (team_drive_id == util::kTeamDriveIdDefaultCorpus) {
    return resource_metadata->SetStartPageToken(value);
  }

  std::string local_id;
  FileError error =
      resource_metadata->GetIdByResourceId(team_drive_id, &local_id);
  if (error != FILE_ERROR_OK) {
    DLOG(ERROR) << "Failed to get team drive local id.";
    return error;
  }

  ResourceEntry entry;
  error = resource_metadata->GetResourceEntryById(local_id, &entry);
  if (error != FILE_ERROR_OK) {
    DLOG(ERROR) << "Failed to get the team drive resource.";
    return error;
  }

  DCHECK(entry.file_info().is_directory());
  DCHECK_EQ(entry.parent_local_id(), util::kDriveTeamDrivesDirLocalId);
  entry.mutable_team_drive_root_specific_info()->set_start_page_token(value);
  return resource_metadata->RefreshEntry(entry);
}

}  // namespace internal
}  // namespace drive
