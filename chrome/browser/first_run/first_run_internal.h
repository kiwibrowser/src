// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FIRST_RUN_FIRST_RUN_INTERNAL_H_
#define CHROME_BROWSER_FIRST_RUN_FIRST_RUN_INTERNAL_H_

class Profile;

namespace base {
class FilePath;
}

namespace installer {
class MasterPreferences;
}

namespace first_run {

struct MasterPrefs;

namespace internal {

enum FirstRunState {
  FIRST_RUN_UNKNOWN,  // The state is not tested or set yet.
  FIRST_RUN_TRUE,
  FIRST_RUN_FALSE,
};

// Sets up master preferences by preferences passed by installer.
void SetupMasterPrefsFromInstallPrefs(
    const installer::MasterPreferences& install_prefs,
    MasterPrefs* out_prefs);

// Get the file path of the first run sentinel; returns false on failure.
bool GetFirstRunSentinelFilePath(base::FilePath* path);

// Create the first run sentinel file; returns false on failure.
bool CreateSentinel();

// -- Platform-specific functions --

void DoPostImportPlatformSpecificTasks(Profile* profile);

// Returns true if the sentinel file exists (or the path cannot be obtained).
bool IsFirstRunSentinelPresent();

// This function has a common implementationin for all non-linux platforms, and
// a linux specific implementation.
bool IsOrganicFirstRun();

// Shows the EULA dialog if required. Returns true if the EULA is accepted,
// returns false if the EULA has not been accepted, in which case the browser
// should exit.
bool ShowPostInstallEULAIfNeeded(installer::MasterPreferences* install_prefs);

// Returns the path for the master preferences file.
base::FilePath MasterPrefsPath();

// Helper for IsChromeFirstRun. Exposed for testing.
FirstRunState DetermineFirstRunState(bool has_sentinel,
                                     bool force_first_run,
                                     bool no_first_run);

}  // namespace internal
}  // namespace first_run

#endif  // CHROME_BROWSER_FIRST_RUN_FIRST_RUN_INTERNAL_H_
