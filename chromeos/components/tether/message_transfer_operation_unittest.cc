// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/message_transfer_operation.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/timer/mock_timer.h"
#include "chromeos/components/tether/connection_reason.h"
#include "chromeos/components/tether/fake_ble_connection_manager.h"
#include "chromeos/components/tether/message_wrapper.h"
#include "chromeos/components/tether/proto_test_util.h"
#include "chromeos/components/tether/timer_factory.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

// Arbitrarily chosen value. The MessageType used in this test does not matter
// except that it must be consistent throughout the test.
const MessageType kTestMessageType = MessageType::TETHER_AVAILABILITY_REQUEST;

const uint32_t kTestTimeoutSeconds = 5;

// A test double for MessageTransferOperation is needed because
// MessageTransferOperation has pure virtual methods which must be overridden in
// order to create a concrete instantiation of the class.
class TestOperation : public MessageTransferOperation {
 public:
  TestOperation(const cryptauth::RemoteDeviceRefList& devices_to_connect,
                BleConnectionManager* connection_manager)
      : MessageTransferOperation(devices_to_connect, connection_manager) {}
  ~TestOperation() override = default;

  bool HasDeviceAuthenticated(cryptauth::RemoteDeviceRef remote_device) {
    const auto iter = device_map_.find(remote_device);
    if (iter == device_map_.end()) {
      return false;
    }

    return iter->second.has_device_authenticated;
  }

  std::vector<std::shared_ptr<MessageWrapper>> GetReceivedMessages(
      cryptauth::RemoteDeviceRef remote_device) {
    const auto iter = device_map_.find(remote_device);
    if (iter == device_map_.end()) {
      return std::vector<std::shared_ptr<MessageWrapper>>();
    }

    return iter->second.received_messages;
  }

  // MessageTransferOperation:
  void OnDeviceAuthenticated(
      cryptauth::RemoteDeviceRef remote_device) override {
    device_map_[remote_device].has_device_authenticated = true;
  }

  void OnMessageReceived(std::unique_ptr<MessageWrapper> message_wrapper,
                         cryptauth::RemoteDeviceRef remote_device) override {
    device_map_[remote_device].received_messages.push_back(
        std::move(message_wrapper));

    if (should_unregister_device_on_message_received_)
      UnregisterDevice(remote_device);
  }

  void OnOperationStarted() override { has_operation_started_ = true; }

  void OnOperationFinished() override { has_operation_finished_ = true; }

  MessageType GetMessageTypeForConnection() override {
    return kTestMessageType;
  }

  uint32_t GetTimeoutSeconds() override { return timeout_seconds_; }

  void set_timeout_seconds(uint32_t timeout_seconds) {
    timeout_seconds_ = timeout_seconds;
  }

  void set_should_unregister_device_on_message_received(
      bool should_unregister_device_on_message_received) {
    should_unregister_device_on_message_received_ =
        should_unregister_device_on_message_received;
  }

  bool has_operation_started() { return has_operation_started_; }

  bool has_operation_finished() { return has_operation_finished_; }

 private:
  struct DeviceMapValue {
    DeviceMapValue() = default;
    ~DeviceMapValue() = default;

    bool has_device_authenticated;
    std::vector<std::shared_ptr<MessageWrapper>> received_messages;
  };

  std::map<cryptauth::RemoteDeviceRef, DeviceMapValue> device_map_;

  uint32_t timeout_seconds_ = kTestTimeoutSeconds;
  bool should_unregister_device_on_message_received_ = false;
  bool has_operation_started_ = false;
  bool has_operation_finished_ = false;
};

class TestTimerFactory : public TimerFactory {
 public:
  ~TestTimerFactory() override = default;

  // TimerFactory:
  std::unique_ptr<base::Timer> CreateOneShotTimer() override {
    EXPECT_FALSE(device_id_for_next_timer_.empty());
    base::MockTimer* mock_timer = new base::MockTimer(
        false /* retain_user_task */, false /* is_repeating */);
    device_id_to_timer_map_[device_id_for_next_timer_] = mock_timer;
    return base::WrapUnique(mock_timer);
  }

