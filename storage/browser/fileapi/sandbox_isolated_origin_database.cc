// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/sandbox_isolated_origin_database.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "storage/browser/fileapi/sandbox_origin_database.h"

namespace storage {

// Special directory name for isolated origin.
const base::FilePath::CharType
SandboxIsolatedOriginDatabase::kObsoleteOriginDirectory[] =
    FILE_PATH_LITERAL("iso");

SandboxIsolatedOriginDatabase::SandboxIsolatedOriginDatabase(
    const std::string& origin,
    const base::FilePath& file_system_directory,
    const base::FilePath& origin_directory)
    : migration_checked_(false),
      origin_(origin),
      file_system_directory_(file_system_directory),
      origin_directory_(origin_directory) {
}

SandboxIsolatedOriginDatabase::~SandboxIsolatedOriginDatabase() = default;

bool SandboxIsolatedOriginDatabase::HasOriginPath(
    const std::string& origin) {
  return (origin_ == origin);
}

bool SandboxIsolatedOriginDatabase::GetPathForOrigin(
    const std::string& origin, base::FilePath* directory) {
  if (origin != origin_)
    return false;
  *directory = origin_directory_;
  return true;
}

bool SandboxIsolatedOriginDatabase::RemovePathForOrigin(
    const std::string& origin) {
  return true;
}

bool SandboxIsolatedOriginDatabase::ListAllOrigins(
    std::vector<OriginRecord>* origins) {
  origins->push_back(OriginRecord(origin_, origin_directory_));
  return true;
}

void SandboxIsolatedOriginDatabase::DropDatabase() {
}

void SandboxIsolatedOriginDatabase::MigrateBackFromObsoleteOriginDatabase(
    const std::string& origin,
    const base::FilePath& file_system_directory,
    SandboxOriginDatabase* database) {
  base::FilePath isolated_directory =
      file_system_directory.Append(kObsoleteOriginDirectory);

  if (database->HasOriginPath(origin)) {
    // Don't bother.
    base::DeleteFile(isolated_directory, true /* recursive */);
    return;
  }

  base::FilePath directory_name;
  if (database->GetPathForOrigin(origin, &directory_name)) {
    base::FilePath origin_directory =
        file_system_directory.Append(directory_name);
    base::DeleteFile(origin_directory, true /* recursive */);
    base::Move(isolated_directory, origin_directory);
  }
}

}  // namespace storage
