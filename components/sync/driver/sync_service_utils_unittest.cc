// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_service_utils.h"

#include <vector>
#include "components/sync/base/model_type.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class TestSyncService : public FakeSyncService {
 public:
  TestSyncService() = default;
  ~TestSyncService() override = default;

  void SetSyncAllowed(bool allowed) { sync_allowed_ = allowed; }
  void SetSyncActive(bool active) { sync_active_ = active; }
  void SetLocalSyncEnabled(bool local) { local_sync_enabled_ = local; }
  void SetPreferredDataTypes(const ModelTypeSet& types) {
    preferred_data_types_ = types;
  }
  void SetConfigurationDone(bool done) { configuration_done_ = done; }
  void SetCustomPassphraseEnabled(bool enabled) {
    custom_passphrase_enabled_ = enabled;
  }
  void SetSyncCycleComplete(bool complete) { sync_cycle_complete_ = complete; }

  // SyncService implementation.
  bool IsSyncAllowed() const override { return sync_allowed_; }
  bool CanSyncStart() const override { return sync_allowed_; }
  bool IsSyncActive() const override { return sync_active_; }
  bool IsLocalSyncEnabled() const override { return local_sync_enabled_; }
  ModelTypeSet GetPreferredDataTypes() const override {
    return preferred_data_types_;
  }
  ModelTypeSet GetActiveDataTypes() const override {
    if (!sync_active_)
      return ModelTypeSet();
    return preferred_data_types_;
  }
  ModelTypeSet GetEncryptedDataTypes() const override {
    if (!custom_passphrase_enabled_) {
      // PASSWORDS are always encrypted.
      return ModelTypeSet(syncer::PASSWORDS);
    }
    // Some types can never be encrypted, e.g. DEVICE_INFO and
    // AUTOFILL_WALLET_DATA, so make sure we don't report them as encrypted.
    return syncer::Intersection(preferred_data_types_,
                                syncer::EncryptableUserTypes());
  }
  SyncCycleSnapshot GetLastCycleSnapshot() const override {
    if (sync_cycle_complete_) {
      return SyncCycleSnapshot(
          ModelNeutralState(), ProgressMarkerMap(), false, 5, 2, 7, false, 0,
          base::Time::Now(), base::Time::Now(),
          std::vector<int>(MODEL_TYPE_COUNT, 0),
          std::vector<int>(MODEL_TYPE_COUNT, 0),
          sync_pb::SyncEnums::UNKNOWN_ORIGIN,
          /*short_poll_interval=*/base::TimeDelta::FromMinutes(30),
          /*long_poll_interval=*/base::TimeDelta::FromMinutes(180),
          /*has_remaining_local_changes=*/false);
    }
    return SyncCycleSnapshot();
  }
  bool ConfigurationDone() const override { return configuration_done_; }
  bool IsUsingSecondaryPassphrase() const override {
    return custom_passphrase_enabled_;
  }

 private:
  bool sync_allowed_ = false;
  bool sync_active_ = false;
  bool sync_cycle_complete_ = false;
  bool local_sync_enabled_ = false;
  ModelTypeSet preferred_data_types_;
  bool configuration_done_ = false;
  bool custom_passphrase_enabled_ = false;
};

