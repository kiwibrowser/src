// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_DRIVE_FILE_UTIL_H_
#define COMPONENTS_DRIVE_CHROMEOS_DRIVE_FILE_UTIL_H_

#include <string>

#include "components/drive/file_errors.h"

namespace drive {
namespace internal {

class ResourceMetadata;

// Obtains the start page token for the supplied |team_drive_id|. If the
// |team_drive_id| equals kTeamDriveIdDefaultCorpus then it will retrieve
// the start page token just for the users default corpus.
FileError GetStartPageToken(internal::ResourceMetadata* resource_metadata,
                            const std::string& team_drive_id,
                            std::string* out_value);

// Sets the start page token for the supplied |team_drive_id|. If the
// |team_drive_id| equals kTeamDriveIdDefaultCorpus then it will set
// the start page token just for the users default corpus.
FileError SetStartPageToken(internal::ResourceMetadata* resource_metadata,
                            const std::string& team_drive_id,
                            const std::string& value);

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_DRIVE_FILE_UTIL_H_
