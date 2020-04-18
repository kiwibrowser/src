// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/browser_sync_switches.h"

namespace switches {

// Disables syncing browser data to a Google Account.
const char kDisableSync[] = "disable-sync";

// Disables syncing one or more sync data types that are on by default.
// See sync/base/model_type.h for possible types. Types
// should be comma separated, and follow the naming convention for string
// representation of model types, e.g.:
// --disable-synctypes='Typed URLs, Bookmarks, Autofill Profiles'
const char kDisableSyncTypes[] = "disable-sync-types";

// Enables synchronizing WiFi credentials across devices, using Chrome Sync.
const char kEnableWifiCredentialSync[] = "enable-wifi-credential-sync";

// Enabled the local sync backend implemented by the LoopbackServer.
const char kEnableLocalSyncBackend[] = "enable-local-sync-backend";

// Specifies the local sync backend directory. The name is chosen to mimic
// user-data-dir etc. This flag only matters if the enable-local-sync-backend
// flag is present.
const char kLocalSyncBackendDir[] = "local-sync-backend-dir";

}  // namespace switches