TEST(SyncServiceUtilsTest, UploadToGoogleDisabledIfSyncNotAllowed) {
  TestSyncService service;

  // If sync is not allowed, uploading should never be enabled, even if
  // configuration is done and all the data types are enabled.
  service.SetSyncAllowed(false);

  service.SetConfigurationDone(true);
  service.SetPreferredDataTypes(ProtocolTypes());

  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // Once sync gets allowed (e.g. policy is updated), uploading should not be
  // disabled anymore (though not necessarily active yet).
  service.SetSyncAllowed(true);

  EXPECT_NE(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
}

TEST(SyncServiceUtilsTest,
     UploadToGoogleInitializingUntilConfiguredAndActiveAndSyncCycleComplete) {
  TestSyncService service;
  service.SetSyncAllowed(true);
  service.SetPreferredDataTypes(ProtocolTypes());

  // By default, if sync isn't disabled, we should be INITIALIZING.
  EXPECT_EQ(UploadState::INITIALIZING,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // Finished configuration is not enough, still INITIALIZING.
  service.SetConfigurationDone(true);
  EXPECT_EQ(UploadState::INITIALIZING,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  service.SetSyncActive(true);
  EXPECT_EQ(UploadState::INITIALIZING,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // Only after sync is both configured and active is upload actually ACTIVE.
  service.SetSyncCycleComplete(true);
  EXPECT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
}

TEST(SyncServiceUtilsTest, UploadToGoogleDisabledForModelType) {
  TestSyncService service;
  service.SetSyncAllowed(true);
  service.SetConfigurationDone(true);
  service.SetSyncActive(true);
  service.SetSyncCycleComplete(true);

  // Sync is enabled only for a specific model type.
  service.SetPreferredDataTypes(ModelTypeSet(syncer::BOOKMARKS));

  // Sanity check: Upload is ACTIVE for this model type.
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // ...but not for other types.
  EXPECT_EQ(
      UploadState::NOT_ACTIVE,
      GetUploadToGoogleState(&service, syncer::HISTORY_DELETE_DIRECTIVES));
  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::PREFERENCES));
}

TEST(SyncServiceUtilsTest, UploadToGoogleDisabledIfLocalSyncEnabled) {
  TestSyncService service;
  service.SetSyncAllowed(true);
  service.SetPreferredDataTypes(ProtocolTypes());
  service.SetSyncActive(true);
  service.SetConfigurationDone(true);
  service.SetSyncCycleComplete(true);

  // Sanity check: Upload is active now.
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // If we're in "local sync" mode, uploading should never be enabled, even if
  // configuration is done and all the data types are enabled.
  service.SetLocalSyncEnabled(true);

  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
}

TEST(SyncServiceUtilsTest, UploadToGoogleDisabledOnPersistentAuthError) {
  TestSyncService service;
  service.SetSyncAllowed(true);
  service.SetPreferredDataTypes(ProtocolTypes());
  service.SetSyncActive(true);
  service.SetConfigurationDone(true);
  service.SetSyncCycleComplete(true);

  // Sanity check: Upload is active now.
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // On a transient error, uploading remains active.
  GoogleServiceAuthError transient_error(
      GoogleServiceAuthError::CONNECTION_FAILED);
  ASSERT_TRUE(transient_error.IsTransientError());
  service.set_auth_error(transient_error);

  EXPECT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // On a persistent error, uploading is not considered active anymore (even
  // though Sync may still be considered active).
  GoogleServiceAuthError persistent_error(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
  ASSERT_TRUE(persistent_error.IsPersistentError());
  service.set_auth_error(persistent_error);

  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));

  // Once the auth error is resolved (e.g. user re-authenticated), uploading is
  // active again.
  service.set_auth_error(GoogleServiceAuthError(GoogleServiceAuthError::NONE));

  EXPECT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
}

TEST(SyncServiceUtilsTest, UploadToGoogleDisabledIfCustomPassphraseInUse) {
  TestSyncService service;
  service.SetSyncAllowed(true);
  service.SetPreferredDataTypes(ProtocolTypes());
  service.SetSyncActive(true);
  service.SetConfigurationDone(true);
  service.SetSyncCycleComplete(true);

  // Sanity check: Upload is ACTIVE, even for data types that are always
  // encrypted implicitly (PASSWORDS).
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::PASSWORDS));
  ASSERT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::DEVICE_INFO));

  // Once a custom passphrase is in use, upload should be considered disabled:
  // Even if we're technically still uploading, Google can't inspect the data.
  service.SetCustomPassphraseEnabled(true);

  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::BOOKMARKS));
  EXPECT_EQ(UploadState::NOT_ACTIVE,
            GetUploadToGoogleState(&service, syncer::PASSWORDS));
  // But unencryptable types like DEVICE_INFO are still active.
  EXPECT_EQ(UploadState::ACTIVE,
            GetUploadToGoogleState(&service, syncer::DEVICE_INFO));
}

}  // namespace syncer
