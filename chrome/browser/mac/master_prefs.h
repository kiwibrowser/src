// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MAC_MASTER_PREFS_H_
#define CHROME_BROWSER_MAC_MASTER_PREFS_H_

#include "base/files/file_path.h"

namespace master_prefs {

// Returns the path to the master preferences file. Note that this path may be
// empty (in the case where this type of build cannot have a master preferences
// file) or may not actually exist on the filesystem.
base::FilePath MasterPrefsPath();

}  // namespace master_prefs

#endif  // CHROME_BROWSER_MAC_MASTER_PREFS_H_
