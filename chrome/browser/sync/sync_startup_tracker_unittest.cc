// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_startup_tracker.h"

#include <memory>

#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/profile_sync_test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

class MockObserver : public SyncStartupTracker::Observer {
 public:
  MOCK_METHOD0(SyncStartupCompleted, void(void));
  MOCK_METHOD0(SyncStartupFailed, void(void));
};

class SyncStartupTrackerTest : public testing::Test {
 public:
  SyncStartupTrackerTest() :
      no_error_(GoogleServiceAuthError::NONE) {
  }
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    mock_pss_ = static_cast<browser_sync::ProfileSyncServiceMock*>(
        ProfileSyncServiceFactory::GetInstance()->SetTestingFactoryAndUse(
            profile_.get(), BuildMockProfileSyncService));

    // Make gmock not spam the output with information about these uninteresting
    // calls.
    EXPECT_CALL(*mock_pss_, AddObserver(_)).Times(AnyNumber());
    EXPECT_CALL(*mock_pss_, RemoveObserver(_)).Times(AnyNumber());
    EXPECT_CALL(*mock_pss_, GetAuthError()).
        WillRepeatedly(ReturnRef(no_error_));
    ON_CALL(*mock_pss_, GetRegisteredDataTypes())
        .WillByDefault(Return(syncer::ModelTypeSet()));
    mock_pss_->Initialize();
  }

  void TearDown() override { profile_.reset(); }

  void SetupNonInitializedPSS() {
    EXPECT_CALL(*mock_pss_, GetAuthError())
        .WillRepeatedly(ReturnRef(no_error_));
    EXPECT_CALL(*mock_pss_, IsEngineInitialized())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_pss_, HasUnrecoverableError())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(true));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  GoogleServiceAuthError no_error_;
  std::unique_ptr<TestingProfile> profile_;
  browser_sync::ProfileSyncServiceMock* mock_pss_;
  MockObserver observer_;
};

TEST_F(SyncStartupTrackerTest, SyncAlreadyInitialized) {
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(true));
  EXPECT_CALL(observer_, SyncStartupCompleted());
  SyncStartupTracker tracker(profile_.get(), &observer_);
}

TEST_F(SyncStartupTrackerTest, SyncNotSignedIn) {
  // Make sure that we get a SyncStartupFailed() callback if sync is not logged
  // in.
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(false));
  EXPECT_CALL(observer_, SyncStartupFailed());
  SyncStartupTracker tracker(profile_.get(), &observer_);
}

TEST_F(SyncStartupTrackerTest, SyncAuthError) {
  // Make sure that we get a SyncStartupFailed() callback if sync gets an auth
  // error.
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(true));
  GoogleServiceAuthError error(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
  EXPECT_CALL(*mock_pss_, GetAuthError()).WillRepeatedly(ReturnRef(error));
  EXPECT_CALL(observer_, SyncStartupFailed());
  SyncStartupTracker tracker(profile_.get(), &observer_);
}

TEST_F(SyncStartupTrackerTest, SyncDelayedInitialization) {
  // Non-initialized PSS should result in no callbacks to the observer.
  SetupNonInitializedPSS();
  EXPECT_CALL(observer_, SyncStartupCompleted()).Times(0);
  EXPECT_CALL(observer_, SyncStartupFailed()).Times(0);
  SyncStartupTracker tracker(profile_.get(), &observer_);
  Mock::VerifyAndClearExpectations(&observer_);
  // Now, mark the PSS as initialized.
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(true));
  EXPECT_CALL(observer_, SyncStartupCompleted());
  tracker.OnStateChanged(mock_pss_);
}

TEST_F(SyncStartupTrackerTest, SyncDelayedAuthError) {
  // Non-initialized PSS should result in no callbacks to the observer.
  SetupNonInitializedPSS();
  EXPECT_CALL(observer_, SyncStartupCompleted()).Times(0);
  EXPECT_CALL(observer_, SyncStartupFailed()).Times(0);
  SyncStartupTracker tracker(profile_.get(), &observer_);
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(mock_pss_);

  // Now, mark the PSS as having an auth error.
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(true));
  GoogleServiceAuthError error(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
  EXPECT_CALL(*mock_pss_, GetAuthError()).WillRepeatedly(ReturnRef(error));
  EXPECT_CALL(observer_, SyncStartupFailed());
  tracker.OnStateChanged(mock_pss_);
}

TEST_F(SyncStartupTrackerTest, SyncDelayedUnrecoverableError) {
  // Non-initialized PSS should result in no callbacks to the observer.
  SetupNonInitializedPSS();
  EXPECT_CALL(observer_, SyncStartupCompleted()).Times(0);
  EXPECT_CALL(observer_, SyncStartupFailed()).Times(0);
  SyncStartupTracker tracker(profile_.get(), &observer_);
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(mock_pss_);

  // Now, mark the PSS as having an unrecoverable error.
  EXPECT_CALL(*mock_pss_, IsEngineInitialized()).WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_pss_, CanSyncStart()).WillRepeatedly(Return(true));
  GoogleServiceAuthError error(
      GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
  EXPECT_CALL(*mock_pss_, GetAuthError()).WillRepeatedly(ReturnRef(error));
  EXPECT_CALL(observer_, SyncStartupFailed());
  tracker.OnStateChanged(mock_pss_);
}

}  // namespace
