// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/profile_sync_service.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/browser_sync/profile_sync_test_util.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/sync/base/pref_names.h"
#include "components/sync/driver/data_type_manager_mock.h"
#include "components/sync/driver/fake_data_type_controller.h"
#include "components/sync/driver/sync_api_component_factory_mock.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/sync/engine/fake_sync_engine.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "google_apis/gaia/gaia_auth_consumer.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/url_request/url_request_test_util.h"
#include "services/identity/public/cpp/identity_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using syncer::DataTypeManager;
using syncer::DataTypeManagerMock;
using syncer::FakeSyncEngine;
using testing::_;
using testing::AnyNumber;
using testing::ByMove;
using testing::DoAll;
using testing::Mock;
using testing::Return;

namespace browser_sync {

namespace {

const char kGaiaId[] = "12345";
const char kEmail[] = "test_user@gmail.com";

class SyncServiceObserverMock : public syncer::SyncServiceObserver {
 public:
  SyncServiceObserverMock();
  ~SyncServiceObserverMock() override;

  MOCK_METHOD1(OnStateChanged, void(syncer::SyncService*));
};

SyncServiceObserverMock::SyncServiceObserverMock() {}

SyncServiceObserverMock::~SyncServiceObserverMock() {}

}  // namespace

ACTION_P(InvokeOnConfigureStart, sync_service) {
  sync_service->OnConfigureStart();
}

ACTION_P3(InvokeOnConfigureDone, sync_service, error_callback, result) {
  DataTypeManager::ConfigureResult configure_result =
      static_cast<DataTypeManager::ConfigureResult>(result);
  if (result.status == syncer::DataTypeManager::ABORTED)
    error_callback.Run(&configure_result);
  sync_service->OnConfigureDone(configure_result);
}

class ProfileSyncServiceStartupTest : public testing::Test {
 public:
  ProfileSyncServiceStartupTest() {
    profile_sync_service_bundle_.auth_service()
        ->set_auto_post_fetch_response_on_message_loop(true);
  }

  ~ProfileSyncServiceStartupTest() override {
    sync_service_->RemoveObserver(&observer_);
    sync_service_->Shutdown();
  }

  void CreateSyncService(ProfileSyncService::StartBehavior start_behavior) {
    component_factory_ = profile_sync_service_bundle_.component_factory();
    ProfileSyncServiceBundle::SyncClientBuilder builder(
        &profile_sync_service_bundle_);
    ProfileSyncService::InitParams init_params =
        profile_sync_service_bundle_.CreateBasicInitParams(start_behavior,
                                                           builder.Build());

    sync_service_ =
        std::make_unique<ProfileSyncService>(std::move(init_params));
    sync_service_->RegisterDataTypeController(
        std::make_unique<syncer::FakeDataTypeController>(syncer::BOOKMARKS));
    sync_service_->AddObserver(&observer_);
  }

  void SetError(DataTypeManager::ConfigureResult* result) {
    syncer::DataTypeStatusTable::TypeErrorMap errors;
    errors[syncer::BOOKMARKS] =
        syncer::SyncError(FROM_HERE, syncer::SyncError::UNRECOVERABLE_ERROR,
                          "Error", syncer::BOOKMARKS);
    result->data_type_status_table.UpdateFailedDataTypes(errors);
  }

 protected:
  virtual void SimulateTestUserSignin() {
    identity::MakePrimaryAccountAvailable(
        profile_sync_service_bundle_.signin_manager(),
        profile_sync_service_bundle_.auth_service(),
        profile_sync_service_bundle_.identity_manager(), kEmail);
  }

  DataTypeManagerMock* SetUpDataTypeManager() {
    auto data_type_manager = std::make_unique<DataTypeManagerMock>();
    DataTypeManagerMock* data_type_manager_raw = data_type_manager.get();
    EXPECT_CALL(*component_factory_, CreateDataTypeManager(_, _, _, _, _, _))
        .WillOnce(Return(ByMove(std::move(data_type_manager))));
    return data_type_manager_raw;
  }

