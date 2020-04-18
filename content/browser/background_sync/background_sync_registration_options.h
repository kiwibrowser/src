// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_REGISTRATION_OPTIONS_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_REGISTRATION_OPTIONS_H_

#include <stdint.h>

#include <string>

#include "content/browser/background_sync/background_sync.pb.h"
#include "content/common/content_export.h"

namespace content {

// The options passed to BackgroundSyncManager::Registration.
struct CONTENT_EXPORT BackgroundSyncRegistrationOptions {
  bool Equals(const BackgroundSyncRegistrationOptions& other) const;

  std::string tag;
  SyncNetworkState network_state = NETWORK_STATE_ONLINE;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_REGISTRATION_OPTIONS_H_
