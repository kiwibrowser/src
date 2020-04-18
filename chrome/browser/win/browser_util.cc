// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/browser_util.h"

#include <windows.h>

#include <algorithm>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"

namespace browser_util {

bool IsBrowserAlreadyRunning() {
  static HANDLE handle = NULL;
  base::FilePath exe_path;
  base::PathService::Get(base::FILE_EXE, &exe_path);
  std::wstring exe = exe_path.value();
  std::replace(exe.begin(), exe.end(), '\\', '!');
  std::transform(exe.begin(), exe.end(), exe.begin(), tolower);
  exe = L"Global\\" + exe;
  if (handle != NULL)
    CloseHandle(handle);
  handle = CreateEvent(NULL, TRUE, TRUE, exe.c_str());
  int error = GetLastError();
  return (error == ERROR_ALREADY_EXISTS || error == ERROR_ACCESS_DENIED);
}

}  // namespace browser_util
