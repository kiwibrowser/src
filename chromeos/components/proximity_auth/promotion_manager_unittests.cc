// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/promotion_manager.h"

#include <string>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/test/simple_test_clock.h"
#include "base/test/test_simple_task_runner.h"
#include "chromeos/components/proximity_auth/fake_lock_handler.h"
#include "chromeos/components/proximity_auth/notification_controller.h"
#include "chromeos/components/proximity_auth/proximity_auth_profile_pref_manager.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "components/cryptauth/mock_local_device_data_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace proximity_auth {
namespace {
const int64_t kPreviousCheckTimestampMs = 123456789L;
const int64_t kFreshnessPeriodMs = 24 * 60 * 60 * 1000;
const char kPublicKey[] = "public key";

// ExternalDeviceInfo fields for the eligible unlock key.
const char kPublicKey1[] = "GOOG";
const char kDeviceName1[] = "Nexus 5";
const char kBluetoothAddress1[] = "aa:bb:cc:ee:dd:ff";
const bool kUnlockKey1 = true;
const bool kUnlockable1 = false;
}  // namespace

// Mock implementation of NotificationController.
class MockNotificationController : public NotificationController {
 public:
  MockNotificationController() {}
  ~MockNotificationController() override {}

  MOCK_METHOD0(ShowChromebookAddedNotification, void());
  MOCK_METHOD0(ShowPairingChangeNotification, void());
  MOCK_METHOD1(ShowPairingChangeAppliedNotification, void(const std::string&));
  MOCK_METHOD0(ShowPromotionNotification, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNotificationController);
};

// Mock implementation of ProximityAuthProfilePrefManager.
class MockProximityAuthPrefManager : public ProximityAuthProfilePrefManager {
 public:
  MockProximityAuthPrefManager() : ProximityAuthProfilePrefManager(nullptr) {}
  ~MockProximityAuthPrefManager() override {}

  MOCK_METHOD1(SetLastPromotionCheckTimestampMs, void(int64_t));
  MOCK_CONST_METHOD0(GetLastPromotionCheckTimestampMs, int64_t());

  MOCK_METHOD1(SetPromotionShownCount, void(int));
  MOCK_CONST_METHOD0(GetPromotionShownCount, int());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProximityAuthPrefManager);
};