  FakeSyncEngine* SetUpSyncEngine() {
    auto sync_engine = std::make_unique<FakeSyncEngine>();
    FakeSyncEngine* sync_engine_raw = sync_engine.get();
    EXPECT_CALL(*component_factory_, CreateSyncEngine(_, _, _, _))
        .WillOnce(Return(ByMove(std::move(sync_engine))));
    return sync_engine_raw;
  }

  PrefService* pref_service() {
    return profile_sync_service_bundle_.pref_service();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ProfileSyncServiceBundle profile_sync_service_bundle_;
  std::unique_ptr<ProfileSyncService> sync_service_;
  SyncServiceObserverMock observer_;
  syncer::DataTypeStatusTable data_type_status_table_;
  syncer::SyncApiComponentFactoryMock* component_factory_ = nullptr;
};

class ProfileSyncServiceStartupCrosTest : public ProfileSyncServiceStartupTest {
 public:
  ProfileSyncServiceStartupCrosTest() {
    CreateSyncService(ProfileSyncService::AUTO_START);
    // Set the primary account *without* providing an OAuth token.
    // TODO(https://crbug.com/814787): Change this flow to go through a
    // mainstream Identity Service API once that API exists. Note that this
    // might require supplying a valid refresh token here as opposed to an
    // empty string.
    profile_sync_service_bundle_.identity_manager()
        ->SetPrimaryAccountSynchronously(kGaiaId, kEmail,
                                         /*refresh_token=*/std::string());
    EXPECT_TRUE(
        profile_sync_service_bundle_.signin_manager()->IsAuthenticated());
  }

  void SimulateTestUserSignin() override {
    // We already populated the primary account above, all that's left to do
    // is provide a refresh token.
    profile_sync_service_bundle_.auth_service()->UpdateCredentials(
        profile_sync_service_bundle_.identity_manager()
            ->GetPrimaryAccountInfo()
            .account_id,
        "oauth2_login_token");
  }
};

TEST_F(ProfileSyncServiceStartupTest, StartFirstTime) {
  // We've never completed startup.
  pref_service()->ClearPref(syncer::prefs::kSyncFirstSetupComplete);
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _)).Times(0);

  // Should not actually start, rather just clean things up and wait
  // to be enabled.
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  sync_service_->Initialize();

  // Preferences should be back to defaults.
  EXPECT_EQ(0, pref_service()->GetInt64(syncer::prefs::kSyncLastSyncedTime));
  EXPECT_FALSE(
      pref_service()->GetBoolean(syncer::prefs::kSyncFirstSetupComplete));
  Mock::VerifyAndClearExpectations(data_type_manager);

  // Then start things up.
  EXPECT_CALL(*data_type_manager, Configure(_, _)).Times(1);
  EXPECT_CALL(*data_type_manager, state())
      .WillOnce(Return(DataTypeManager::CONFIGURED))
      .WillOnce(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());

  auto sync_blocker = sync_service_->GetSetupInProgressHandle();

  // Confirmation isn't needed before sign in occurs.
  EXPECT_FALSE(sync_service_->IsSyncConfirmationNeeded());

  // Simulate successful signin as test_user.
  SimulateTestUserSignin();

  // Simulate the UI telling sync it has finished setting up.
  sync_blocker.reset();
  sync_service_->SetFirstSetupComplete();
  EXPECT_TRUE(sync_service_->IsSyncActive());
}

