// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_file_system_id.h"

#include <string>

#include "base/files/file_path.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"

namespace chromeos {
namespace smb_client {
namespace {

constexpr char kDelimiter[] = "@@";
constexpr size_t kDelimiterLen = arraysize(kDelimiter) - 1;

size_t GetDelimLocation(const std::string& file_system_id) {
  const size_t delim_location = file_system_id.find(kDelimiter);
  DCHECK(delim_location != std::string::npos);

  return delim_location;
}

}  // namespace.

std::string CreateFileSystemId(int32_t mount_id,
                               const base::FilePath& share_path) {
  return base::StrCat(
      {base::NumberToString(mount_id), kDelimiter, share_path.value()});
}

int32_t GetMountIdFromFileSystemId(const std::string& file_system_id) {
  const size_t delim_location = GetDelimLocation(file_system_id);

  const std::string mount_id_string = file_system_id.substr(0, delim_location);

  int32_t mount_id;
  bool result = base::StringToInt(mount_id_string, &mount_id);
  DCHECK(result);

  return mount_id;
}

base::FilePath GetSharePathFromFileSystemId(const std::string& file_system_id) {
  const size_t delim_location = GetDelimLocation(file_system_id);
  const size_t share_path_start = delim_location + kDelimiterLen;

  return base::FilePath(file_system_id.substr(share_path_start));
}

}  // namespace smb_client
}  // namespace chromeos
