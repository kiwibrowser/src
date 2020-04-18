// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_OBSERVER_H_
#define COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_OBSERVER_H_

#include <string>

namespace sync_preferences {

class SyncedPrefObserver {
 public:
  virtual void OnSyncedPrefChanged(const std::string& path, bool from_sync) = 0;
};

}  // namespace sync_preferences

#endif  // COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_OBSERVER_H_
