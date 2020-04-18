// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/chrome_finder.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#include "base/win/windows_version.h"
#endif

namespace {

#if defined(OS_WIN)
void GetApplicationDirs(std::vector<base::FilePath>* locations) {
  std::vector<base::FilePath> installation_locations;
  base::FilePath local_app_data, program_files, program_files_x86;
  if (base::PathService::Get(base::DIR_LOCAL_APP_DATA, &local_app_data))
    installation_locations.push_back(local_app_data);
  if (base::PathService::Get(base::DIR_PROGRAM_FILES, &program_files))
    installation_locations.push_back(program_files);
  if (base::PathService::Get(base::DIR_PROGRAM_FILESX86, &program_files_x86))
    installation_locations.push_back(program_files_x86);

  for (size_t i = 0; i < installation_locations.size(); ++i) {
    locations->push_back(
        installation_locations[i].Append(L"Google\\Chrome\\Application"));
  }
  for (size_t i = 0; i < installation_locations.size(); ++i) {
    locations->push_back(
        installation_locations[i].Append(L"Chromium\\Application"));
  }
}
#elif defined(OS_LINUX)
void GetApplicationDirs(std::vector<base::FilePath>* locations) {
  // TODO: Respect users' PATH variables.
  // Until then, we use an approximation of the most common defaults.
  locations->push_back(base::FilePath("/usr/local/sbin"));
  locations->push_back(base::FilePath("/usr/local/bin"));
  locations->push_back(base::FilePath("/usr/sbin"));
  locations->push_back(base::FilePath("/usr/bin"));
  locations->push_back(base::FilePath("/sbin"));
  locations->push_back(base::FilePath("/bin"));
  // Lastly, try the default installation location.
  locations->push_back(base::FilePath("/opt/google/chrome"));
}
#elif defined(OS_ANDROID)
void GetApplicationDirs(std::vector<base::FilePath>* locations) {
  // On Android we won't be able to find Chrome executable
}
#endif

}  // namespace

namespace internal {

bool FindExe(
    const base::Callback<bool(const base::FilePath&)>& exists_func,
    const std::vector<base::FilePath>& rel_paths,
    const std::vector<base::FilePath>& locations,
    base::FilePath* out_path) {
  for (size_t i = 0; i < rel_paths.size(); ++i) {
    for (size_t j = 0; j < locations.size(); ++j) {
      base::FilePath path = locations[j].Append(rel_paths[i]);
      if (exists_func.Run(path)) {
        *out_path = path;
        return true;
      }
    }
  }
  return false;
}

}  // namespace internal

#if defined(OS_MACOSX)
void GetApplicationDirs(std::vector<base::FilePath>* locations);
#endif

bool FindChrome(base::FilePath* browser_exe) {
  base::FilePath browser_exes_array[] = {
#if defined(OS_WIN)
      base::FilePath(L"chrome.exe")
#elif defined(OS_MACOSX)
      base::FilePath("Google Chrome.app/Contents/MacOS/Google Chrome"),
      base::FilePath("Chromium.app/Contents/MacOS/Chromium")
#elif defined(OS_LINUX)
      base::FilePath("google-chrome"),
      base::FilePath("chrome"),
      base::FilePath("chromium"),
      base::FilePath("chromium-browser")
#else
      // it will compile but won't work on other OSes
      base::FilePath()
#endif
  };

  std::vector<base::FilePath> browser_exes(
      browser_exes_array, browser_exes_array + arraysize(browser_exes_array));
  base::FilePath module_dir;
  if (base::PathService::Get(base::DIR_MODULE, &module_dir)) {
    for (size_t i = 0; i < browser_exes.size(); ++i) {
      base::FilePath path = module_dir.Append(browser_exes[i]);
      if (base::PathExists(path)) {
        *browser_exe = path;
        return true;
      }
    }
  }

  std::vector<base::FilePath> locations;
  GetApplicationDirs(&locations);
  return internal::FindExe(
      base::Bind(&base::PathExists),
      browser_exes,
      locations,
      browser_exe);
}
