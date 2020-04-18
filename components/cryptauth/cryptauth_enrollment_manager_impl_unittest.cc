// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_enrollment_manager_impl.h"

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/test/simple_test_clock.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/mock_sync_scheduler.h"
#include "components/cryptauth/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace cryptauth {

namespace {

// The GCM registration id from a successful registration.
const char kGCMRegistrationId[] = "new gcm registration id";

// The user's persistent public key identifying the local device.
const char kUserPublicKey[] = "user public key";

// The initial "Now" time for testing.
const double kInitialTimeNowSeconds = 20000000;

// A later "Now" time for testing.
const double kLaterTimeNow = kInitialTimeNowSeconds + 30;

// The timestamp of a last successful enrollment that is still valid.
const double kLastEnrollmentTimeSeconds =
    kInitialTimeNowSeconds - (60 * 60 * 24 * 15);

// The timestamp of a last successful enrollment that is expired.
const double kLastExpiredEnrollmentTimeSeconds =
    kInitialTimeNowSeconds - (60 * 60 * 24 * 100);

// Mocks out the actual enrollment flow.
class MockCryptAuthEnroller : public CryptAuthEnroller {
 public:
  MockCryptAuthEnroller() {}
  ~MockCryptAuthEnroller() override {}

  MOCK_METHOD5(Enroll,
               void(const std::string& user_public_key,
                    const std::string& user_private_key,
                    const GcmDeviceInfo& device_info,
                    InvocationReason invocation_reason,
                    const EnrollmentFinishedCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCryptAuthEnroller);
};

// Creates MockCryptAuthEnroller instances, and allows expecations to be set
// before they are returned.
class MockCryptAuthEnrollerFactory : public CryptAuthEnrollerFactory {
 public:
  MockCryptAuthEnrollerFactory()
      : next_cryptauth_enroller_(new NiceMock<MockCryptAuthEnroller>()) {}
  ~MockCryptAuthEnrollerFactory() override {}

  // CryptAuthEnrollerFactory:
  std::unique_ptr<CryptAuthEnroller> CreateInstance() override {
    auto passed_cryptauth_enroller = std::move(next_cryptauth_enroller_);
    next_cryptauth_enroller_.reset(new NiceMock<MockCryptAuthEnroller>());
    return std::move(passed_cryptauth_enroller);
  }

  MockCryptAuthEnroller* next_cryptauth_enroller() {
    return next_cryptauth_enroller_.get();
  }

 private:
  // Stores the next CryptAuthEnroller to be created.
  // Ownership is passed to the caller of |CreateInstance()|.
  std::unique_ptr<MockCryptAuthEnroller> next_cryptauth_enroller_;

  DISALLOW_COPY_AND_ASSIGN(MockCryptAuthEnrollerFactory);
};

// Harness for testing CryptAuthEnrollmentManager.
class TestCryptAuthEnrollmentManager : public CryptAuthEnrollmentManagerImpl {
 public:
  TestCryptAuthEnrollmentManager(
      base::Clock* clock,
      std::unique_ptr<CryptAuthEnrollerFactory> enroller_factory,
      std::unique_ptr<SecureMessageDelegate> secure_message_delegate,
      const GcmDeviceInfo& device_info,
      CryptAuthGCMManager* gcm_manager,
      PrefService* pref_service)
      : CryptAuthEnrollmentManagerImpl(clock,
                                       std::move(enroller_factory),
                                       std::move(secure_message_delegate),
                                       device_info,
                                       gcm_manager,
                                       pref_service),
        scoped_sync_scheduler_(new NiceMock<MockSyncScheduler>()),
        weak_sync_scheduler_factory_(scoped_sync_scheduler_) {
    SetSyncSchedulerForTest(base::WrapUnique(scoped_sync_scheduler_));
  }

  ~TestCryptAuthEnrollmentManager() override {}

  base::WeakPtr<MockSyncScheduler> GetSyncScheduler() {
    return weak_sync_scheduler_factory_.GetWeakPtr();
  }

 private:
  // Ownership is passed to |CryptAuthEnrollmentManager| super class when
  // |CreateSyncScheduler()| is called.
  NiceMock<MockSyncScheduler>* scoped_sync_scheduler_;

  // Stores the pointer of |scoped_sync_scheduler_| after ownership is passed to
  // the super class.
  // This should be safe because the life-time this SyncScheduler will always be
  // within the life of the TestCryptAuthEnrollmentManager object.
  base::WeakPtrFactory<MockSyncScheduler> weak_sync_scheduler_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestCryptAuthEnrollmentManager);
};

}  // namespace

