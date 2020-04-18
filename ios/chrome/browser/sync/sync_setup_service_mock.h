// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_MOCK_H_
#define IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_MOCK_H_

#include "ios/chrome/browser/sync/sync_setup_service.h"

#include "testing/gmock/include/gmock/gmock.h"

// Mock for the class that allows configuring sync on iOS.
class SyncSetupServiceMock : public SyncSetupService {
 public:
  SyncSetupServiceMock(syncer::SyncService* sync_service, PrefService* prefs);
  ~SyncSetupServiceMock();

  MOCK_CONST_METHOD0(IsSyncEnabled, bool());

  MOCK_CONST_METHOD0(IsSyncingAllDataTypes, bool());

  MOCK_METHOD0(GetSyncServiceState, SyncServiceState());

  MOCK_CONST_METHOD1(IsDataTypePreferred, bool(syncer::ModelType));

  MOCK_CONST_METHOD1(IsDataTypeActive, bool(syncer::ModelType));

  MOCK_METHOD0(HasFinishedInitialSetup, bool());

  MOCK_METHOD0(PrepareForFirstSyncSetup, void());

  // Allow the real SyncSetupService::HasFinishedInitialSetup() to be used when
  // mocking HasFinishedInitialSetup().
  bool SyncSetupServiceHasFinishedInitialSetup();
};

#endif  // IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_MOCK_H_
