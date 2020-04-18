// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/installer_util_test_common.h"

#include <windows.h>
#include <shellapi.h>

#include "base/files/file_path.h"
#include "base/strings/string16.h"

namespace installer {

namespace test {

bool CopyFileHierarchy(const base::FilePath& from, const base::FilePath& to) {
  // In SHFILEOPSTRUCT below, |pFrom| and |pTo| have to be double-null
  // terminated: http://msdn.microsoft.com/library/bb759795.aspx
  base::string16 double_null_from(from.value());
  double_null_from.push_back(L'\0');
  base::string16 double_null_to(to.value());
  double_null_to.push_back(L'\0');

  SHFILEOPSTRUCT file_op = {};
  file_op.wFunc = FO_COPY;
  file_op.pFrom = double_null_from.c_str();
  file_op.pTo = double_null_to.c_str();
  file_op.fFlags = FOF_NO_UI;

  return (SHFileOperation(&file_op) == 0 && !file_op.fAnyOperationsAborted);
}

}  // namespace test

}  // namespace installer