// TODO(pavely): Reenable test once android is switched to oauth2.
TEST_F(ProfileSyncServiceStartupTest, DISABLED_StartNoCredentials) {
  // We've never completed startup.
  pref_service()->ClearPref(syncer::prefs::kSyncFirstSetupComplete);
  CreateSyncService(ProfileSyncService::MANUAL_START);

  // Should not actually start, rather just clean things up and wait
  // to be enabled.
  EXPECT_CALL(*component_factory_, CreateDataTypeManager(_, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  sync_service_->Initialize();

  // Preferences should be back to defaults.
  EXPECT_EQ(0, pref_service()->GetInt64(syncer::prefs::kSyncLastSyncedTime));
  EXPECT_FALSE(
      pref_service()->GetBoolean(syncer::prefs::kSyncFirstSetupComplete));

  // Then start things up.
  auto sync_blocker = sync_service_->GetSetupInProgressHandle();

  // Simulate successful signin as test_user.
  SimulateTestUserSignin();

  sync_blocker.reset();
  // ProfileSyncService should try to start by requesting access token.
  // This request should fail as login token was not issued.
  EXPECT_FALSE(sync_service_->IsSyncActive());
  EXPECT_EQ(GoogleServiceAuthError::USER_NOT_SIGNED_UP,
            sync_service_->GetAuthError().state());
}

// TODO(pavely): Reenable test once android is switched to oauth2.
TEST_F(ProfileSyncServiceStartupTest, DISABLED_StartInvalidCredentials) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  FakeSyncEngine* mock_sbh = SetUpSyncEngine();

  // Tell the backend to stall while downloading control types (simulating an
  // auth error).
  mock_sbh->set_fail_initial_download(true);

  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _)).Times(0);

  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  sync_service_->Initialize();
  EXPECT_FALSE(sync_service_->IsSyncActive());
  Mock::VerifyAndClearExpectations(data_type_manager);

  // Update the credentials, unstalling the backend.
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  auto sync_blocker = sync_service_->GetSetupInProgressHandle();

  // Simulate successful signin.
  SimulateTestUserSignin();

  sync_blocker.reset();

  // Verify we successfully finish startup and configuration.
  EXPECT_TRUE(sync_service_->IsSyncActive());
}

TEST_F(ProfileSyncServiceStartupCrosTest, StartCrosNoCredentials) {
  EXPECT_CALL(*component_factory_, CreateDataTypeManager(_, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(*component_factory_, CreateSyncEngine(_, _, _, _)).Times(0);
  pref_service()->ClearPref(syncer::prefs::kSyncFirstSetupComplete);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());

  sync_service_->Initialize();
  // Sync should not start because there are no tokens yet.
  EXPECT_FALSE(sync_service_->IsSyncActive());
  sync_service_->SetFirstSetupComplete();

  // Sync should not start because there are still no tokens.
  EXPECT_FALSE(sync_service_->IsSyncActive());
}

TEST_F(ProfileSyncServiceStartupCrosTest, StartFirstTime) {
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  pref_service()->ClearPref(syncer::prefs::kSyncFirstSetupComplete);
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop());
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());

  SimulateTestUserSignin();
  sync_service_->Initialize();
  EXPECT_TRUE(sync_service_->IsSyncActive());
}

TEST_F(ProfileSyncServiceStartupTest, StartNormal) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  sync_service_->SetFirstSetupComplete();
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  ON_CALL(*data_type_manager, IsNigoriEnabled()).WillByDefault(Return(true));

  sync_service_->Initialize();
}

// Test that we can recover from a case where a bug in the code resulted in
// OnUserChoseDatatypes not being properly called and datatype preferences
// therefore being left unset.
TEST_F(ProfileSyncServiceStartupTest, StartRecoverDatatypePrefs) {
  // Clear the datatype preference fields (simulating bug 154940).
  pref_service()->ClearPref(syncer::prefs::kSyncKeepEverythingSynced);
  syncer::ModelTypeSet user_types = syncer::UserTypes();
  for (syncer::ModelTypeSet::Iterator iter = user_types.First(); iter.Good();
       iter.Inc()) {
    pref_service()->ClearPref(
        syncer::SyncPrefs::GetPrefNameForDataType(iter.Get()));
  }

  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  sync_service_->SetFirstSetupComplete();
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  ON_CALL(*data_type_manager, IsNigoriEnabled()).WillByDefault(Return(true));

  sync_service_->Initialize();

  EXPECT_TRUE(
      pref_service()->GetBoolean(syncer::prefs::kSyncKeepEverythingSynced));
}

// Verify that the recovery of datatype preferences doesn't overwrite a valid
// case where only bookmarks are enabled.
TEST_F(ProfileSyncServiceStartupTest, StartDontRecoverDatatypePrefs) {
  // Explicitly set Keep Everything Synced to false and have only bookmarks
  // enabled.
  pref_service()->SetBoolean(syncer::prefs::kSyncKeepEverythingSynced, false);

  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  sync_service_->SetFirstSetupComplete();
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  ON_CALL(*data_type_manager, IsNigoriEnabled()).WillByDefault(Return(true));
  sync_service_->Initialize();

  EXPECT_FALSE(
      pref_service()->GetBoolean(syncer::prefs::kSyncKeepEverythingSynced));
}

