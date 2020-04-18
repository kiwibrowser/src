// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_SYNC_SERVICE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_SYNC_SERVICE_H_

#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace autofill {
class TestSyncService : public syncer::FakeSyncService {
 public:
  TestSyncService();
  ~TestSyncService() override;

  // FakeSyncService:
  bool CanSyncStart() const override;
  syncer::ModelTypeSet GetPreferredDataTypes() const override;
  bool IsEngineInitialized() const override;
  bool IsUsingSecondaryPassphrase() const override;
  bool IsSyncActive() const override;
  bool ConfigurationDone() const override;
  syncer::SyncCycleSnapshot GetLastCycleSnapshot() const override;
  const GoogleServiceAuthError& GetAuthError() const override;
  syncer::SyncTokenStatus GetSyncTokenStatus() const override;

  void SetCanSyncStart(bool can_sync_start) {
    can_sync_start_ = can_sync_start;
  }

  void SetPreferredDataTypes(syncer::ModelTypeSet preferred_data_types) {
    preferred_data_types_ = preferred_data_types;
  }

  void SetIsEngineInitialized(bool is_engine_initialized) {
    is_engine_initialized_ = is_engine_initialized;
  }

  void SetIsUsingSecondaryPassphrase(bool is_using_secondary_passphrase) {
    is_using_secondary_passphrase_ = is_using_secondary_passphrase;
  }

  void SetIsSyncActive(bool is_sync_active) {
    is_sync_active_ = is_sync_active;
  }

  void SetConfigurationDone(bool configuration_done) {
    configuration_done_ = configuration_done;
  }

  void SetSyncCycleComplete(bool complete) { sync_cycle_complete_ = complete; }

  void SetInAuthError(bool is_in_auth_error);

 private:
  bool can_sync_start_ = true;
  syncer::ModelTypeSet preferred_data_types_;
  bool is_engine_initialized_ = true;
  bool is_using_secondary_passphrase_ = false;
  bool is_sync_active_ = true;
  bool configuration_done_ = true;
  bool sync_cycle_complete_ = true;
  GoogleServiceAuthError auth_error_;
  bool is_in_auth_error_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestSyncService);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_SYNC_SERVICE_H_
