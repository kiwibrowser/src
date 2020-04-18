// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/first_run_internal.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/installer/util/master_preferences.h"

namespace first_run {
namespace internal {

bool IsOrganicFirstRun() {
  // We treat all installs as organic.
  return true;
}

base::FilePath MasterPrefsPath() {
  // The standard location of the master prefs is next to the chrome binary.
  base::FilePath master_prefs;
  if (!base::PathService::Get(base::DIR_EXE, &master_prefs))
    return base::FilePath();
  return master_prefs.AppendASCII(installer::kDefaultMasterPrefs);
}

}  // namespace internal
}  // namespace first_run
