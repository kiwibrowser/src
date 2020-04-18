// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/drive_pref_names.h"

namespace drive {
namespace prefs {

// A boolean pref to disable Google Drive integration.
// The pref prefix should remain as "gdata" for backward compatibility.
const char kDisableDrive[] = "gdata.disabled";

// A boolean pref to disable Drive over cellular connections.
// The pref prefix should remain as "gdata" for backward compatibility.
const char kDisableDriveOverCellular[] = "gdata.cellular.disabled";

// A boolean pref to disable hosted files on Drive.
// The pref prefix should remain as "gdata" for backward compatibility.
const char kDisableDriveHostedFiles[] = "gdata.hosted_files.disabled";

}  // namespace prefs
}  // namespace drive
