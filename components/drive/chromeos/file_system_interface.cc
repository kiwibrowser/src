// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system_interface.h"

namespace drive {

MetadataSearchResult::MetadataSearchResult(
    const base::FilePath& path,
    bool is_directory,
    const std::string& highlighted_base_name,
    const std::string& md5)
    : path(path),
      is_directory(is_directory),
      highlighted_base_name(highlighted_base_name),
      md5(md5) {
}

MetadataSearchResult::MetadataSearchResult(const MetadataSearchResult& other) =
    default;

}  // namespace drive