class ProximityAuthPromotionManagerTest
    : public testing::Test,
      public cryptauth::MockCryptAuthClientFactory::Observer {
 protected:
  ProximityAuthPromotionManagerTest()
      : local_device_data_provider_(
            new cryptauth::MockLocalDeviceDataProvider()),
        notification_controller_(new StrictMock<MockNotificationController>()),
        expect_eligible_unlock_devices_request_(false),
        client_factory_(new cryptauth::MockCryptAuthClientFactory(
            cryptauth::MockCryptAuthClientFactory::MockType::
                MAKE_STRICT_MOCKS)),
        pref_manager_(new StrictMock<MockProximityAuthPrefManager>()),
        task_runner_(new base::TestSimpleTaskRunner),
        promotion_manager_(
            new PromotionManager(local_device_data_provider_.get(),
                                 notification_controller_.get(),
                                 pref_manager_.get(),
                                 base::WrapUnique(client_factory_),
                                 &clock_,
                                 task_runner_)) {
    client_factory_->AddObserver(this);
    local_device_data_provider_->SetPublicKey(
        std::make_unique<std::string>(kPublicKey));

    unlock_key_.set_public_key(kPublicKey1);
    unlock_key_.set_friendly_device_name(kDeviceName1);
    unlock_key_.set_bluetooth_address(kBluetoothAddress1);
    unlock_key_.set_unlock_key(kUnlockKey1);
    unlock_key_.set_unlockable(kUnlockable1);
  }

  ~ProximityAuthPromotionManagerTest() override {
    client_factory_->RemoveObserver(this);
  }

  void SetUp() override {
    EXPECT_CALL(*pref_manager_, GetPromotionShownCount()).WillOnce(Return(0));
    promotion_manager_->set_check_eligibility_probability(2.0);
    promotion_manager_->Start();
    LockScreen();
  }

  // MockCryptAuthClientFactory::Observer:
  void OnCryptAuthClientCreated(
      cryptauth::MockCryptAuthClient* client) override {
    if (expect_eligible_unlock_devices_request_) {
      EXPECT_CALL(*client, FindEligibleUnlockDevices(_, _, _))
          .WillOnce(
              DoAll(SaveArg<0>(&find_eligible_unlock_devices_request_),
                    SaveArg<1>(&find_eligible_unlock_devices_success_callback_),
                    SaveArg<2>(&find_eligible_unlock_devices_error_callback_)));
    } else {
      EXPECT_CALL(*client, FindEligibleForPromotion(_, _, _))
          .WillOnce(
              DoAll(SaveArg<0>(&find_eligible_for_promotion_request_),
                    SaveArg<1>(&find_eligible_for_promotion_success_callback_),
                    SaveArg<2>(&find_eligible_for_promotion_error_callback_)));
    }
  }

  void LockScreen() { ScreenlockBridge::Get()->SetLockHandler(&lock_handler_); }
  void UnlockScreen() { ScreenlockBridge::Get()->SetLockHandler(nullptr); }

  void ExpectFreshnessPeriodElapsed() {
    EXPECT_CALL(*pref_manager_, GetLastPromotionCheckTimestampMs())
        .WillOnce(Return(kPreviousCheckTimestampMs));
    const int64_t now = kPreviousCheckTimestampMs + kFreshnessPeriodMs + 1;
    clock_.SetNow(base::Time::FromJavaTime(now));
    EXPECT_CALL(*pref_manager_, SetLastPromotionCheckTimestampMs(now));
  }

  FakeLockHandler lock_handler_;
  std::unique_ptr<cryptauth::MockLocalDeviceDataProvider>
      local_device_data_provider_;
  std::unique_ptr<MockNotificationController> notification_controller_;
  base::SimpleTestClock clock_;
  bool expect_eligible_unlock_devices_request_;
  cryptauth::MockCryptAuthClientFactory* client_factory_;
  std::unique_ptr<MockProximityAuthPrefManager> pref_manager_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  cryptauth::ExternalDeviceInfo unlock_key_;

  // The instance being tested.
  std::unique_ptr<PromotionManager> promotion_manager_;

  cryptauth::FindEligibleForPromotionResponse
      find_eligible_for_promotion_response_;
  cryptauth::FindEligibleForPromotionRequest
      find_eligible_for_promotion_request_;
  cryptauth::CryptAuthClient::FindEligibleForPromotionCallback
      find_eligible_for_promotion_success_callback_;
  cryptauth::CryptAuthClient::ErrorCallback
      find_eligible_for_promotion_error_callback_;

  cryptauth::FindEligibleUnlockDevicesResponse
      find_eligible_unlock_devices_response_;
  cryptauth::FindEligibleUnlockDevicesRequest
      find_eligible_unlock_devices_request_;
  cryptauth::CryptAuthClient::FindEligibleUnlockDevicesCallback
      find_eligible_unlock_devices_success_callback_;
  cryptauth::CryptAuthClient::ErrorCallback
      find_eligible_unlock_devices_error_callback_;
};

TEST_F(ProximityAuthPromotionManagerTest, ShowPromotion) {
  ExpectFreshnessPeriodElapsed();
  UnlockScreen();

  // Receives the FindEligibleForPromotion response.
  find_eligible_for_promotion_response_.set_may_show_promo(true);
  find_eligible_for_promotion_success_callback_.Run(
      find_eligible_for_promotion_response_);

  // Waits before sending the FindEligibleUnlockDevices request.
  expect_eligible_unlock_devices_request_ = true;
  task_runner_->RunUntilIdle();

  // Receives the FindEligibleUnlockDevices response.
  EXPECT_CALL(*notification_controller_, ShowPromotionNotification());
  EXPECT_CALL(*pref_manager_, GetPromotionShownCount()).WillOnce(Return(0));
  EXPECT_CALL(*pref_manager_, SetPromotionShownCount(1));
  find_eligible_unlock_devices_response_.add_eligible_devices()->CopyFrom(
      unlock_key_);
  find_eligible_unlock_devices_success_callback_.Run(
      find_eligible_unlock_devices_response_);
}

