// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/importer/safari_importer_utils.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "chrome/common/importer/importer_data_types.h"

bool SafariImporterCanImport(const base::FilePath& library_dir,
                             uint16_t* services_supported) {
  DCHECK(services_supported);
  *services_supported = importer::NONE;

  // Import features are toggled by the following:
  // bookmarks import: existence of ~/Library/Safari/Bookmarks.plist file.
  // history import: existence of ~/Library/Safari/History.plist file.
  base::FilePath safari_dir = library_dir.Append("Safari");
  base::FilePath bookmarks_path = safari_dir.Append("Bookmarks.plist");
  base::FilePath history_path = safari_dir.Append("History.plist");

  if (base::PathExists(bookmarks_path))
    *services_supported |= importer::FAVORITES;
  if (base::PathExists(history_path))
    *services_supported |= importer::HISTORY;

  return *services_supported != importer::NONE;
}