  base::MockTimer* GetTimerForDeviceId(const std::string& device_id) {
    return device_id_to_timer_map_[device_id_for_next_timer_];
  }

  void set_device_id_for_next_timer(
      const std::string& device_id_for_next_timer) {
    device_id_for_next_timer_ = device_id_for_next_timer;
  }

 private:
  std::string device_id_for_next_timer_;
  std::unordered_map<std::string, base::MockTimer*> device_id_to_timer_map_;
};

TetherAvailabilityResponse CreateTetherAvailabilityResponse() {
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode::
          TetherAvailabilityResponse_ResponseCode_TETHER_AVAILABLE);
  response.mutable_device_status()->CopyFrom(
      CreateDeviceStatusWithFakeFields());
  return response;
}

}  // namespace

class MessageTransferOperationTest : public testing::Test {
 protected:
  MessageTransferOperationTest()
      : test_devices_(cryptauth::CreateRemoteDeviceRefListForTest(4)) {
    // These tests are written under the assumption that there are a maximum of
    // 3 "empty scan" connection attempts and 6 "GATT" connection attempts; the
    // tests need to be edited if these values change.
    EXPECT_EQ(3u, MessageTransferOperation::kMaxEmptyScansPerDevice);
    EXPECT_EQ(6u,
              MessageTransferOperation::kMaxGattConnectionAttemptsPerDevice);
  }

  void SetUp() override {
    fake_ble_connection_manager_ = std::make_unique<FakeBleConnectionManager>();
  }

  void ConstructOperation(cryptauth::RemoteDeviceRefList remote_devices) {
    test_timer_factory_ = new TestTimerFactory();
    operation_ = base::WrapUnique(
        new TestOperation(remote_devices, fake_ble_connection_manager_.get()));
    operation_->SetTimerFactoryForTest(base::WrapUnique(test_timer_factory_));
    VerifyOperationStartedAndFinished(false /* has_started */,
                                      false /* has_finished */);
  }

  void InitializeOperation() {
    VerifyOperationStartedAndFinished(false /* has_started */,
                                      false /* has_finished */);
    operation_->Initialize();
    VerifyOperationStartedAndFinished(true /* has_started */,
                                      false /* has_finished */);
  }

  void VerifyOperationStartedAndFinished(bool has_started, bool has_finished) {
    EXPECT_EQ(has_started, operation_->has_operation_started());
    EXPECT_EQ(has_finished, operation_->has_operation_finished());
  }

