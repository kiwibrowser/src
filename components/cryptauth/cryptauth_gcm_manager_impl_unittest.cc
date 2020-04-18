// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_gcm_manager_impl.h"

#include "base/macros.h"
#include "components/cryptauth/pref_names.h"
#include "components/gcm_driver/fake_gcm_driver.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::SaveArg;

namespace cryptauth {

namespace {

const char kCryptAuthGCMAppId[] = "com.google.chrome.cryptauth";
const char kCryptAuthGCMSenderId[] = "381449029288";
const char kExistingGCMRegistrationId[] = "cirrus";
const char kNewGCMRegistrationId[] = "stratus";
const char kCryptAuthMessageCollapseKey[] =
    "collapse_cryptauth_sync_DEVICES_SYNC";

// Mock GCMDriver implementation for testing.
class MockGCMDriver : public gcm::FakeGCMDriver {
 public:
  MockGCMDriver() {}
  ~MockGCMDriver() override {}

  MOCK_METHOD2(AddAppHandler,
               void(const std::string& app_id, gcm::GCMAppHandler* handler));

  MOCK_METHOD2(RegisterImpl,
               void(const std::string& app_id,
                    const std::vector<std::string>& sender_ids));

  using gcm::GCMDriver::RegisterFinished;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockGCMDriver);
};

}  // namespace

class CryptAuthGCMManagerImplTest
    : public testing::Test,
      public CryptAuthGCMManager::Observer {
 protected:
  CryptAuthGCMManagerImplTest()
      : gcm_manager_(&gcm_driver_, &pref_service_) {}

  // testing::Test:
  void SetUp() override {
    CryptAuthGCMManager::RegisterPrefs(pref_service_.registry());
    gcm_manager_.AddObserver(this);
    EXPECT_CALL(gcm_driver_, AddAppHandler(kCryptAuthGCMAppId, &gcm_manager_));
    gcm_manager_.StartListening();
  }

  void TearDown() override { gcm_manager_.RemoveObserver(this); }

  void RegisterWithGCM(gcm::GCMClient::Result registration_result) {
    std::vector<std::string> sender_ids;
    EXPECT_CALL(gcm_driver_, RegisterImpl(kCryptAuthGCMAppId, _))
        .WillOnce(SaveArg<1>(&sender_ids));
    gcm_manager_.RegisterWithGCM();

    ASSERT_EQ(1u, sender_ids.size());
    EXPECT_EQ(kCryptAuthGCMSenderId, sender_ids[0]);

    bool success = (registration_result == gcm::GCMClient::SUCCESS);
    EXPECT_CALL(*this, OnGCMRegistrationResultProxy(success));
    gcm_driver_.RegisterFinished(kCryptAuthGCMAppId, kNewGCMRegistrationId,
                                 registration_result);
  }

  // CryptAuthGCMManager::Observer:
  void OnGCMRegistrationResult(bool success) override {
    OnGCMRegistrationResultProxy(success);
  }

  void OnReenrollMessage() override { OnReenrollMessageProxy(); }

  void OnResyncMessage() override { OnResyncMessageProxy(); }

  MOCK_METHOD1(OnGCMRegistrationResultProxy, void(bool));
  MOCK_METHOD0(OnReenrollMessageProxy, void());
  MOCK_METHOD0(OnResyncMessageProxy, void());

  testing::StrictMock<MockGCMDriver> gcm_driver_;

  TestingPrefServiceSimple pref_service_;

  CryptAuthGCMManagerImpl gcm_manager_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthGCMManagerImplTest);
};

TEST_F(CryptAuthGCMManagerImplTest, RegisterPrefs) {
  TestingPrefServiceSimple pref_service;
  CryptAuthGCMManager::RegisterPrefs(pref_service.registry());
  EXPECT_TRUE(pref_service.FindPreference(prefs::kCryptAuthGCMRegistrationId));
}

TEST_F(CryptAuthGCMManagerImplTest, RegistrationSucceeds) {
  EXPECT_EQ(std::string(), gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::SUCCESS);
  EXPECT_EQ(kNewGCMRegistrationId, gcm_manager_.GetRegistrationId());
}

