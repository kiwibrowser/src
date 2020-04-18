// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_util.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "third_party/zlib/google/zip.h"

namespace feedback_util {

bool ZipString(const base::FilePath& filename,
               const std::string& data, std::string* compressed_logs) {
  base::FilePath temp_path;
  base::FilePath zip_file;

  // Create a temporary directory, put the logs into a file in it. Create
  // another temporary file to receive the zip file in.
  if (!base::CreateNewTempDirectory(base::FilePath::StringType(), &temp_path))
    return false;
  if (base::WriteFile(temp_path.Append(filename), data.c_str(), data.size()) ==
      -1) {
    return false;
  }

  bool succeed = base::CreateTemporaryFile(&zip_file) &&
      zip::Zip(temp_path, zip_file, false) &&
      base::ReadFileToString(zip_file, compressed_logs);

  base::DeleteFile(temp_path, true);
  base::DeleteFile(zip_file, false);

  return succeed;
}

}  // namespace feedback_util