TEST_F(ProximityAuthPromotionManagerTest,
       NoPromotion_FreshnessPeriodNotElapsed) {
  EXPECT_CALL(*pref_manager_, GetLastPromotionCheckTimestampMs())
      .WillOnce(Return(kPreviousCheckTimestampMs));
  const int64_t now = kPreviousCheckTimestampMs + 1;
  clock_.SetNow(base::Time::FromJavaTime(now));
  UnlockScreen();
}

TEST_F(ProximityAuthPromotionManagerTest, NoPromotion_NotEligibleForPromotion) {
  ExpectFreshnessPeriodElapsed();
  UnlockScreen();

  // Receives the FindEligibleForPromotion response.
  find_eligible_for_promotion_response_.set_may_show_promo(false);
  find_eligible_for_promotion_success_callback_.Run(
      find_eligible_for_promotion_response_);
}

TEST_F(ProximityAuthPromotionManagerTest,
       NoPromotion_EligibleForPromotionRequestFailed) {
  ExpectFreshnessPeriodElapsed();
  UnlockScreen();

  // Receives the FindEligibleForPromotion response.
  find_eligible_for_promotion_error_callback_.Run("some error");
}

TEST_F(ProximityAuthPromotionManagerTest, NoPromotion_NoEligibleUnlockDevices) {
  ExpectFreshnessPeriodElapsed();
  UnlockScreen();

  // Receives the FindEligibleForPromotion response.
  find_eligible_for_promotion_response_.set_may_show_promo(true);
  find_eligible_for_promotion_success_callback_.Run(
      find_eligible_for_promotion_response_);

  // Waits before sending the FindEligibleUnlockDevices request.
  expect_eligible_unlock_devices_request_ = true;
  task_runner_->RunUntilIdle();

  // Receives the FindEligibleUnlockDevices response.
  find_eligible_unlock_devices_success_callback_.Run(
      find_eligible_unlock_devices_response_);
}

TEST_F(ProximityAuthPromotionManagerTest,
       NoPromotion_EligibleUnlockDevicesRequestFailed) {
  ExpectFreshnessPeriodElapsed();
  UnlockScreen();

  // Receives the FindEligibleForPromotion response.
  find_eligible_for_promotion_response_.set_may_show_promo(true);
  find_eligible_for_promotion_success_callback_.Run(
      find_eligible_for_promotion_response_);

  // Waits before sending the FindEligibleUnlockDevices request.
  expect_eligible_unlock_devices_request_ = true;
  task_runner_->RunUntilIdle();

  // Receives the FindEligibleUnlockDevices response.
  find_eligible_unlock_devices_error_callback_.Run("some error");
}

TEST_F(ProximityAuthPromotionManagerTest,
       StopShowingPromotionAfterMaxCountIsReached) {
  // Show the promotion 3 times.
  for (int i = 0; i < 3; i++) {
    ExpectFreshnessPeriodElapsed();
    UnlockScreen();

    // Receives the FindEligibleForPromotion response.
    find_eligible_for_promotion_response_.set_may_show_promo(true);
    find_eligible_for_promotion_success_callback_.Run(
        find_eligible_for_promotion_response_);

    // Waits before sending the FindEligibleUnlockDevices request.
    expect_eligible_unlock_devices_request_ = true;
    task_runner_->RunUntilIdle();

    // Receives the FindEligibleUnlockDevices response.
    EXPECT_CALL(*notification_controller_, ShowPromotionNotification());
    EXPECT_CALL(*pref_manager_, GetPromotionShownCount()).WillOnce(Return(i));
    EXPECT_CALL(*pref_manager_, SetPromotionShownCount(i + 1));
    find_eligible_unlock_devices_response_.add_eligible_devices()->CopyFrom(
        unlock_key_);
    find_eligible_unlock_devices_success_callback_.Run(
        find_eligible_unlock_devices_response_);

    // Prepare for the iteration.
    LockScreen();
    expect_eligible_unlock_devices_request_ = false;
  }

  // Nothing should happen.
  UnlockScreen();
}

}  // namespace proximity_auth