TEST_F(CryptAuthGCMManagerImplTest,
       RegistrationSucceedsWithExistingRegistration) {
  pref_service_.SetString(prefs::kCryptAuthGCMRegistrationId,
                          kExistingGCMRegistrationId);
  EXPECT_EQ(kExistingGCMRegistrationId, gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::SUCCESS);
  EXPECT_EQ(kNewGCMRegistrationId, gcm_manager_.GetRegistrationId());
  EXPECT_EQ(kNewGCMRegistrationId,
            pref_service_.GetString(prefs::kCryptAuthGCMRegistrationId));
}

TEST_F(CryptAuthGCMManagerImplTest, RegisterWithGCMFails) {
  EXPECT_EQ(std::string(), gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::SERVER_ERROR);
  EXPECT_EQ(std::string(), gcm_manager_.GetRegistrationId());
  EXPECT_EQ(std::string(),
            pref_service_.GetString(prefs::kCryptAuthGCMRegistrationId));
}

TEST_F(CryptAuthGCMManagerImplTest,
       RegisterWithGCMFailsWithExistingRegistration) {
  pref_service_.SetString(prefs::kCryptAuthGCMRegistrationId,
                          kExistingGCMRegistrationId);
  EXPECT_EQ(kExistingGCMRegistrationId, gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::SERVER_ERROR);
  EXPECT_EQ(kExistingGCMRegistrationId, gcm_manager_.GetRegistrationId());
  EXPECT_EQ(kExistingGCMRegistrationId,
            pref_service_.GetString(prefs::kCryptAuthGCMRegistrationId));
}

TEST_F(CryptAuthGCMManagerImplTest,
       RegistrationFailsThenSucceeds) {
  EXPECT_EQ(std::string(), gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::NETWORK_ERROR);
  EXPECT_EQ(std::string(), gcm_manager_.GetRegistrationId());
  RegisterWithGCM(gcm::GCMClient::SUCCESS);
  EXPECT_EQ(kNewGCMRegistrationId, gcm_manager_.GetRegistrationId());
}

TEST_F(CryptAuthGCMManagerImplTest, ConcurrentRegistrations) {
  // If multiple RegisterWithGCM() calls are made concurrently, only one
  // registration attempt should actually be made.
  EXPECT_CALL(gcm_driver_, RegisterImpl(kCryptAuthGCMAppId, _));
  gcm_manager_.RegisterWithGCM();
  gcm_manager_.RegisterWithGCM();
  gcm_manager_.RegisterWithGCM();

  EXPECT_CALL(*this, OnGCMRegistrationResultProxy(true));
  gcm_driver_.RegisterFinished(kCryptAuthGCMAppId, kNewGCMRegistrationId,
                               gcm::GCMClient::SUCCESS);
  EXPECT_EQ(kNewGCMRegistrationId, gcm_manager_.GetRegistrationId());
}

TEST_F(CryptAuthGCMManagerImplTest, ReenrollmentMessagesReceived) {
  EXPECT_CALL(*this, OnReenrollMessageProxy()).Times(2);

  gcm::IncomingMessage message;
  message.data["registrationTickleType"] = "1";  // FORCE_ENROLLMENT
  message.collapse_key = kCryptAuthMessageCollapseKey;
  message.sender_id = kCryptAuthGCMSenderId;

  gcm::GCMAppHandler* gcm_app_handler =
      static_cast<gcm::GCMAppHandler*>(&gcm_manager_);
  gcm_app_handler->OnMessage(kCryptAuthGCMAppId, message);
  message.data["registrationTickleType"] = "2";  // UPDATE_ENROLLMENT
  gcm_app_handler->OnMessage(kCryptAuthGCMAppId, message);
}

TEST_F(CryptAuthGCMManagerImplTest, ResyncMessagesReceived) {
  EXPECT_CALL(*this, OnResyncMessageProxy()).Times(2);

  gcm::IncomingMessage message;
  message.data["registrationTickleType"] = "3";  // DEVICES_SYNC
  message.collapse_key = kCryptAuthMessageCollapseKey;
  message.sender_id = kCryptAuthGCMSenderId;

  gcm::GCMAppHandler* gcm_app_handler =
      static_cast<gcm::GCMAppHandler*>(&gcm_manager_);
  gcm_app_handler->OnMessage(kCryptAuthGCMAppId, message);
  gcm_app_handler->OnMessage(kCryptAuthGCMAppId, message);
}

}  // namespace cryptauth