  void TransitionDeviceStatusFromDisconnectedToAuthenticated(
      cryptauth::RemoteDeviceRef remote_device) {
    test_timer_factory_->set_device_id_for_next_timer(
        remote_device.GetDeviceId());

    fake_ble_connection_manager_->SetDeviceStatus(
        remote_device.GetDeviceId(),
        cryptauth::SecureChannel::Status::CONNECTING,
        BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    fake_ble_connection_manager_->SetDeviceStatus(
        remote_device.GetDeviceId(),
        cryptauth::SecureChannel::Status::CONNECTED,
        BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    fake_ble_connection_manager_->SetDeviceStatus(
        remote_device.GetDeviceId(),
        cryptauth::SecureChannel::Status::AUTHENTICATING,
        BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    fake_ble_connection_manager_->SetDeviceStatus(
        remote_device.GetDeviceId(),
        cryptauth::SecureChannel::Status::AUTHENTICATED,
        BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  }

  base::MockTimer* GetTimerForDevice(cryptauth::RemoteDeviceRef remote_device) {
    return test_timer_factory_->GetTimerForDeviceId(
        remote_device.GetDeviceId());
  }

  void VerifyDefaultTimerCreatedForDevice(
      cryptauth::RemoteDeviceRef remote_device) {
    VerifyTimerCreatedForDevice(remote_device, kTestTimeoutSeconds);
  }

  void VerifyTimerCreatedForDevice(cryptauth::RemoteDeviceRef remote_device,
                                   uint32_t timeout_seconds) {
    EXPECT_TRUE(GetTimerForDevice(remote_device));
    EXPECT_EQ(base::TimeDelta::FromSeconds(timeout_seconds),
              GetTimerForDevice(remote_device)->GetCurrentDelay());
  }

  const cryptauth::RemoteDeviceRefList test_devices_;

  std::unique_ptr<FakeBleConnectionManager> fake_ble_connection_manager_;
  TestTimerFactory* test_timer_factory_;
  std::unique_ptr<TestOperation> operation_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageTransferOperationTest);
};

TEST_F(MessageTransferOperationTest, CannotReceiveResponse_RetryLimitReached) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try to connect and fail. The device should still be registered.
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTING,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::DISCONNECTED,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try and fail again. The device should still be registered.
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTING,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::DISCONNECTED,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try and fail a third time. The maximum number of unanswered failures has
  // been reached, so the device should be unregistered.
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTING,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::DISCONNECTED,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  VerifyOperationStartedAndFinished(true /* has_started */,
                                    true /* has_finished */);

  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  EXPECT_TRUE(operation_->GetReceivedMessages(test_devices_[0]).empty());
}

TEST_F(MessageTransferOperationTest,
       CannotCompleteGattConnection_RetryLimitReached) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  fake_ble_connection_manager_->SimulateGattErrorConnectionAttempts(
      test_devices_[0].GetDeviceId(),
      MessageTransferOperation::kMaxGattConnectionAttemptsPerDevice);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  VerifyOperationStartedAndFinished(true /* has_started */,
                                    true /* has_finished */);
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  EXPECT_TRUE(operation_->GetReceivedMessages(test_devices_[0]).empty());
}

TEST_F(MessageTransferOperationTest, MixedConnectionAttemptFailures) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Fail to establish a connection one fewer time than the maximum allowed. The
  // device should still be registered since the maximum was not hit.
  fake_ble_connection_manager_->SimulateUnansweredConnectionAttempts(
      test_devices_[0].GetDeviceId(),
      MessageTransferOperation::kMaxEmptyScansPerDevice - 1);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Now, fail to establish a connection via GATT errors.
  fake_ble_connection_manager_->SimulateGattErrorConnectionAttempts(
      test_devices_[0].GetDeviceId(),
      MessageTransferOperation::kMaxGattConnectionAttemptsPerDevice);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  VerifyOperationStartedAndFinished(true /* has_started */,
                                    true /* has_finished */);
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  EXPECT_TRUE(operation_->GetReceivedMessages(test_devices_[0]).empty());
}

TEST_F(MessageTransferOperationTest, TestFailsThenConnects_Unanswered) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try to connect and fail. The device should still be registered.
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTING,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::DISCONNECTED,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try again and succeed.
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  EXPECT_TRUE(operation_->GetReceivedMessages(test_devices_[0]).empty());
}

TEST_F(MessageTransferOperationTest, TestFailsThenConnects_GattError) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try to connect and fail. The device should still be registered.
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTING,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::CONNECTED,
      BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
  fake_ble_connection_manager_->SetDeviceStatus(
      test_devices_[0].GetDeviceId(),
      cryptauth::SecureChannel::Status::DISCONNECTED,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Try again and succeed.
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  EXPECT_TRUE(operation_->GetReceivedMessages(test_devices_[0]).empty());
}

TEST_F(MessageTransferOperationTest,
       TestSuccessfulConnectionAndReceiveMessage) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Simulate how subclasses behave after a successful response: unregister the
  // device.
  operation_->set_should_unregister_device_on_message_received(true);

  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  fake_ble_connection_manager_->ReceiveMessage(
      test_devices_[0].GetDeviceId(),
      MessageWrapper(CreateTetherAvailabilityResponse()).ToRawMessage());

  EXPECT_EQ(1u, operation_->GetReceivedMessages(test_devices_[0]).size());
  std::shared_ptr<MessageWrapper> message =
      operation_->GetReceivedMessages(test_devices_[0])[0];
  EXPECT_EQ(MessageType::TETHER_AVAILABILITY_RESPONSE,
            message->GetMessageType());
  EXPECT_EQ(CreateTetherAvailabilityResponse().SerializeAsString(),
            message->GetProto()->SerializeAsString());
}

