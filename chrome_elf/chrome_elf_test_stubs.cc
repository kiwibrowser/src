// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/common/chrome_switches.h"
#include "chrome_elf/chrome_elf_main.h"

// This function is a temporary workaround for https://crbug.com/655788. We
// need to come up with a better way to initialize crash reporting that can
// happen inside DllMain().
void SignalInitializeCrashReporting() {}

void SignalChromeElf() {}

bool GetUserDataDirectoryThunk(wchar_t* user_data_dir,
                               size_t user_data_dir_length,
                               wchar_t* invalid_user_data_dir,
                               size_t invalid_user_data_dir_length) {
  // In tests, just respect the user-data-dir switch if given.
  base::FilePath user_data_dir_path =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
          switches::kUserDataDir);
  if (!user_data_dir_path.empty() && user_data_dir_path.EndsWithSeparator())
    user_data_dir_path = user_data_dir_path.StripTrailingSeparators();

  wcsncpy_s(user_data_dir, user_data_dir_length,
            user_data_dir_path.value().c_str(), _TRUNCATE);
  wcsncpy_s(invalid_user_data_dir, invalid_user_data_dir_length, L"",
            _TRUNCATE);

  return !user_data_dir_path.empty();
}

void SetMetricsClientId(const char* client_id) {}