class CryptAuthEnrollmentManagerImplTest
    : public testing::Test,
      public CryptAuthEnrollmentManager::Observer {
 protected:
  CryptAuthEnrollmentManagerImplTest()
      : public_key_(kUserPublicKey),
        enroller_factory_(new MockCryptAuthEnrollerFactory()),
        secure_message_delegate_(new FakeSecureMessageDelegate()),
        gcm_manager_(kGCMRegistrationId),
        enrollment_manager_(&clock_,
                            base::WrapUnique(enroller_factory_),
                            base::WrapUnique(secure_message_delegate_),
                            device_info_,
                            &gcm_manager_,
                            &pref_service_) {}

  // testing::Test:
  void SetUp() override {
    clock_.SetNow(base::Time::FromDoubleT(kInitialTimeNowSeconds));
    enrollment_manager_.AddObserver(this);

    private_key_ =
        secure_message_delegate_->GetPrivateKeyForPublicKey(public_key_);
    secure_message_delegate_->set_next_public_key(public_key_);

    CryptAuthEnrollmentManager::RegisterPrefs(pref_service_.registry());
    pref_service_.SetUserPref(
        prefs::kCryptAuthEnrollmentIsRecoveringFromFailure,
        std::make_unique<base::Value>(false));
    pref_service_.SetUserPref(
        prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds,
        std::make_unique<base::Value>(kLastEnrollmentTimeSeconds));
    pref_service_.SetUserPref(
        prefs::kCryptAuthEnrollmentReason,
        std::make_unique<base::Value>(INVOCATION_REASON_UNKNOWN));

    std::string public_key_b64, private_key_b64;
    base::Base64UrlEncode(public_key_,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &public_key_b64);
    base::Base64UrlEncode(private_key_,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &private_key_b64);
    pref_service_.SetString(prefs::kCryptAuthEnrollmentUserPublicKey,
                            public_key_b64);
    pref_service_.SetString(prefs::kCryptAuthEnrollmentUserPrivateKey,
                            private_key_b64);

    ON_CALL(*sync_scheduler(), GetStrategy())
        .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  }

  void TearDown() override { enrollment_manager_.RemoveObserver(this); }

  // CryptAuthEnrollmentManager::Observer:
  void OnEnrollmentStarted() override { OnEnrollmentStartedProxy(); }

  void OnEnrollmentFinished(bool success) override {
    // Simulate the scheduler changing strategies based on success or failure.
    SyncScheduler::Strategy new_strategy =
        SyncScheduler::Strategy::AGGRESSIVE_RECOVERY;
    ON_CALL(*sync_scheduler(), GetStrategy())
        .WillByDefault(Return(new_strategy));

    OnEnrollmentFinishedProxy(success);
  }

  MOCK_METHOD0(OnEnrollmentStartedProxy, void());
  MOCK_METHOD1(OnEnrollmentFinishedProxy, void(bool success));

  // Simulates firing the SyncScheduler to trigger an enrollment attempt.
  CryptAuthEnroller::EnrollmentFinishedCallback FireSchedulerForEnrollment(
      InvocationReason expected_invocation_reason) {
    CryptAuthEnroller::EnrollmentFinishedCallback completion_callback;
    EXPECT_CALL(
        *next_cryptauth_enroller(),
        Enroll(public_key_, private_key_, _, expected_invocation_reason, _))
        .WillOnce(SaveArg<4>(&completion_callback));

    auto sync_request = std::make_unique<SyncScheduler::SyncRequest>(
        enrollment_manager_.GetSyncScheduler());
    EXPECT_CALL(*this, OnEnrollmentStartedProxy());

    SyncScheduler::Delegate* delegate =
        static_cast<SyncScheduler::Delegate*>(&enrollment_manager_);
    delegate->OnSyncRequested(std::move(sync_request));

    return completion_callback;
  }

  MockSyncScheduler* sync_scheduler() {
    return enrollment_manager_.GetSyncScheduler().get();
  }

  MockCryptAuthEnroller* next_cryptauth_enroller() {
    return enroller_factory_->next_cryptauth_enroller();
  }

  // The expected persistent keypair.
  std::string public_key_;
  std::string private_key_;

  base::SimpleTestClock clock_;

  // Owned by |enrollment_manager_|.
  MockCryptAuthEnrollerFactory* enroller_factory_;

  // Ownered by |enrollment_manager_|.
  FakeSecureMessageDelegate* secure_message_delegate_;

  GcmDeviceInfo device_info_;

  TestingPrefServiceSimple pref_service_;

  FakeCryptAuthGCMManager gcm_manager_;

  TestCryptAuthEnrollmentManager enrollment_manager_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthEnrollmentManagerImplTest);
};