TEST_F(MessageTransferOperationTest, TestDevicesUnregisteredAfterDeletion) {
  ConstructOperation(test_devices_);
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[1].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[2].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[3].GetDeviceId()));

  // Delete the operation. All registered devices should be unregistered.
  operation_.reset();
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[1].GetDeviceId()));
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[2].GetDeviceId()));
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[3].GetDeviceId()));
}

TEST_F(MessageTransferOperationTest,
       TestSuccessfulConnectionAndReceiveMessage_TimeoutSeconds) {
  const uint32_t kTimeoutSeconds = 90;

  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  operation_->set_timeout_seconds(kTimeoutSeconds);

  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyTimerCreatedForDevice(test_devices_[0], kTimeoutSeconds);

  EXPECT_EQ(base::TimeDelta::FromSeconds(kTimeoutSeconds),
            GetTimerForDevice(test_devices_[0])->GetCurrentDelay());

  fake_ble_connection_manager_->ReceiveMessage(
      test_devices_[0].GetDeviceId(),
      MessageWrapper(CreateTetherAvailabilityResponse()).ToRawMessage());

  EXPECT_EQ(1u, operation_->GetReceivedMessages(test_devices_[0]).size());
  std::shared_ptr<MessageWrapper> message =
      operation_->GetReceivedMessages(test_devices_[0])[0];
  EXPECT_EQ(MessageType::TETHER_AVAILABILITY_RESPONSE,
            message->GetMessageType());
  EXPECT_EQ(CreateTetherAvailabilityResponse().SerializeAsString(),
            message->GetProto()->SerializeAsString());
}

TEST_F(MessageTransferOperationTest, TestAuthenticatesButTimesOut) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  GetTimerForDevice(test_devices_[0])->Fire();

  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->has_operation_finished());
}

TEST_F(MessageTransferOperationTest, TestRepeatedInputDevice) {
  // Construct with two copies of the same device.
  ConstructOperation(
      cryptauth::RemoteDeviceRefList{test_devices_[0], test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  fake_ble_connection_manager_->ReceiveMessage(
      test_devices_[0].GetDeviceId(),
      MessageWrapper(CreateTetherAvailabilityResponse()).ToRawMessage());

  // Should still have received only one message even though the device was
  // repeated twice in the constructor.
  EXPECT_EQ(1u, operation_->GetReceivedMessages(test_devices_[0]).size());
  std::shared_ptr<MessageWrapper> message =
      operation_->GetReceivedMessages(test_devices_[0])[0];
  EXPECT_EQ(MessageType::TETHER_AVAILABILITY_RESPONSE,
            message->GetMessageType());
  EXPECT_EQ(CreateTetherAvailabilityResponse().SerializeAsString(),
            message->GetProto()->SerializeAsString());
}

TEST_F(MessageTransferOperationTest, TestReceiveEventForOtherDevice) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));

  // Simulate the authentication of |test_devices_[1]|'s channel. Since the
  // operation was only constructed with |test_devices_[0]|, this operation
  // should not be affected.
  fake_ble_connection_manager_->RegisterRemoteDevice(
      test_devices_[1].GetDeviceId(),
      ConnectionReason::CONNECT_TETHERING_REQUEST);
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[1]);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[1].GetDeviceId()));
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[1]));

  // Now, receive a message for |test_devices[1]|. Likewise, this operation
  // should not be affected.
  fake_ble_connection_manager_->ReceiveMessage(
      test_devices_[1].GetDeviceId(),
      MessageWrapper(CreateTetherAvailabilityResponse()).ToRawMessage());

  EXPECT_FALSE(operation_->GetReceivedMessages(test_devices_[0]).size());
}

