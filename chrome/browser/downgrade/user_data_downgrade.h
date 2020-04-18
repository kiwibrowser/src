// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNGRADE_USER_DATA_DOWNGRADE_H_
#define CHROME_BROWSER_DOWNGRADE_USER_DATA_DOWNGRADE_H_

#include "base/files/file_path.h"
#include "base/version.h"

namespace downgrade {

// The suffix of pending deleted directory.
extern const base::FilePath::CharType kDowngradeDeleteSuffix[];
// The name of "Last Version" file.
extern const base::FilePath::CharType kDowngradeLastVersionFile[];

// Moves aside all directories containing user data to prevent the browser from
// reading them when Chrome is downgraded from a higher version. When called
// early in startup, this will result in the browser performing first-run for
// the user. To avoid long-running I/O operations, this function merely moves
// directories aside. A subsequent call to DeleteMovedUserDataSoon (below) must
// be made later in startup to free up disk space.
void MoveUserDataForFirstRunAfterDowngrade();

// Update the content of "Last Version" file with current version number
// in |user_data_dir|.
void UpdateLastVersion(const base::FilePath& user_data_dir);

// Get the content of "Last Version" file in |user_data_dir|.
base::Version GetLastVersion(const base::FilePath& user_data_dir);

// Schedules a search for the removal of any directories moved aside by
// MoveUserDataForFirstRunAfterDowngrade. This operation is idempotent,
// and may be safely called when no such directories exist.
void DeleteMovedUserDataSoon();

// Returns true if Chrome is installed by MSI.
bool IsMSIInstall();

}  // namespace downgrade

#endif  // CHROME_BROWSER_DOWNGRADE_USER_DATA_DOWNGRADE_H_
