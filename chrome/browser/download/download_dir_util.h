// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIR_UTIL_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIR_UTIL_H_

#include "base/files/file_path.h"

namespace policy {
struct PolicyHandlerParameters;
}  // namespace policy

namespace download_dir_util {

#if defined(OS_CHROMEOS)
// Returns whether |string_value| points to a directory in Drive or not.
bool DownloadToDrive(const base::FilePath::StringType& string_value,
                     const policy::PolicyHandlerParameters& parameters);
#endif  // defined(OS_CHROMEOS)

// Expands path variables in the download directory path |string_value|.
base::FilePath::StringType ExpandDownloadDirectoryPath(
    const base::FilePath::StringType& string_value,
    const policy::PolicyHandlerParameters& parameters);

}  // namespace download_dir_util

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIR_UTIL_H_
