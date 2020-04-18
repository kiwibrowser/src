// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/multi_client_status_change_checker.h"

#include "base/logging.h"
#include "components/browser_sync/profile_sync_service.h"

MultiClientStatusChangeChecker::MultiClientStatusChangeChecker(
    std::vector<browser_sync::ProfileSyncService*> services)
    : services_(services), scoped_observer_(this) {
  for (browser_sync::ProfileSyncService* service : services) {
    scoped_observer_.Add(service);
  }
}

MultiClientStatusChangeChecker::~MultiClientStatusChangeChecker() {}

void MultiClientStatusChangeChecker::OnStateChanged(syncer::SyncService* sync) {
  CheckExitCondition();
}

base::TimeDelta MultiClientStatusChangeChecker::GetTimeoutDuration() {
  // TODO(crbug.com/802025): This increased timeout seems to have become
  // necessary with kSyncUSSTypedURL. We should figure out why.
  return base::TimeDelta::FromSeconds(90);
}
