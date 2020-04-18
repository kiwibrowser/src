// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/auto_enrollment_client_impl.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/server_backed_device_state.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/policy/core/common/cloud/mock_device_management_service.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "crypto/sha2.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace policy {

namespace {

const char kStateKey[] = "state_key";
const char kStateKeyHash[] =
    "\xde\x74\xcd\xf0\x03\x36\x8c\x21\x79\xba\xb1\x5a\xc4\x32\xee\xd6"
    "\xb3\x4a\x5e\xff\x73\x7e\x92\xd9\xf8\x6e\x72\x44\xd0\x97\xc3\xe6";
const char kDisabledMessage[] = "This device has been disabled.";

const char kSerialNumber[] = "SN123456";
const char kBrandCode[] = "AABC";
const char kInitialEnrollmentIdHash[] = "\x30\x18\xb7\x0f\x76\x09\xc5\xc7";

const int kInitialEnrollmentIdHashLength = 8;

// This is modulus power value used in initial enrollment to detect that the
// server is outdated and does not support initial enrollment. See the
// |DetectOutdatedServer| test case.
const int kInitialEnrollmentModulusPowerOutdatedServer = 14;

using ::testing::InSequence;
using ::testing::Mock;
using ::testing::SaveArg;
using ::testing::_;

enum class AutoEnrollmentProtocol { kFRE, kInitialEnrollment };

class AutoEnrollmentClientImplTest
    : public testing::Test,
      public ::testing::WithParamInterface<AutoEnrollmentProtocol> {
 protected:
  AutoEnrollmentClientImplTest()
      : scoped_testing_local_state_(TestingBrowserProcess::GetGlobal()),
        local_state_(scoped_testing_local_state_.Get()),
        state_(AUTO_ENROLLMENT_STATE_PENDING) {}

  void SetUp() override {
    CreateClient(4, 8);
    ASSERT_FALSE(local_state_->GetUserPref(prefs::kShouldAutoEnroll));
    ASSERT_FALSE(local_state_->GetUserPref(prefs::kAutoEnrollmentPowerLimit));
  }

  void TearDown() override {
    // Flush any deletion tasks.
    base::RunLoop().RunUntilIdle();
  }

  void CreateClient(int power_initial, int power_limit) {
    state_ = AUTO_ENROLLMENT_STATE_PENDING;
    service_.reset(new MockDeviceManagementService());
    EXPECT_CALL(*service_, StartJob(_, _, _, _, _, _))
        .WillRepeatedly(SaveArg<5>(&last_request_));
    auto progress_callback =
        base::BindRepeating(&AutoEnrollmentClientImplTest::ProgressCallback,
                            base::Unretained(this));
    auto* url_request_context_getter = new net::TestURLRequestContextGetter(
        base::ThreadTaskRunnerHandle::Get());
    if (GetParam() == AutoEnrollmentProtocol::kFRE) {
      client_ = AutoEnrollmentClientImpl::FactoryImpl().CreateForFRE(
          progress_callback, service_.get(), local_state_,
          url_request_context_getter, kStateKey, power_initial, power_limit);
    } else {
      client_ =
          AutoEnrollmentClientImpl::FactoryImpl().CreateForInitialEnrollment(
              progress_callback, service_.get(), local_state_,
              url_request_context_getter, kSerialNumber, kBrandCode,
              power_initial, power_limit,
              kInitialEnrollmentModulusPowerOutdatedServer);
    }
  }

  void ProgressCallback(AutoEnrollmentState state) { state_ = state; }

  void ServerWillFail(DeviceManagementStatus error) {
    em::DeviceManagementResponse dummy_response;
    EXPECT_CALL(*service_,
                CreateJob(DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT, _))
        .WillOnce(service_->FailJob(error));
  }

  void ServerWillReply(int64_t modulus, bool with_hashes, bool with_id_hash) {
    em::DeviceManagementResponse response;
    em::DeviceAutoEnrollmentResponse* enrollment_response =
        response.mutable_auto_enrollment_response();
    if (modulus >= 0)
      enrollment_response->set_expected_modulus(modulus);
    if (with_hashes) {
      for (int i = 0; i < 10; ++i) {
        std::string state_key = base::StringPrintf("state_key %d", i);
        std::string hash_full = crypto::SHA256HashString(state_key);
        std::string hash =
            GetParam() == AutoEnrollmentProtocol::kFRE
                ? hash_full
                : hash_full.substr(0, kInitialEnrollmentIdHashLength);
        enrollment_response->mutable_hash()->Add()->assign(hash);
      }
    }
    if (with_id_hash) {
      if (GetParam() == AutoEnrollmentProtocol::kFRE) {
        enrollment_response->mutable_hash()->Add()->assign(
            kStateKeyHash, crypto::kSHA256Length);
      } else {
        enrollment_response->mutable_hash()->Add()->assign(
            kInitialEnrollmentIdHash, kInitialEnrollmentIdHashLength);
      }
    }
    EXPECT_CALL(*service_,
                CreateJob(DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT, _))
        .WillOnce(service_->SucceedJob(response));
  }

  em::DeviceInitialEnrollmentStateResponse::InitialEnrollmentMode
  MapRestoreModeToInitialEnrollmentMode(
      em::DeviceStateRetrievalResponse::RestoreMode restore_mode) {
    using DeviceStateRetrieval = em::DeviceStateRetrievalResponse;
    using DeviceInitialEnrollmentState =
        em::DeviceInitialEnrollmentStateResponse;

    switch (restore_mode) {
      case DeviceStateRetrieval::RESTORE_MODE_NONE:
        return DeviceInitialEnrollmentState::INITIAL_ENROLLMENT_MODE_NONE;
      case DeviceStateRetrieval::RESTORE_MODE_REENROLLMENT_REQUESTED:
        return DeviceInitialEnrollmentState::INITIAL_ENROLLMENT_MODE_NONE;
      case DeviceStateRetrieval::RESTORE_MODE_REENROLLMENT_ENFORCED:
        return DeviceInitialEnrollmentState::
            INITIAL_ENROLLMENT_MODE_ENROLLMENT_ENFORCED;
      case DeviceStateRetrieval::RESTORE_MODE_DISABLED:
        return DeviceInitialEnrollmentState::INITIAL_ENROLLMENT_MODE_NONE;
      case DeviceStateRetrieval::RESTORE_MODE_REENROLLMENT_ZERO_TOUCH:
        return DeviceInitialEnrollmentState::INITIAL_ENROLLMENT_MODE_NONE;
    }
  }

  void ServerWillSendState(
      const std::string& management_domain,
      em::DeviceStateRetrievalResponse::RestoreMode restore_mode,
      const std::string& device_disabled_message) {
    if (GetParam() == AutoEnrollmentProtocol::kFRE) {
      ServerWillSendFREState(management_domain, restore_mode,
                             device_disabled_message);
    } else {
      ServerWillSendInitialEnrollmentState(
          management_domain,
          MapRestoreModeToInitialEnrollmentMode(restore_mode));
    }
  }

  DeviceManagementRequestJob::JobType GetStateRetrievalJobType() {
    return GetParam() == AutoEnrollmentProtocol::kFRE
               ? DeviceManagementRequestJob::TYPE_DEVICE_STATE_RETRIEVAL
               : DeviceManagementRequestJob::
                     TYPE_INITIAL_ENROLLMENT_STATE_RETRIEVAL;
  }

  void ServerWillSendFREState(
      const std::string& management_domain,
      em::DeviceStateRetrievalResponse::RestoreMode restore_mode,
      const std::string& device_disabled_message) {
    em::DeviceManagementResponse response;
    em::DeviceStateRetrievalResponse* state_response =
        response.mutable_device_state_retrieval_response();
    state_response->set_restore_mode(restore_mode);
    state_response->set_management_domain(management_domain);
    state_response->mutable_disabled_state()->set_message(
        device_disabled_message);
    EXPECT_CALL(*service_, CreateJob(GetStateRetrievalJobType(), _))
        .WillOnce(service_->SucceedJob(response));
  }

  void ServerWillSendInitialEnrollmentState(
      const std::string& management_domain,
      em::DeviceInitialEnrollmentStateResponse::InitialEnrollmentMode
          initial_enrollment_mode) {
    em::DeviceManagementResponse response;
    em::DeviceInitialEnrollmentStateResponse* state_response =
        response.mutable_device_initial_enrollment_state_response();
    state_response->set_initial_enrollment_mode(initial_enrollment_mode);
    state_response->set_management_domain(management_domain);
    EXPECT_CALL(*service_, CreateJob(GetStateRetrievalJobType(), _))
        .WillOnce(service_->SucceedJob(response));
  }

  void ServerWillReplyAsync(MockDeviceManagementJob** job) {
    EXPECT_CALL(*service_,
                CreateJob(DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT, _))
        .WillOnce(service_->CreateAsyncJob(job));
  }

  bool HasCachedDecision() {
    return local_state_->GetUserPref(prefs::kShouldAutoEnroll);
  }

  void VerifyCachedResult(bool should_enroll, int power_limit) {
    base::Value value_should_enroll(should_enroll);
    base::Value value_power_limit(power_limit);
    EXPECT_EQ(value_should_enroll,
              *local_state_->GetUserPref(prefs::kShouldAutoEnroll));
    EXPECT_EQ(value_power_limit,
              *local_state_->GetUserPref(prefs::kAutoEnrollmentPowerLimit));
  }

  bool HasServerBackedState() {
    return local_state_->GetUserPref(prefs::kServerBackedDeviceState);
  }

  void VerifyServerBackedState(const std::string& expected_management_domain,
                               const std::string& expected_restore_mode,
                               const std::string& expected_disabled_message) {
    const base::Value* state =
        local_state_->GetUserPref(prefs::kServerBackedDeviceState);
    ASSERT_TRUE(state);
    const base::DictionaryValue* state_dict = nullptr;
    ASSERT_TRUE(state->GetAsDictionary(&state_dict));

    std::string actual_management_domain;
    EXPECT_TRUE(state_dict->GetString(kDeviceStateManagementDomain,
                                      &actual_management_domain));
    EXPECT_EQ(expected_management_domain, actual_management_domain);

    if (!expected_restore_mode.empty()) {
      std::string actual_restore_mode;
      EXPECT_TRUE(
          state_dict->GetString(kDeviceStateRestoreMode, &actual_restore_mode));
      EXPECT_EQ(expected_restore_mode, actual_restore_mode);
    } else {
      EXPECT_FALSE(state_dict->HasKey(kDeviceStateRestoreMode));
    }

    std::string actual_disabled_message;
    if (GetParam() == AutoEnrollmentProtocol::kFRE) {
      EXPECT_TRUE(state_dict->GetString(kDeviceStateDisabledMessage,
                                        &actual_disabled_message));
      EXPECT_EQ(expected_disabled_message, actual_disabled_message);
    } else {
      EXPECT_FALSE(state_dict->GetString(kDeviceStateDisabledMessage,
                                         &actual_disabled_message));
    }
  }

  const em::DeviceAutoEnrollmentRequest& auto_enrollment_request() {
    return last_request_.auto_enrollment_request();
  }

  // Returns |client_| as |AutoEnrollmentClientImpl*|. This is fine because this
  // test only creates |client_| using |AutoEnrollmentClientImpl::FactoryImpl|.
  AutoEnrollmentClientImpl* client() {
    return static_cast<AutoEnrollmentClientImpl*>(client_.get());
  }

  // Releases |client_| and returns the pointer as |AutoEnrollmentClientImpl*|.
  // This is fine because this test only creates |client_| using
  // |AutoEnrollmentClientImpl::FactoryImpl|.
  AutoEnrollmentClientImpl* release_client() {
    return static_cast<AutoEnrollmentClientImpl*>(client_.release());
  }

  content::TestBrowserThreadBundle browser_threads_;
  ScopedTestingLocalState scoped_testing_local_state_;
  TestingPrefServiceSimple* local_state_;
  std::unique_ptr<MockDeviceManagementService> service_;
  em::DeviceManagementRequest last_request_;
  AutoEnrollmentState state_;

 private:
  std::unique_ptr<AutoEnrollmentClient> client_;
  DISALLOW_COPY_AND_ASSIGN(AutoEnrollmentClientImplTest);
};

TEST_P(AutoEnrollmentClientImplTest, NetworkFailure) {
  ServerWillFail(DM_STATUS_TEMPORARY_UNAVAILABLE);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, EmptyReply) {
  ServerWillReply(-1, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
  VerifyCachedResult(false, 8);
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, ClientUploadsRightBits) {
  ServerWillReply(-1, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);

  EXPECT_TRUE(auto_enrollment_request().has_remainder());
  EXPECT_TRUE(auto_enrollment_request().has_modulus());
  EXPECT_EQ(16, auto_enrollment_request().modulus());
  if (GetParam() == AutoEnrollmentProtocol::kFRE) {
    EXPECT_EQ(kStateKeyHash[31] & 0xf, auto_enrollment_request().remainder());
  } else {
    EXPECT_EQ(kInitialEnrollmentIdHash[7] & 0xf,
              auto_enrollment_request().remainder());
  }
  VerifyCachedResult(false, 8);
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, AskForMoreThenFail) {
  InSequence sequence;
  ServerWillReply(32, false, false);
  ServerWillFail(DM_STATUS_TEMPORARY_UNAVAILABLE);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, AskForMoreThenEvenMore) {
  InSequence sequence;
  ServerWillReply(32, false, false);
  ServerWillReply(64, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, AskForLess) {
  InSequence sequence;
  ServerWillReply(8, false, false);
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, AskForSame) {
  InSequence sequence;
  ServerWillReply(16, false, false);
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, AskForSameTwice) {
  InSequence sequence;
  ServerWillReply(16, false, false);
  ServerWillReply(16, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, AskForTooMuch) {
  ServerWillReply(512, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, DetectOutdatedServer) {
  CreateClient(0, kInitialEnrollmentModulusPowerOutdatedServer + 1);
  InSequence sequence;
  ServerWillReply(1 << kInitialEnrollmentModulusPowerOutdatedServer, false,
                  false);

  if (GetParam() == AutoEnrollmentProtocol::kInitialEnrollment) {
    // For initial enrollment, a modulus power higher or equal to
    // |kInitialEnrollmentModulusPowerOutdatedServer| means that the client will
    // detect the server as outdated and will skip enrollment.
    client()->Start();
    EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
    EXPECT_TRUE(HasCachedDecision());
    EXPECT_FALSE(HasServerBackedState());
  } else {
    // For FRE, such a detection does not exist. The client will do the second
    // round and upload bits of its device identifier hash.
    ServerWillReply(-1, false, false);
    client()->Start();
    EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
    EXPECT_TRUE(HasCachedDecision());
    EXPECT_FALSE(HasServerBackedState());
  }
}

TEST_P(AutoEnrollmentClientImplTest, AskNonPowerOf2) {
  InSequence sequence;
  ServerWillReply(100, false, false);
  ServerWillReply(-1, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
  EXPECT_TRUE(auto_enrollment_request().has_remainder());
  EXPECT_TRUE(auto_enrollment_request().has_modulus());
  EXPECT_EQ(128, auto_enrollment_request().modulus());
  if (GetParam() == AutoEnrollmentProtocol::kFRE) {
    EXPECT_EQ(kStateKeyHash[31] & 0x7f, auto_enrollment_request().remainder());
  } else {
    EXPECT_EQ(kInitialEnrollmentIdHash[7] & 0x7f,
              auto_enrollment_request().remainder());
  }
  VerifyCachedResult(false, 8);
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, ConsumerDevice) {
  ServerWillReply(-1, true, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
  VerifyCachedResult(false, 8);
  EXPECT_FALSE(HasServerBackedState());

  // Network changes don't trigger retries after obtaining a response from
  // the server.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
}

TEST_P(AutoEnrollmentClientImplTest, ForcedReEnrollment) {
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);

  // Network changes don't trigger retries after obtaining a response from
  // the server.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
}

TEST_P(AutoEnrollmentClientImplTest, ForcedReEnrollmentZeroTouch) {
  // Zero-Touch is currently not supported in the initial-enrollment exchange.
  if (GetParam() == AutoEnrollmentProtocol::kInitialEnrollment)
    return;

  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ZERO_TOUCH,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ZERO_TOUCH, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentZeroTouch,
                          kDisabledMessage);

  // Network changes don't trigger retries after obtaining a response from
  // the server.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ZERO_TOUCH, state_);
}

TEST_P(AutoEnrollmentClientImplTest, RequestedReEnrollment) {
  // Requesting re-enrollment is currently not supported in the
  // initial-enrollment exchange.
  if (GetParam() == AutoEnrollmentProtocol::kInitialEnrollment)
    return;

  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_REQUESTED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentRequested,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, DeviceDisabled) {
  // Disabling is currently not supported in the initial-enrollment exchange.
  if (GetParam() == AutoEnrollmentProtocol::kInitialEnrollment)
    return;

  ServerWillReply(-1, true, true);
  ServerWillSendState("example.com",
                      em::DeviceStateRetrievalResponse::RESTORE_MODE_DISABLED,
                      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
  VerifyCachedResult(true, 8);
  VerifyServerBackedState("example.com", kDeviceStateRestoreModeDisabled,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, NoBitsUploaded) {
  CreateClient(0, 0);
  ServerWillReply(-1, false, false);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
  EXPECT_TRUE(auto_enrollment_request().has_remainder());
  EXPECT_TRUE(auto_enrollment_request().has_modulus());
  EXPECT_EQ(1, auto_enrollment_request().modulus());
  EXPECT_EQ(0, auto_enrollment_request().remainder());
  VerifyCachedResult(false, 0);
  EXPECT_FALSE(HasServerBackedState());
}

TEST_P(AutoEnrollmentClientImplTest, ManyBitsUploaded) {
  int64_t bottom62 = GetParam() == AutoEnrollmentProtocol::kFRE
                         ? INT64_C(0x386e7244d097c3e6)
                         : INT64_C(0x3018b70f7609c5c7);
  for (int i = 0; i <= 62; ++i) {
    CreateClient(i, i);
    ServerWillReply(-1, false, false);
    client()->Start();
    EXPECT_EQ(AUTO_ENROLLMENT_STATE_NO_ENROLLMENT, state_);
    EXPECT_TRUE(auto_enrollment_request().has_remainder());
    EXPECT_TRUE(auto_enrollment_request().has_modulus());
    EXPECT_EQ(INT64_C(1) << i, auto_enrollment_request().modulus());
    EXPECT_EQ(bottom62 % (INT64_C(1) << i),
              auto_enrollment_request().remainder());
    VerifyCachedResult(false, i);
    EXPECT_FALSE(HasServerBackedState());
  }
}

TEST_P(AutoEnrollmentClientImplTest, MoreThan32BitsUploaded) {
  // Skip for initial enrollment, because the outdated server detection would
  // kick in when more than |kInitialEnrollmentModulusPowerOutdatedServer| bits
  // are requested.
  if (GetParam() == AutoEnrollmentProtocol::kInitialEnrollment)
    return;

  CreateClient(10, 37);
  InSequence sequence;
  ServerWillReply(INT64_C(1) << 37, false, false);
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyCachedResult(true, 37);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, ReuseCachedDecision) {
  // No bucket download requests should be issued.
  EXPECT_CALL(*service_,
              CreateJob(DeviceManagementRequestJob::TYPE_AUTO_ENROLLMENT, _))
      .Times(0);
  local_state_->SetUserPref(prefs::kShouldAutoEnroll,
                            std::make_unique<base::Value>(true));
  local_state_->SetUserPref(prefs::kAutoEnrollmentPowerLimit,
                            std::make_unique<base::Value>(8));

  // Note that device state will be retrieved every time, regardless of any
  // cached information. This is intentional, the idea is that device state on
  // the server may change.
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);

  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, RetryIfPowerLargerThanCached) {
  local_state_->SetUserPref(prefs::kShouldAutoEnroll,
                            std::make_unique<base::Value>(false));
  local_state_->SetUserPref(prefs::kAutoEnrollmentPowerLimit,
                            std::make_unique<base::Value>(8));
  CreateClient(5, 10);
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, NetworkChangeRetryAfterErrors) {
  ServerWillFail(DM_STATUS_TEMPORARY_UNAVAILABLE);
  client()->Start();
  // Don't invoke the callback if there was a network failure.
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());

  // The client doesn't retry if no new connection became available.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());

  // Retry once the network is back.
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  EXPECT_TRUE(HasCachedDecision());
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);

  // Subsequent network changes don't trigger retries.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_NONE);
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  EXPECT_TRUE(HasCachedDecision());
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
}

TEST_P(AutoEnrollmentClientImplTest, CancelAndDeleteSoonWithPendingRequest) {
  MockDeviceManagementJob* job = NULL;
  ServerWillReplyAsync(&job);
  EXPECT_FALSE(job);
  client()->Start();
  ASSERT_TRUE(job);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);

  // Cancel while a request is in flight.
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  release_client()->CancelAndDeleteSoon();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());

  // The client cleans itself up once a reply is received.
  job->SendResponse(DM_STATUS_TEMPORARY_UNAVAILABLE,
                    em::DeviceManagementResponse());
  // The DeleteSoon task has been posted:
  EXPECT_FALSE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);
}

TEST_P(AutoEnrollmentClientImplTest, NetworkChangedAfterCancelAndDeleteSoon) {
  MockDeviceManagementJob* job = NULL;
  ServerWillReplyAsync(&job);
  EXPECT_FALSE(job);
  client()->Start();
  ASSERT_TRUE(job);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);