TEST_F(ProfileSyncServiceStartupTest, ManagedStartup) {
  // Service should not be started by Initialize() since it's managed.
  pref_service()->SetString(prefs::kGoogleServicesAccountId, kEmail);
  CreateSyncService(ProfileSyncService::MANUAL_START);

  // Disable sync through policy.
  pref_service()->SetBoolean(syncer::prefs::kSyncManaged, true);
  EXPECT_CALL(*component_factory_, CreateDataTypeManager(_, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());

  sync_service_->Initialize();
}

TEST_F(ProfileSyncServiceStartupTest, SwitchManaged) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  sync_service_->SetFirstSetupComplete();
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  EXPECT_CALL(*data_type_manager, Configure(_, _));
  EXPECT_CALL(*data_type_manager, state())
      .WillRepeatedly(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  ON_CALL(*data_type_manager, IsNigoriEnabled()).WillByDefault(Return(true));
  sync_service_->Initialize();
  EXPECT_TRUE(sync_service_->IsEngineInitialized());
  EXPECT_TRUE(sync_service_->IsSyncActive());

  // The service should stop when switching to managed mode.
  Mock::VerifyAndClearExpectations(data_type_manager);
  EXPECT_CALL(*data_type_manager, state())
      .WillOnce(Return(DataTypeManager::CONFIGURED));
  EXPECT_CALL(*data_type_manager, Stop()).Times(1);
  pref_service()->SetBoolean(syncer::prefs::kSyncManaged, true);
  EXPECT_FALSE(sync_service_->IsEngineInitialized());
  // Note that PSS no longer references |data_type_manager| after stopping.

  // When switching back to unmanaged, the state should change but sync should
  // not start automatically because IsFirstSetupComplete() will be false.
  // A new DataTypeManager should not be created.
  Mock::VerifyAndClearExpectations(data_type_manager);
  EXPECT_CALL(*component_factory_, CreateDataTypeManager(_, _, _, _, _, _))
      .Times(0);
  pref_service()->ClearPref(syncer::prefs::kSyncManaged);
  EXPECT_FALSE(sync_service_->IsEngineInitialized());
  EXPECT_FALSE(sync_service_->IsSyncActive());
}

TEST_F(ProfileSyncServiceStartupTest, StartFailure) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  sync_service_->SetFirstSetupComplete();
  SetUpSyncEngine();
  DataTypeManagerMock* data_type_manager = SetUpDataTypeManager();
  DataTypeManager::ConfigureStatus status = DataTypeManager::ABORTED;
  DataTypeManager::ConfigureResult result(status, syncer::ModelTypeSet());
  EXPECT_CALL(*data_type_manager, Configure(_, _))
      .WillRepeatedly(
          DoAll(InvokeOnConfigureStart(sync_service_.get()),
                InvokeOnConfigureDone(
                    sync_service_.get(),
                    base::Bind(&ProfileSyncServiceStartupTest::SetError,
                               base::Unretained(this)),
                    result)));
  EXPECT_CALL(*data_type_manager, state())
      .WillOnce(Return(DataTypeManager::STOPPED));
  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  ON_CALL(*data_type_manager, IsNigoriEnabled()).WillByDefault(Return(true));
  sync_service_->Initialize();
  EXPECT_TRUE(sync_service_->HasUnrecoverableError());
}

TEST_F(ProfileSyncServiceStartupTest, StartDownloadFailed) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  SimulateTestUserSignin();
  FakeSyncEngine* mock_sbh = SetUpSyncEngine();
  mock_sbh->set_fail_initial_download(true);

  pref_service()->ClearPref(syncer::prefs::kSyncFirstSetupComplete);

  EXPECT_CALL(observer_, OnStateChanged(_)).Times(AnyNumber());
  sync_service_->Initialize();

  auto sync_blocker = sync_service_->GetSetupInProgressHandle();
  sync_blocker.reset();
  EXPECT_FALSE(sync_service_->IsSyncActive());
}

}  // namespace browser_sync
