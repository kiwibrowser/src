// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/download/pass_kit_test_util.h"

#include "base/base_paths.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/path_service.h"

namespace testing {

std::string GetTestPass() {
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  const char kFilePath[] = "ios/testing/data/http_server_files/generic.pkpass";
  path = path.Append(FILE_PATH_LITERAL(kFilePath));
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  char pass_kit_data[file.GetLength()];
  file.ReadAtCurrentPos(pass_kit_data, file.GetLength());
  return std::string(pass_kit_data, file.GetLength());
}

}  // namespace testing
