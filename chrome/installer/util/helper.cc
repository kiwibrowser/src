// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/helper.h"

#include "base/path_service.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/util_constants.h"

namespace installer {

base::FilePath GetChromeInstallPath(bool system_install) {
  base::FilePath install_path;
#if defined(_WIN64)
  // TODO(wfh): Place Chrome binaries into DIR_PROGRAM_FILESX86 until the code
  // to support moving the binaries is added.
  int key =
      system_install ? base::DIR_PROGRAM_FILESX86 : base::DIR_LOCAL_APP_DATA;
#else
  int key = system_install ? base::DIR_PROGRAM_FILES : base::DIR_LOCAL_APP_DATA;
#endif
  if (base::PathService::Get(key, &install_path)) {
    install_path =
        install_path.Append(install_static::GetChromeInstallSubDirectory());
    install_path = install_path.Append(kInstallBinaryDir);
  }
  return install_path;
}

}  // namespace installer.