TEST_F(MessageTransferOperationTest,
       TestAlreadyAuthenticatedBeforeInitialization) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});

  // Simulate the authentication of |test_devices_[0]|'s channel before
  // initialization.
  fake_ble_connection_manager_->RegisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::CONNECT_TETHERING_REQUEST);
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);

  // Now initialize; the authentication handler should have been invoked.
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  // Receiving a message should work at this point.
  fake_ble_connection_manager_->ReceiveMessage(
      test_devices_[0].GetDeviceId(),
      MessageWrapper(CreateTetherAvailabilityResponse()).ToRawMessage());

  EXPECT_EQ(1u, operation_->GetReceivedMessages(test_devices_[0]).size());
  std::shared_ptr<MessageWrapper> message =
      operation_->GetReceivedMessages(test_devices_[0])[0];
  EXPECT_EQ(MessageType::TETHER_AVAILABILITY_RESPONSE,
            message->GetMessageType());
  EXPECT_EQ(CreateTetherAvailabilityResponse().SerializeAsString(),
            message->GetProto()->SerializeAsString());
}

TEST_F(MessageTransferOperationTest,
       AlreadyAuthenticatedBeforeInitialization_TimesOut) {
  ConstructOperation(cryptauth::RemoteDeviceRefList{test_devices_[0]});

  // Simulate the authentication of |test_devices_[0]|'s channel before
  // initialization.
  fake_ble_connection_manager_->RegisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::CONNECT_TETHERING_REQUEST);
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);

  // Now initialize; the authentication handler should have been invoked.
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  GetTimerForDevice(test_devices_[0])->Fire();
  EXPECT_TRUE(operation_->has_operation_finished());

  // Should still be registered since it was also registered for the
  // CONNECT_TETHERING_REQUEST MessageType.
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
}

TEST_F(MessageTransferOperationTest, MultipleDevices) {
  ConstructOperation(test_devices_);
  InitializeOperation();
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[1].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[2].GetDeviceId()));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[3].GetDeviceId()));

  // Authenticate |test_devices_[0]|'s channel.
  fake_ble_connection_manager_->RegisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::CONNECT_TETHERING_REQUEST);
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[0]);
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[0]));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[0].GetDeviceId()));
  VerifyDefaultTimerCreatedForDevice(test_devices_[0]);

  // Fail 3 unanswered times to connect to |test_devices_[1]|.
  test_timer_factory_->set_device_id_for_next_timer(
      test_devices_[1].GetDeviceId());
  fake_ble_connection_manager_->SimulateUnansweredConnectionAttempts(
      test_devices_[1].GetDeviceId(),
      MessageTransferOperation::kMaxEmptyScansPerDevice);
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[1]));
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[1].GetDeviceId()));
  EXPECT_FALSE(GetTimerForDevice(test_devices_[1]));

  // Authenticate |test_devices_[2]|'s channel.
  fake_ble_connection_manager_->RegisterRemoteDevice(
      test_devices_[2].GetDeviceId(),
      ConnectionReason::CONNECT_TETHERING_REQUEST);
  TransitionDeviceStatusFromDisconnectedToAuthenticated(test_devices_[2]);
  EXPECT_TRUE(operation_->HasDeviceAuthenticated(test_devices_[2]));
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[2].GetDeviceId()));
  VerifyDefaultTimerCreatedForDevice(test_devices_[2]);

  // Fail 3 unanswered times to connect to |test_devices_[3]|.
  test_timer_factory_->set_device_id_for_next_timer(
      test_devices_[3].GetDeviceId());
  fake_ble_connection_manager_->SimulateUnansweredConnectionAttempts(
      test_devices_[3].GetDeviceId(),
      MessageTransferOperation::kMaxEmptyScansPerDevice);
  EXPECT_FALSE(operation_->HasDeviceAuthenticated(test_devices_[3]));
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(
      test_devices_[3].GetDeviceId()));
  EXPECT_FALSE(GetTimerForDevice(test_devices_[3]));
}

}  // namespace tether

}  // namespace chromeos
