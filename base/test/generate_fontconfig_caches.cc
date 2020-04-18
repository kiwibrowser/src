// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/fontconfig_util_linux.h"

int main(void) {
  base::SetUpFontconfig();
  base::TearDownFontconfig();

  base::FilePath dir_module;
  CHECK(base::PathService::Get(base::DIR_MODULE, &dir_module));
  base::FilePath fontconfig_caches = dir_module.Append("fontconfig_caches");
  CHECK(base::DirectoryExists(fontconfig_caches));
  base::FilePath stamp = fontconfig_caches.Append("STAMP");
  CHECK_EQ(0, base::WriteFile(stamp, "", 0));

  return 0;
}
