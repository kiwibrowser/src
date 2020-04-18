// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/sync_setup_service_mock.h"

SyncSetupServiceMock::SyncSetupServiceMock(syncer::SyncService* sync_service,
                                           PrefService* prefs)
    : SyncSetupService(sync_service, prefs) {}

SyncSetupServiceMock::~SyncSetupServiceMock() {
}

bool SyncSetupServiceMock::SyncSetupServiceHasFinishedInitialSetup() {
  return SyncSetupService::HasFinishedInitialSetup();
}