TEST_F(CryptAuthEnrollmentManagerImplTest, RegisterPrefs) {
  TestingPrefServiceSimple pref_service;
  CryptAuthEnrollmentManager::RegisterPrefs(pref_service.registry());
  EXPECT_TRUE(pref_service.FindPreference(
      prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds));
  EXPECT_TRUE(pref_service.FindPreference(
      prefs::kCryptAuthEnrollmentIsRecoveringFromFailure));
  EXPECT_TRUE(pref_service.FindPreference(prefs::kCryptAuthEnrollmentReason));
}

TEST_F(CryptAuthEnrollmentManagerImplTest, GetEnrollmentState) {
  enrollment_manager_.Start();

  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  EXPECT_FALSE(enrollment_manager_.IsRecoveringFromFailure());

  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  EXPECT_TRUE(enrollment_manager_.IsRecoveringFromFailure());

  base::TimeDelta time_to_next_sync = base::TimeDelta::FromMinutes(60);
  ON_CALL(*sync_scheduler(), GetTimeToNextSync())
      .WillByDefault(Return(time_to_next_sync));
  EXPECT_EQ(time_to_next_sync, enrollment_manager_.GetTimeToNextAttempt());

  ON_CALL(*sync_scheduler(), GetSyncState())
      .WillByDefault(Return(SyncScheduler::SyncState::SYNC_IN_PROGRESS));
  EXPECT_TRUE(enrollment_manager_.IsEnrollmentInProgress());

  ON_CALL(*sync_scheduler(), GetSyncState())
      .WillByDefault(Return(SyncScheduler::SyncState::WAITING_FOR_REFRESH));
  EXPECT_FALSE(enrollment_manager_.IsEnrollmentInProgress());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, InitWithDefaultPrefs) {
  base::SimpleTestClock clock;
  clock.SetNow(base::Time::FromDoubleT(kInitialTimeNowSeconds));
  base::TimeDelta elapsed_time = clock.Now() - base::Time::FromDoubleT(0);

  TestingPrefServiceSimple pref_service;
  CryptAuthEnrollmentManager::RegisterPrefs(pref_service.registry());

  TestCryptAuthEnrollmentManager enrollment_manager(
      &clock, std::make_unique<MockCryptAuthEnrollerFactory>(),
      std::make_unique<FakeSecureMessageDelegate>(), device_info_,
      &gcm_manager_, &pref_service);

  EXPECT_CALL(
      *enrollment_manager.GetSyncScheduler(),
      Start(elapsed_time, SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  enrollment_manager.Start();

  EXPECT_FALSE(enrollment_manager.IsEnrollmentValid());
  EXPECT_TRUE(enrollment_manager.GetLastEnrollmentTime().is_null());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, InitWithExistingPrefs) {
  EXPECT_CALL(
      *sync_scheduler(),
      Start(clock_.Now() - base::Time::FromDoubleT(kLastEnrollmentTimeSeconds),
            SyncScheduler::Strategy::PERIODIC_REFRESH));

  enrollment_manager_.Start();
  EXPECT_TRUE(enrollment_manager_.IsEnrollmentValid());
  EXPECT_EQ(base::Time::FromDoubleT(kLastEnrollmentTimeSeconds),
            enrollment_manager_.GetLastEnrollmentTime());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, InitWithExpiredEnrollment) {
  pref_service_.SetUserPref(
      prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds,
      std::make_unique<base::Value>(kLastExpiredEnrollmentTimeSeconds));

  EXPECT_CALL(*sync_scheduler(),
              Start(clock_.Now() - base::Time::FromDoubleT(
                                       kLastExpiredEnrollmentTimeSeconds),
                    SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));

  enrollment_manager_.Start();
  EXPECT_FALSE(enrollment_manager_.IsEnrollmentValid());
  EXPECT_EQ(base::Time::FromDoubleT(kLastExpiredEnrollmentTimeSeconds),
            enrollment_manager_.GetLastEnrollmentTime());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, ForceEnrollment) {
  enrollment_manager_.Start();

  EXPECT_CALL(*sync_scheduler(), ForceSync());
  enrollment_manager_.ForceEnrollmentNow(INVOCATION_REASON_SERVER_INITIATED);

  auto completion_callback =
      FireSchedulerForEnrollment(INVOCATION_REASON_SERVER_INITIATED);

  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNow));
  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(true));
  completion_callback.Run(true);
  EXPECT_EQ(clock_.Now(), enrollment_manager_.GetLastEnrollmentTime());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, EnrollmentFailsThenSucceeds) {
  enrollment_manager_.Start();
  base::Time old_enrollment_time = enrollment_manager_.GetLastEnrollmentTime();

  // The first periodic enrollment fails.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  auto completion_callback =
      FireSchedulerForEnrollment(INVOCATION_REASON_PERIODIC);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNow));
  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(false));
  completion_callback.Run(false);
  EXPECT_EQ(old_enrollment_time, enrollment_manager_.GetLastEnrollmentTime());
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kCryptAuthEnrollmentIsRecoveringFromFailure));

  // The second recovery enrollment succeeds.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  completion_callback =
      FireSchedulerForEnrollment(INVOCATION_REASON_FAILURE_RECOVERY);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNow + 30));
  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(true));
  completion_callback.Run(true);
  EXPECT_EQ(clock_.Now(), enrollment_manager_.GetLastEnrollmentTime());
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kCryptAuthEnrollmentIsRecoveringFromFailure));
}

