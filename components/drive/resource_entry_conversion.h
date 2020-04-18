// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_RESOURCE_ENTRY_CONVERSION_H_
#define COMPONENTS_DRIVE_RESOURCE_ENTRY_CONVERSION_H_

#include <string>

#include "base/files/file.h"

namespace google_apis {
class ChangeResource;
class FileResource;
class TeamDriveResource;
}

namespace drive {

class ResourceEntry;

// Converts a google_apis::ChangeResource into a drive::ResourceEntry.
// If the conversion succeeded, return true and sets the result to |out_entry|.
// |out_parent_resource_id| will be set to the resource ID of the parent entry.
// If failed, it returns false and keeps output arguments untouched.
//
// Every entry is guaranteed to have one parent resource ID in ResourceMetadata.
// This requirement is needed to represent contents in Drive as a file system
// tree, and achieved as follows:
//
// 1) Entries without parents are allowed on drive.google.com. These entries are
// collected to "drive/other", and have "drive/other" as the parent.
//
// 2) Entries with multiple parents are allowed on drive.google.com. For these
// entries, the first parent is chosen.
bool ConvertChangeResourceToResourceEntry(
    const google_apis::ChangeResource& input,
    ResourceEntry* out_entry,
    std::string* out_parent_resource_id);

// Converts a google_apis::FileResource into a drive::ResourceEntry.
// Also see the comment for ConvertChangeResourceToResourceEntry above.
bool ConvertFileResourceToResourceEntry(
    const google_apis::FileResource& input,
    ResourceEntry* out_entry,
    std::string* out_parent_resource_id);

// Converts a TeamDriveResource into a drive::ResourceEntry.
void ConvertTeamDriveResourceToResourceEntry(
    const google_apis::TeamDriveResource& input,
    ResourceEntry* out_entry);

// Converts the resource entry to the platform file info.
void ConvertResourceEntryToFileInfo(const ResourceEntry& entry,
                                    base::File::Info* file_info);

}  // namespace drive

#endif  // COMPONENTS_DRIVE_RESOURCE_ENTRY_CONVERSION_H_