  // Cancel while a request is in flight.
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  AutoEnrollmentClientImpl* client = release_client();
  client->CancelAndDeleteSoon();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());

  // Network change events are ignored while a request is pending.
  client->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);

  // The client cleans itself up once a reply is received.
  job->SendResponse(DM_STATUS_TEMPORARY_UNAVAILABLE,
                    em::DeviceManagementResponse());
  // The DeleteSoon task has been posted:
  EXPECT_FALSE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);

  // Network changes that have been posted before are also ignored:
  client->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_PENDING, state_);
}

TEST_P(AutoEnrollmentClientImplTest, CancelAndDeleteSoonAfterCompletion) {
  ServerWillReply(-1, true, true);
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);

  // The client will delete itself immediately if there are no pending
  // requests.
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  release_client()->CancelAndDeleteSoon();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
}

TEST_P(AutoEnrollmentClientImplTest, CancelAndDeleteSoonAfterNetworkFailure) {
  ServerWillFail(DM_STATUS_TEMPORARY_UNAVAILABLE);
  client()->Start();
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_SERVER_ERROR, state_);

  // The client will delete itself immediately if there are no pending
  // requests.
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
  release_client()->CancelAndDeleteSoon();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
}

TEST_P(AutoEnrollmentClientImplTest, NetworkFailureThenRequireUpdatedModulus) {
  // This test verifies that if the first request fails due to a network
  // problem then the second request will correctly handle an updated
  // modulus request from the server.

  ServerWillFail(DM_STATUS_REQUEST_FAILED);
  client()->Start();
  // Callback should signal the connection error.
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_CONNECTION_ERROR, state_);
  EXPECT_FALSE(HasCachedDecision());
  EXPECT_FALSE(HasServerBackedState());
  Mock::VerifyAndClearExpectations(service_.get());

  InSequence sequence;
  // The default client uploads 4 bits. Make the server ask for 5.
  ServerWillReply(1 << 5, false, false);
  EXPECT_CALL(*service_, StartJob(_, _, _, _, _, _));
  // Then reply with a valid response and include the hash.
  ServerWillReply(-1, true, true);
  EXPECT_CALL(*service_, StartJob(_, _, _, _, _, _));
  // State download triggers.
  ServerWillSendState(
      "example.com",
      em::DeviceStateRetrievalResponse::RESTORE_MODE_REENROLLMENT_ENFORCED,
      kDisabledMessage);
  EXPECT_CALL(*service_, StartJob(_, _, _, _, _, _));

  // Trigger a network change event.
  client()->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_EQ(AUTO_ENROLLMENT_STATE_TRIGGER_ENROLLMENT, state_);
  EXPECT_TRUE(HasCachedDecision());
  VerifyServerBackedState("example.com",
                          kDeviceStateRestoreModeReEnrollmentEnforced,
                          kDisabledMessage);
  Mock::VerifyAndClearExpectations(service_.get());
}

INSTANTIATE_TEST_CASE_P(FRE,
                        AutoEnrollmentClientImplTest,
                        testing::Values(AutoEnrollmentProtocol::kFRE));
INSTANTIATE_TEST_CASE_P(
    InitialEnrollment,
    AutoEnrollmentClientImplTest,
    testing::Values(AutoEnrollmentProtocol::kInitialEnrollment));

}  // namespace
}  // namespace policy