TEST_F(CryptAuthEnrollmentManagerImplTest, EnrollmentSucceedsForFirstTime) {
  // Initialize |enrollment_manager_|.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  gcm_manager_.set_registration_id(std::string());
  pref_service_.ClearPref(prefs::kCryptAuthEnrollmentUserPublicKey);
  pref_service_.ClearPref(prefs::kCryptAuthEnrollmentUserPrivateKey);
  pref_service_.ClearPref(prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds);
  enrollment_manager_.Start();
  EXPECT_FALSE(enrollment_manager_.IsEnrollmentValid());

  // Trigger a sync request.
  EXPECT_CALL(*this, OnEnrollmentStartedProxy());
  auto sync_request = std::make_unique<SyncScheduler::SyncRequest>(
      enrollment_manager_.GetSyncScheduler());
  static_cast<SyncScheduler::Delegate*>(&enrollment_manager_)
      ->OnSyncRequested(std::move(sync_request));

  // Complete GCM registration successfully, and expect an enrollment.
  CryptAuthEnroller::EnrollmentFinishedCallback enrollment_callback;
  EXPECT_CALL(
      *next_cryptauth_enroller(),
      Enroll(public_key_, private_key_, _, INVOCATION_REASON_INITIALIZATION, _))
      .WillOnce(SaveArg<4>(&enrollment_callback));
  ASSERT_TRUE(gcm_manager_.registration_in_progress());
  gcm_manager_.CompleteRegistration(kGCMRegistrationId);

  // Complete CryptAuth enrollment.
  ASSERT_FALSE(enrollment_callback.is_null());
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNow));
  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(true));
  enrollment_callback.Run(true);
  EXPECT_EQ(clock_.Now(), enrollment_manager_.GetLastEnrollmentTime());
  EXPECT_TRUE(enrollment_manager_.IsEnrollmentValid());

  // Check that CryptAuthEnrollmentManager returns the expected key-pair.
  EXPECT_EQ(public_key_, enrollment_manager_.GetUserPublicKey());
  EXPECT_EQ(private_key_, enrollment_manager_.GetUserPrivateKey());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, GCMRegistrationFails) {
  // Initialize |enrollment_manager_|.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  gcm_manager_.set_registration_id(std::string());
  enrollment_manager_.Start();

  // Trigger a sync request.
  EXPECT_CALL(*this, OnEnrollmentStartedProxy());
  auto sync_request = std::make_unique<SyncScheduler::SyncRequest>(
      enrollment_manager_.GetSyncScheduler());
  static_cast<SyncScheduler::Delegate*>(&enrollment_manager_)
      ->OnSyncRequested(std::move(sync_request));

  // Complete GCM registration with failure.
  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(false));
  gcm_manager_.CompleteRegistration(std::string());
}

TEST_F(CryptAuthEnrollmentManagerImplTest, ReenrollOnGCMPushMessage) {
  enrollment_manager_.Start();

  // Simulate receiving a GCM push message, forcing the device to re-enroll.
  gcm_manager_.PushReenrollMessage();
  auto completion_callback =
      FireSchedulerForEnrollment(INVOCATION_REASON_SERVER_INITIATED);

  EXPECT_CALL(*this, OnEnrollmentFinishedProxy(true));
  completion_callback.Run(true);
}

}  // namespace cryptauth
