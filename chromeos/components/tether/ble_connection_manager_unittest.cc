// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_connection_manager.h"

#include "base/memory/ptr_util.h"
#include "base/timer/mock_timer.h"
#include "chromeos/components/tether/ble_constants.h"
#include "chromeos/components/tether/connection_reason.h"
#include "chromeos/components/tether/fake_ad_hoc_ble_advertiser.h"
#include "chromeos/components/tether/fake_ble_advertiser.h"
#include "chromeos/components/tether/fake_ble_scanner.h"
#include "chromeos/components/tether/proto/tether.pb.h"
#include "chromeos/components/tether/timer_factory.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_client_connection.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/fake_cryptauth_service.h"
#include "components/cryptauth/fake_secure_channel.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace chromeos {

namespace tether {

namespace {

const char kTetherFeature[] = "magic_tether";

const char kBluetoothAddress1[] = "11:22:33:44:55:66";
const char kBluetoothAddress2[] = "22:33:44:55:66:77";
const char kBluetoothAddress3[] = "33:44:55:66:77:88";

struct SecureChannelStatusChange {
  SecureChannelStatusChange(
      const std::string& device_id,
      const cryptauth::SecureChannel::Status& old_status,
      const cryptauth::SecureChannel::Status& new_status,
      BleConnectionManager::StateChangeDetail status_change_detail)
      : device_id(device_id),
        old_status(old_status),
        new_status(new_status),
        status_change_detail(status_change_detail) {}

  std::string device_id;
  cryptauth::SecureChannel::Status old_status;
  cryptauth::SecureChannel::Status new_status;
  BleConnectionManager::StateChangeDetail status_change_detail;
};

struct ReceivedMessage {
  ReceivedMessage(const std::string& device_id, const std::string& payload)
      : device_id(device_id), payload(payload) {}

  std::string device_id;
  std::string payload;
};

class MockTimerFactory : public TimerFactory {
 public:
  std::unique_ptr<base::Timer> CreateOneShotTimer() override {
    return std::make_unique<base::MockTimer>(false /* retains_user_task */,
                                             false /* is_repeating */);
  }
};

// Observer used in all tests except for ObserverUnregisters() which tracks all
// status changes and messages received.
class TestObserver final : public BleConnectionManager::Observer {
 public:
  TestObserver() = default;

  // BleConnectionManager::Observer:
  void OnSecureChannelStatusChanged(
      const std::string& device_id,
      const cryptauth::SecureChannel::Status& old_status,
      const cryptauth::SecureChannel::Status& new_status,
      BleConnectionManager::StateChangeDetail status_change_detail) override {
    connection_status_changes_.emplace_back(device_id, old_status, new_status,
                                            status_change_detail);
  }

  void OnMessageReceived(const std::string& device_id,
                         const std::string& payload) override {
    received_messages_.push_back(ReceivedMessage(device_id, payload));
  }

  void OnMessageSent(int sequence_number) override {
    sent_sequence_numbers_.push_back(sequence_number);
  }

  const std::vector<SecureChannelStatusChange>& connection_status_changes() {
    return connection_status_changes_;
  }

  const std::vector<ReceivedMessage>& received_messages() {
    return received_messages_;
  }

  const std::vector<int>& sent_sequence_numbers() {
    return sent_sequence_numbers_;
  }

 private:
  std::vector<SecureChannelStatusChange> connection_status_changes_;
  std::vector<ReceivedMessage> received_messages_;
  std::vector<int> sent_sequence_numbers_;
};

// Observer used in ObserverUnregisters() which unregisters a device when it
// receives a callback from the manager. The device it unregisters is the same
// one it receives the event about.
class UnregisteringObserver : public BleConnectionManager::Observer {
 public:
  UnregisteringObserver(BleConnectionManager* manager,
                        ConnectionReason connection_reason)
      : manager_(manager), connection_reason_(connection_reason) {}

  // BleConnectionManager::Observer:
  void OnSecureChannelStatusChanged(
      const std::string& device_id,
      const cryptauth::SecureChannel::Status& old_status,
      const cryptauth::SecureChannel::Status& new_status,
      BleConnectionManager::StateChangeDetail status_change_detail) override {
    manager_->UnregisterRemoteDevice(device_id, connection_reason_);
  }

  void OnMessageReceived(const std::string& device_id,
                         const std::string& payload) override {
    manager_->UnregisterRemoteDevice(device_id, connection_reason_);
  }

  void OnMessageSent(int sequence_number) override { NOTIMPLEMENTED(); }

 private:
  BleConnectionManager* manager_;
  ConnectionReason connection_reason_;
};

class TestMetricsObserver final : public BleConnectionManager::MetricsObserver {
 public:
  TestMetricsObserver() = default;

  bool HasDeviceStartedScan(const std::string& device_id) {
    return base::ContainsKey(device_id_started_scan_set_, device_id);
  }

  bool HasDeviceReceivedAdvertisement(
      const std::string& device_id,
      bool is_background_advertisement_expected) {
    return base::ContainsKey(device_id_to_received_advertisement_map_,
                             device_id) &&
           device_id_to_received_advertisement_map_[device_id] ==
               is_background_advertisement_expected;
  }

  bool HasDeviceConnected(const std::string& device_id,
                          bool is_background_advertisement_expected) {
    return base::ContainsKey(device_id_to_connected_map_, device_id) &&
           device_id_to_connected_map_[device_id] ==
               is_background_advertisement_expected;
  }

  bool HasDeviceCreatedSecureChannel(
      const std::string& device_id,
      bool is_background_advertisement_expected) {
    return base::ContainsKey(device_id_to_secure_channel_created_map_,
                             device_id) &&
           device_id_to_secure_channel_created_map_[device_id] ==
               is_background_advertisement_expected;
  }

  bool HasDeviceDisconnected(
      const std::string& device_id,
      BleConnectionManager::StateChangeDetail expected_state_change_detail,
      bool is_background_advertisement_expected) {
    if (!base::ContainsKey(device_id_to_disconnected_map_, device_id))
      return false;

    for (std::pair<BleConnectionManager::StateChangeDetail, bool> event_pair :
         device_id_to_disconnected_map_[device_id]) {
      if (event_pair.first == expected_state_change_detail &&
          event_pair.second == is_background_advertisement_expected)
        return true;
    }

    return false;
  }

  // BleConnectionManager::MetricsObserver:
  void OnConnectionAttemptStarted(const std::string& device_id) override {
    device_id_started_scan_set_.insert(device_id);
  }

  void OnAdvertisementReceived(const std::string& device_id,
                               bool is_background_advertisement) override {
    device_id_to_received_advertisement_map_[device_id] =
        is_background_advertisement;
  }

  void OnConnection(const std::string& device_id,
                    bool is_background_advertisement) override {
    device_id_to_connected_map_[device_id] = is_background_advertisement;
  }

  void OnSecureChannelCreated(const std::string& device_id,
                              bool is_background_advertisement) override {
    device_id_to_secure_channel_created_map_[device_id] =
        is_background_advertisement;
  }

  void OnDeviceDisconnected(
      const std::string& device_id,
      BleConnectionManager::StateChangeDetail state_change_detail,
      bool is_background_advertisement) override {
    device_id_to_disconnected_map_[device_id].push_back(
        std::make_pair(state_change_detail, is_background_advertisement));
  }

 private:
  std::set<std::string> device_id_started_scan_set_;

  // Maps to whether the call indicated that it was a background advertisement.
  std::map<std::string, bool> device_id_to_received_advertisement_map_;
  std::map<std::string, bool> device_id_to_connected_map_;
  std::map<std::string, bool> device_id_to_secure_channel_created_map_;

  // The |second| item tracks if the call indicated that it was a background
  // advertisement.
  std::map<
      std::string,
      std::vector<std::pair<BleConnectionManager::StateChangeDetail, bool>>>
      device_id_to_disconnected_map_;
};

class FakeConnectionWithAddress : public cryptauth::FakeConnection {
 public:
  FakeConnectionWithAddress(cryptauth::RemoteDeviceRef remote_device,
                            const std::string& device_address)
      : FakeConnection(remote_device, false /* should_auto_connect */),
        device_address_(device_address) {}

  // cryptauth::Connection:
  std::string GetDeviceAddress() override { return device_address_; }

 private:
  const std::string device_address_;
};

class FakeConnectionFactory final
    : public cryptauth::weave::BluetoothLowEnergyWeaveClientConnection::
          Factory {
 public:
  FakeConnectionFactory(
      scoped_refptr<device::BluetoothAdapter> expected_adapter,
      const device::BluetoothUUID expected_remote_service_uuid)
      : expected_adapter_(expected_adapter),
        expected_remote_service_uuid_(expected_remote_service_uuid) {}

  std::unique_ptr<cryptauth::Connection> BuildInstance(
      cryptauth::RemoteDeviceRef remote_device,
      scoped_refptr<device::BluetoothAdapter> adapter,
      const device::BluetoothUUID remote_service_uuid,
      device::BluetoothDevice* bluetooth_device,
      bool should_set_low_connection_latency) override {
    EXPECT_EQ(expected_adapter_, adapter);
    EXPECT_EQ(expected_remote_service_uuid_, remote_service_uuid);
    EXPECT_FALSE(should_set_low_connection_latency);
    return base::WrapUnique(new FakeConnectionWithAddress(
        remote_device, bluetooth_device->GetAddress()));
  }

 private:
  scoped_refptr<device::BluetoothAdapter> expected_adapter_;
  const device::BluetoothUUID expected_remote_service_uuid_;
};

cryptauth::RemoteDeviceRefList CreateTestDevices(size_t num_to_create) {
  return cryptauth::CreateRemoteDeviceRefListForTest(num_to_create);
}

}  // namespace

class BleConnectionManagerTest : public testing::Test {
 protected:
  class FakeSecureChannel : public cryptauth::FakeSecureChannel {
   public:
    FakeSecureChannel(std::unique_ptr<cryptauth::Connection> connection,
                      cryptauth::CryptAuthService* cryptauth_service)
        : cryptauth::FakeSecureChannel(std::move(connection),
                                       cryptauth_service) {}
    ~FakeSecureChannel() override = default;

    void AddObserver(Observer* observer) override {
      cryptauth::FakeSecureChannel::AddObserver(observer);

      EXPECT_EQ(static_cast<size_t>(1), observers().size());
    }

    void RemoveObserver(Observer* observer) override {
      cryptauth::FakeSecureChannel::RemoveObserver(observer);
      EXPECT_EQ(static_cast<size_t>(0), observers().size());
    }
  };

  class FakeSecureChannelFactory final
      : public cryptauth::SecureChannel::Factory {
   public:
    FakeSecureChannelFactory() = default;

    std::vector<FakeSecureChannel*>& created_channels() {
      return created_channels_;
    }

    void SetExpectedDeviceAddress(const std::string& expected_device_address) {
      expected_device_address_ = expected_device_address;
    }

    std::unique_ptr<cryptauth::SecureChannel> BuildInstance(
        std::unique_ptr<cryptauth::Connection> connection,
        cryptauth::CryptAuthService* cryptauth_service) override {
      FakeConnectionWithAddress* fake_connection =
          static_cast<FakeConnectionWithAddress*>(connection.get());
      EXPECT_EQ(expected_device_address_, fake_connection->GetDeviceAddress());
      FakeSecureChannel* channel =
          new FakeSecureChannel(std::move(connection), cryptauth_service);
      created_channels_.push_back(channel);
      return base::WrapUnique(channel);
    }

   private:
    std::string expected_device_address_;
    std::vector<FakeSecureChannel*> created_channels_;
  };

  BleConnectionManagerTest() : test_devices_(CreateTestDevices(4)) {
    // These tests assume a maximum of two concurrent advertisers. Some of the
    // multi-device tests would need to be re-written if this constant changes.
    EXPECT_EQ(2u, kMaxConcurrentAdvertisements);
  }

  void SetUp() override {
    verified_status_changes_.clear();
    verified_received_messages_.clear();

    fake_cryptauth_service_ =
        std::make_unique<cryptauth::FakeCryptAuthService>();
    mock_adapter_ =
        base::MakeRefCounted<NiceMock<device::MockBluetoothAdapter>>();

    device_queue_ = std::make_unique<BleAdvertisementDeviceQueue>();

    fake_ble_advertiser_ = std::make_unique<FakeBleAdvertiser>(
        true /* automatically_update_active_advertisements */);

    fake_ble_scanner_ = std::make_unique<FakeBleScanner>(
        true /* automatically_update_discovery_session */);

    fake_ad_hoc_ble_advertiser_ = std::make_unique<FakeAdHocBleAdvertiser>();

    fake_connection_factory_ = base::WrapUnique(new FakeConnectionFactory(
        mock_adapter_, device::BluetoothUUID(kGattServerUuid)));
    cryptauth::weave::BluetoothLowEnergyWeaveClientConnection::Factory::
        SetInstanceForTesting(fake_connection_factory_.get());

    fake_secure_channel_factory_ =
        base::WrapUnique(new FakeSecureChannelFactory());
    cryptauth::SecureChannel::Factory::SetInstanceForTesting(
        fake_secure_channel_factory_.get());

    manager_ = base::WrapUnique(new BleConnectionManager(
        fake_cryptauth_service_.get(), mock_adapter_, device_queue_.get(),
        fake_ble_advertiser_.get(), fake_ble_scanner_.get(),
        fake_ad_hoc_ble_advertiser_.get()));
    test_observer_ = base::WrapUnique(new TestObserver());
    manager_->AddObserver(test_observer_.get());
    test_metrics_observer_ = base::WrapUnique(new TestMetricsObserver());
    manager_->AddMetricsObserver(test_metrics_observer_.get());

    mock_timer_factory_ = new MockTimerFactory();
    manager_->SetTestTimerFactoryForTesting(
        base::WrapUnique(mock_timer_factory_));
  }

  void TearDown() override {
    // All state changes should have already been verified. This ensures that
    // no test has missed one.
    VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>(),
                                 used_background_advertisements_);

    // Same with received messages.
    VerifyReceivedMessages(std::vector<ReceivedMessage>());
  }

  void VerifyConnectionStateChanges(
      const std::vector<SecureChannelStatusChange>& expected_changes,
      bool is_background_advertisement = false) {
    verified_status_changes_.insert(verified_status_changes_.end(),
                                    expected_changes.begin(),
                                    expected_changes.end());

    ASSERT_EQ(verified_status_changes_.size(),
              test_observer_->connection_status_changes().size());

    for (size_t i = 0; i < verified_status_changes_.size(); i++) {
      EXPECT_EQ(verified_status_changes_[i].device_id,
                test_observer_->connection_status_changes()[i].device_id);
      EXPECT_EQ(verified_status_changes_[i].old_status,
                test_observer_->connection_status_changes()[i].old_status);
      EXPECT_EQ(verified_status_changes_[i].new_status,
                test_observer_->connection_status_changes()[i].new_status);
      EXPECT_EQ(
          verified_status_changes_[i].status_change_detail,
          test_observer_->connection_status_changes()[i].status_change_detail);

      if (verified_status_changes_[i].new_status ==
          cryptauth::SecureChannel::Status::CONNECTING) {
        EXPECT_TRUE(test_metrics_observer_->HasDeviceStartedScan(
            verified_status_changes_[i].device_id));
      } else if (verified_status_changes_[i].new_status ==
                 cryptauth::SecureChannel::Status::CONNECTED) {
        EXPECT_TRUE(test_metrics_observer_->HasDeviceConnected(
            verified_status_changes_[i].device_id,
            is_background_advertisement));
      } else if (verified_status_changes_[i].new_status ==
                 cryptauth::SecureChannel::Status::AUTHENTICATED) {
        EXPECT_TRUE(test_metrics_observer_->HasDeviceCreatedSecureChannel(
            verified_status_changes_[i].device_id,
            is_background_advertisement));
      } else if (verified_status_changes_[i].new_status ==
                 cryptauth::SecureChannel::Status::DISCONNECTED) {
        EXPECT_TRUE(test_metrics_observer_->HasDeviceDisconnected(
            verified_status_changes_[i].device_id,
            verified_status_changes_[i].status_change_detail,
            is_background_advertisement));
      }
    }
  }

  void VerifyReceivedMessages(
      const std::vector<ReceivedMessage>& expected_messages) {
    verified_received_messages_.insert(verified_received_messages_.end(),
                                       expected_messages.begin(),
                                       expected_messages.end());

    ASSERT_EQ(verified_received_messages_.size(),
              test_observer_->received_messages().size());

    for (size_t i = 0; i < verified_received_messages_.size(); i++) {
      EXPECT_EQ(verified_received_messages_[i].device_id,
                test_observer_->received_messages()[i].device_id);
      EXPECT_EQ(verified_received_messages_[i].payload,
                test_observer_->received_messages()[i].payload);
    }
  }

  void VerifyNoTimeoutSet(cryptauth::RemoteDeviceRef remote_device) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    EXPECT_TRUE(connection_metadata);
    EXPECT_FALSE(
        connection_metadata->connection_attempt_timeout_timer_->IsRunning());
  }

  void VerifyFailImmediatelyTimeoutSet(
      cryptauth::RemoteDeviceRef remote_device) {
    VerifyTimeoutSet(remote_device,
                     BleConnectionManager::kFailImmediatelyTimeoutMillis);
  }

  void VerifyAdvertisingTimeoutSet(cryptauth::RemoteDeviceRef remote_device) {
    VerifyTimeoutSet(remote_device,
                     BleConnectionManager::kAdvertisingTimeoutMillis);
  }

  void VerifyTimeoutSet(cryptauth::RemoteDeviceRef remote_device,
                        int64_t expected_num_millis) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    EXPECT_TRUE(connection_metadata);
    EXPECT_TRUE(
        connection_metadata->connection_attempt_timeout_timer_->IsRunning());
    EXPECT_EQ(base::TimeDelta::FromMilliseconds(expected_num_millis),
              connection_metadata->connection_attempt_timeout_timer_
                  ->GetCurrentDelay());
  }

  void FireTimerForDevice(cryptauth::RemoteDeviceRef remote_device) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    EXPECT_TRUE(connection_metadata);
    EXPECT_TRUE(
        connection_metadata->connection_attempt_timeout_timer_->IsRunning());
    static_cast<base::MockTimer*>(
        connection_metadata->connection_attempt_timeout_timer_.get())
        ->Fire();
  }

  void NotifyReceivedAdvertisementFromDevice(
      const std::string& bluetooth_address,
      cryptauth::RemoteDeviceRef remote_device,
      bool is_background_advertisement) {
    device::MockBluetoothDevice device(
        mock_adapter_.get(), 0u /* bluetooth_class */, "name",
        bluetooth_address, false /* paired */, false /* connected */);
    fake_ble_scanner_->NotifyReceivedAdvertisementFromDevice(
        remote_device, &device, is_background_advertisement);
  }

  FakeSecureChannel* GetChannelForDevice(
      cryptauth::RemoteDeviceRef remote_device) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    EXPECT_TRUE(connection_metadata);
    EXPECT_TRUE(connection_metadata->secure_channel_);
    return static_cast<FakeSecureChannel*>(
        connection_metadata->secure_channel_.get());
  }

  void VerifyDeviceRegistered(cryptauth::RemoteDeviceRef remote_device) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    EXPECT_TRUE(connection_metadata);
  }

  void VerifyDeviceNotRegistered(cryptauth::RemoteDeviceRef remote_device) {
    BleConnectionManager::ConnectionMetadata* connection_metadata =
        manager_->GetConnectionMetadata(remote_device.GetDeviceId());
    if (connection_metadata)
      EXPECT_FALSE(connection_metadata->HasReasonForConnection());
  }

  // Registers |remote_device|, creates a connection to that device at
  // |bluetooth_address|, and authenticates the resulting channel.
  FakeSecureChannel* ConnectSuccessfully(
      cryptauth::RemoteDeviceRef remote_device,
      const std::string& bluetooth_address,
      const ConnectionReason connection_reason,
      bool is_background_advertisement) {
    manager_->RegisterRemoteDevice(remote_device.GetDeviceId(),
                                   connection_reason);
    VerifyAdvertisingTimeoutSet(remote_device);
    VerifyConnectionStateChanges(
        std::vector<SecureChannelStatusChange>{
            {remote_device.GetDeviceId(),
             cryptauth::SecureChannel::Status::DISCONNECTED,
             cryptauth::SecureChannel::Status::CONNECTING,
             BleConnectionManager::StateChangeDetail::
                 STATE_CHANGE_DETAIL_NONE}},
        is_background_advertisement);

    FakeSecureChannel* channel = ReceiveAdvertisementAndConnectChannel(
        remote_device, bluetooth_address, is_background_advertisement);
    AuthenticateChannel(remote_device, is_background_advertisement);
    return channel;
  }

  // Creates a connection to |remote_device| at |bluetooth_address|. The device
  // must be registered before calling this function.
  FakeSecureChannel* ReceiveAdvertisementAndConnectChannel(
      cryptauth::RemoteDeviceRef remote_device,
      const std::string& bluetooth_address,
      bool is_background_advertisement) {
    VerifyDeviceRegistered(remote_device);

    fake_secure_channel_factory_->SetExpectedDeviceAddress(bluetooth_address);

    NotifyReceivedAdvertisementFromDevice(bluetooth_address, remote_device,
                                          is_background_advertisement);

    return GetChannelForDevice(remote_device);
  }

  // Authenticates the SecureChannel associated with |remote_device|. The device
  // must be registered and already have an associated channel before calling
  // this function.
  void AuthenticateChannel(cryptauth::RemoteDeviceRef remote_device,
                           bool is_background_advertisement) {
    VerifyDeviceRegistered(remote_device);

    FakeSecureChannel* channel = GetChannelForDevice(remote_device);
    DCHECK(channel);

    num_expected_authenticated_channels_++;

    channel->ChangeStatus(cryptauth::SecureChannel::Status::CONNECTED);

    channel->ChangeStatus(cryptauth::SecureChannel::Status::AUTHENTICATING);
    channel->ChangeStatus(cryptauth::SecureChannel::Status::AUTHENTICATED);

    VerifyConnectionStateChanges(
        std::vector<SecureChannelStatusChange>{
            {remote_device.GetDeviceId(),
             cryptauth::SecureChannel::Status::CONNECTING,
             cryptauth::SecureChannel::Status::CONNECTED,
             BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE},
            {remote_device.GetDeviceId(),
             cryptauth::SecureChannel::Status::CONNECTED,
             cryptauth::SecureChannel::Status::AUTHENTICATING,
             BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE},
            {remote_device.GetDeviceId(),
             cryptauth::SecureChannel::Status::AUTHENTICATING,
             cryptauth::SecureChannel::Status::AUTHENTICATED,
             BleConnectionManager::StateChangeDetail::
                 STATE_CHANGE_DETAIL_NONE}},
        is_background_advertisement);
  }

  void VerifyLastMessageSent(FakeSecureChannel* channel,
                             int sequence_number,
                             const std::string& payload,
                             size_t expected_size) {
    ASSERT_EQ(expected_size, channel->sent_messages().size());
    cryptauth::FakeSecureChannel::SentMessage sent_message =
        channel->sent_messages()[expected_size - 1];
    EXPECT_EQ(kTetherFeature, sent_message.feature);
    EXPECT_EQ(payload, sent_message.payload);

    std::vector<int> sent_sequence_numbers_before_finished =
        test_observer_->sent_sequence_numbers();
    channel->CompleteSendingMessage(sequence_number);
    std::vector<int> sent_sequence_numbers_after_finished =
        test_observer_->sent_sequence_numbers();
    EXPECT_EQ(sent_sequence_numbers_before_finished.size() + 1u,
              sent_sequence_numbers_after_finished.size());
    EXPECT_EQ(sequence_number, sent_sequence_numbers_after_finished.back());
  }

  const cryptauth::RemoteDeviceRefList test_devices_;

  std::unique_ptr<cryptauth::FakeCryptAuthService> fake_cryptauth_service_;
  scoped_refptr<NiceMock<device::MockBluetoothAdapter>> mock_adapter_;
  std::unique_ptr<FakeBleAdvertiser> fake_ble_advertiser_;
  std::unique_ptr<FakeBleScanner> fake_ble_scanner_;
  std::unique_ptr<FakeAdHocBleAdvertiser> fake_ad_hoc_ble_advertiser_;
  std::unique_ptr<BleAdvertisementDeviceQueue> device_queue_;
  MockTimerFactory* mock_timer_factory_;
  std::unique_ptr<FakeConnectionFactory> fake_connection_factory_;
  std::unique_ptr<FakeSecureChannelFactory> fake_secure_channel_factory_;
  std::unique_ptr<TestObserver> test_observer_;
  std::unique_ptr<TestMetricsObserver> test_metrics_observer_;

  std::vector<SecureChannelStatusChange> verified_status_changes_;
  std::vector<ReceivedMessage> verified_received_messages_;
  bool used_background_advertisements_ = false;

  std::unique_ptr<BleConnectionManager> manager_;

  int num_expected_authenticated_channels_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BleConnectionManagerTest);
};

TEST_F(BleConnectionManagerTest, TestCannotScan) {
  fake_ble_scanner_->set_should_fail_to_register(true);

  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TestCannotAdvertise) {
  fake_ble_advertiser_->set_should_fail_to_start_advertising(true);

  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TestRegistersButNoResult) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TestRegistersAndUnregister_NoConnection) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, TestAdHocBleAdvertiser) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Simulate the channel failing to find GATT services and disconnecting.
  FakeSecureChannel* channel = ReceiveAdvertisementAndConnectChannel(
      test_devices_[0], kBluetoothAddress1,
      false /* is_background_advertisement */);
  channel->NotifyGattCharacteristicsNotAvailable();
  channel->Disconnect();
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // A GATT services workaround should have been requested for that device.
  EXPECT_EQ(std::vector<std::string>{test_devices_[0].GetDeviceId()},
            fake_ad_hoc_ble_advertiser_->requested_device_ids());
}

TEST_F(BleConnectionManagerTest, TestRegisterWithNoConnection_TimeoutOccurs) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  FireTimerForDevice(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, TestSuccessfulConnection_FailsAuthentication) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  fake_secure_channel_factory_->SetExpectedDeviceAddress(kBluetoothAddress1);
  NotifyReceivedAdvertisementFromDevice(
      kBluetoothAddress1, test_devices_[0],
      false /* is_background_advertisement */);
  FakeSecureChannel* channel = GetChannelForDevice(test_devices_[0]);

  // Should not result in an additional "disconnected => connecting" broadcast.
  channel->ChangeStatus(cryptauth::SecureChannel::Status::CONNECTING);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>());

  channel->ChangeStatus(cryptauth::SecureChannel::Status::CONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::CONNECTED,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  channel->ChangeStatus(cryptauth::SecureChannel::Status::AUTHENTICATING);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTED,
       cryptauth::SecureChannel::Status::AUTHENTICATING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Fail authentication, which should automatically start a retry.
  channel->ChangeStatus(cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TestSuccessfulConnection_SendAndReceive) {
  FakeSecureChannel* channel =
      ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                          ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                          false /* is_background_advertisement */);

  int sequence_number =
      manager_->SendMessage(test_devices_[0].GetDeviceId(), "request1");
  VerifyLastMessageSent(channel, sequence_number, "request1", 1);

  channel->ReceiveMessage(kTetherFeature, "response1");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response1"}});

  sequence_number =
      manager_->SendMessage(test_devices_[0].GetDeviceId(), "request2");
  VerifyLastMessageSent(channel, sequence_number, "request2", 2);

  channel->ReceiveMessage(kTetherFeature, "response2");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response2"}});

  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  VerifyDeviceNotRegistered(test_devices_[0]);

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest,
       TestSuccessfulConnection_BackgroundAdvertisement) {
  ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                      ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                      true /* is_background_advertisement */);
  used_background_advertisements_ = true;
}

// Test for fix to crbug.com/706640. This test will crash without the fix.
TEST_F(BleConnectionManagerTest,
       TestSuccessfulConnection_MultipleAdvertisementsReceived) {
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  fake_secure_channel_factory_->SetExpectedDeviceAddress(kBluetoothAddress1);

  // Simulate multiple advertisements being received:
  NotifyReceivedAdvertisementFromDevice(
      kBluetoothAddress1, test_devices_[0],
      false /* is_background_advertisement */);
  FakeSecureChannel* channel = GetChannelForDevice(test_devices_[0]);

  NotifyReceivedAdvertisementFromDevice(
      kBluetoothAddress1, test_devices_[0],
      false /* is_background_advertisement */);
  // Verify that a new channel has not been created:
  EXPECT_EQ(channel, GetChannelForDevice(test_devices_[0]));

  NotifyReceivedAdvertisementFromDevice(
      kBluetoothAddress1, test_devices_[0],
      false /* is_background_advertisement */);
  // Verify that a new channel has not been created:
  EXPECT_EQ(channel, GetChannelForDevice(test_devices_[0]));
}

TEST_F(BleConnectionManagerTest,
       TestSuccessfulConnection_MultipleConnectionReasons) {
  ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                      ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                      false /* is_background_advertisement */);

  // Now, register a different connection reason.
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::CONNECT_TETHERING_REQUEST);

  // Unregister the |TETHER_AVAILABILITY_REQUEST| reason, but leave the
  // |CONNECT_TETHERING_REQUEST| registered.
  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyDeviceRegistered(test_devices_[0]);

  // Now, unregister the other reason; this should cause the device to be
  // fully unregistered.
  manager_->UnregisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                   ConnectionReason::CONNECT_TETHERING_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  VerifyDeviceNotRegistered(test_devices_[0]);

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, TestGetStatusForDevice) {
  cryptauth::SecureChannel::Status status;

  // Should return false when the device has not yet been registered at all.
  EXPECT_FALSE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));

  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Should be CONNECTING at this point.
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::CONNECTING, status);

  fake_secure_channel_factory_->SetExpectedDeviceAddress(kBluetoothAddress1);
  NotifyReceivedAdvertisementFromDevice(
      kBluetoothAddress1, test_devices_[0],
      false /* is_background_advertisement */);
  FakeSecureChannel* channel = GetChannelForDevice(test_devices_[0]);

  channel->ChangeStatus(cryptauth::SecureChannel::Status::CONNECTING);
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::CONNECTING, status);

  channel->ChangeStatus(cryptauth::SecureChannel::Status::CONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::CONNECTED,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::CONNECTED, status);

  channel->ChangeStatus(cryptauth::SecureChannel::Status::AUTHENTICATING);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTED,
       cryptauth::SecureChannel::Status::AUTHENTICATING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::AUTHENTICATING, status);

  channel->ChangeStatus(cryptauth::SecureChannel::Status::AUTHENTICATED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATING,
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::AUTHENTICATED, status);

  // Now, unregister the device.
  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  EXPECT_TRUE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
  EXPECT_EQ(cryptauth::SecureChannel::Status::DISCONNECTING, status);

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
  EXPECT_FALSE(
      manager_->GetStatusForDevice(test_devices_[0].GetDeviceId(), &status));
}

TEST_F(BleConnectionManagerTest,
       TestSuccessfulConnection_DisconnectsAfterConnection) {
  FakeSecureChannel* channel =
      ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                          ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                          false /* is_background_advertisement */);

  channel->ChangeStatus(cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TwoDevices_NeitherCanScan) {
  fake_ble_scanner_->set_should_fail_to_register(true);
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  manager_->RegisterRemoteDevice(test_devices_[1].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest, TwoDevices_NeitherCanAdvertise) {
  fake_ble_advertiser_->set_should_fail_to_start_advertising(true);

  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  manager_->RegisterRemoteDevice(test_devices_[1].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyFailImmediatelyTimeoutSet(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
}

TEST_F(BleConnectionManagerTest,
       TwoDevices_RegisterWithNoConnection_TimerFires) {
  // Register device 0.
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Register device 1.
  manager_->RegisterRemoteDevice(test_devices_[1].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Simulate timeout for device 0 by firing timeout.
  FireTimerForDevice(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Simulate timeout for device 1 by firing timeout.
  FireTimerForDevice(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Unregister device 0.
  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  // Unregister device 1.
  manager_->UnregisterRemoteDevice(
      test_devices_[1].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, TwoDevices_OneConnects) {
  // Successfully connect to device 0.
  ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                      ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                      false /* is_background_advertisement */);

  // Register device 1.
  manager_->RegisterRemoteDevice(test_devices_[1].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyAdvertisingTimeoutSet(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Simulate timeout for device 1 by firing timeout.
  FireTimerForDevice(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Unregister device 0.
  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  // Unregister device 1.
  manager_->UnregisterRemoteDevice(
      test_devices_[1].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, TwoDevices_BothConnectSendAndReceive) {
  FakeSecureChannel* channel0 =
      ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                          ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                          false /* is_background_advertisement */);

  FakeSecureChannel* channel1 =
      ConnectSuccessfully(test_devices_[1], kBluetoothAddress2,
                          ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                          false /* is_background_advertisement */);

  int sequence_number =
      manager_->SendMessage(test_devices_[0].GetDeviceId(), "request1_device0");
  VerifyLastMessageSent(channel0, sequence_number, "request1_device0", 1);

  sequence_number =
      manager_->SendMessage(test_devices_[1].GetDeviceId(), "request1_device1");
  VerifyLastMessageSent(channel1, sequence_number, "request1_device1", 1);

  channel0->ReceiveMessage(kTetherFeature, "response1_device0");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response1_device0"}});

  channel1->ReceiveMessage(kTetherFeature, "response1_device1");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[1].GetDeviceId(), "response1_device1"}});

  sequence_number =
      manager_->SendMessage(test_devices_[0].GetDeviceId(), "request2_device0");
  VerifyLastMessageSent(channel0, sequence_number, "request2_device0", 2);

  sequence_number =
      manager_->SendMessage(test_devices_[1].GetDeviceId(), "request2_device1");
  VerifyLastMessageSent(channel1, sequence_number, "request2_device1", 2);

  channel0->ReceiveMessage(kTetherFeature, "response2_device0");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response2_device0"}});

  channel1->ReceiveMessage(kTetherFeature, "response2_device1");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[1].GetDeviceId(), "response2_device1"}});

  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  VerifyDeviceNotRegistered(test_devices_[0]);

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  manager_->UnregisterRemoteDevice(
      test_devices_[1].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  VerifyDeviceNotRegistered(test_devices_[1]);

  fake_secure_channel_factory_->created_channels()[1]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

TEST_F(BleConnectionManagerTest, FourDevices_ComprehensiveTest) {
  // Register all devices. Since the maximum number of simultaneous connection
  // attempts is 2, only devices 0 and 1 should actually start connecting.
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  manager_->RegisterRemoteDevice(test_devices_[1].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  manager_->RegisterRemoteDevice(test_devices_[2].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  manager_->RegisterRemoteDevice(test_devices_[3].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);

  // Devices 0 and 1 should be advertising; devices 2 and 3 should not be.
  VerifyAdvertisingTimeoutSet(test_devices_[0]);
  VerifyAdvertisingTimeoutSet(test_devices_[1]);
  VerifyNoTimeoutSet(test_devices_[2]);
  VerifyNoTimeoutSet(test_devices_[3]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE},
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Device 0 connects successfully.
  FakeSecureChannel* channel0 = ReceiveAdvertisementAndConnectChannel(
      test_devices_[0], kBluetoothAddress1,
      false /* is_background_advertisement */);

  // Since device 0 has connected, advertising to that device is no longer
  // necessary. Device 2 should have filled up that advertising slot.
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[2].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Meanwhile, device 1 fails to connect, so the timeout fires. The advertising
  // slot left by device 1 creates space for device 3 to start connecting.
  FireTimerForDevice(test_devices_[1]);
  VerifyAdvertisingTimeoutSet(test_devices_[3]);
  VerifyNoTimeoutSet(test_devices_[1]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[3].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Now, device 0 authenticates and sends and receives a message.
  AuthenticateChannel(test_devices_[0],
                      false /* is_background_advertisement */);
  int sequence_number =
      manager_->SendMessage(test_devices_[0].GetDeviceId(), "request1");
  VerifyLastMessageSent(channel0, sequence_number, "request1", 1);

  channel0->ReceiveMessage(kTetherFeature, "response1");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response1"}});

  // Now, device 0 is unregistered.
  manager_->UnregisterRemoteDevice(
      test_devices_[0].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyDeviceNotRegistered(test_devices_[0]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  // Device 2 fails to connect, so the timeout fires. Device 1 takes its spot.
  FireTimerForDevice(test_devices_[2]);
  VerifyAdvertisingTimeoutSet(test_devices_[1]);
  VerifyNoTimeoutSet(test_devices_[2]);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[2].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION},
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Device 3 connects successfully.
  FakeSecureChannel* channel3 = ReceiveAdvertisementAndConnectChannel(
      test_devices_[3], kBluetoothAddress3,
      false /* is_background_advertisement */);

  // Since device 3 has connected, device 2 starts connecting again.
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[2].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});

  // Now, device 3 authenticates and sends and receives a message.
  AuthenticateChannel(test_devices_[3],
                      false /* is_background_advertisement */);
  sequence_number =
      manager_->SendMessage(test_devices_[3].GetDeviceId(), "request3");
  VerifyLastMessageSent(channel3, sequence_number, "request3", 1);

  channel3->ReceiveMessage(kTetherFeature, "response3");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[3].GetDeviceId(), "response3"}});

  // Assume that none of the other devices can connect, and unregister the
  // remaining 3 devices.
  manager_->UnregisterRemoteDevice(
      test_devices_[3].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyDeviceNotRegistered(test_devices_[3]);
  manager_->UnregisterRemoteDevice(
      test_devices_[1].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyDeviceNotRegistered(test_devices_[1]);
  manager_->UnregisterRemoteDevice(
      test_devices_[2].GetDeviceId(),
      ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  VerifyDeviceNotRegistered(test_devices_[2]);

  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[3].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE},
      {test_devices_[1].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED},
      {test_devices_[2].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  fake_secure_channel_factory_->created_channels()[1]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[3].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
}

// Regression test for crbug.com/733360. This bug caused a crash when there were
// multiple observers of BleConnectionManager. The bug was that when an event
// occurred which triggered observers to be notified (i.e., a status changed or
// message received event), sometimes one of the observers would unregister the
// device, which, in turn, caused the associated ConnectionMetadata to be
// deleted. Then, the next observer would be notified of the event after the
// deletion. Since the parameters to the observer callbacks were passed by
// reference (and the reference was held by the deleted ConnectionMetadata),
// this led to the second observer referencing deleted memory. The fix is to
// pass the parameters from the ConnectionMetadata to BleConnectionManager by
// value instead.
TEST_F(BleConnectionManagerTest, ObserverUnregisters) {
  FakeSecureChannel* channel =
      ConnectSuccessfully(test_devices_[0], kBluetoothAddress1,
                          ConnectionReason::TETHER_AVAILABILITY_REQUEST,
                          false /* is_background_advertisement */);

  // Register two separate UnregisteringObservers. When a message is received,
  // the first observer will unregister the device.
  UnregisteringObserver first(manager_.get(),
                              ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  UnregisteringObserver second(manager_.get(),
                               ConnectionReason::TETHER_AVAILABILITY_REQUEST);
  manager_->AddObserver(&first);
  manager_->AddObserver(&second);

  // Receive a message over the channel. This should invoke the observer
  // callbacks. This would have caused a crash before the fix for
  // crbug.com/733360.
  channel->ReceiveMessage(kTetherFeature, "response1");
  VerifyReceivedMessages(std::vector<ReceivedMessage>{
      {test_devices_[0].GetDeviceId(), "response1"}});
  // We expect the device to be unregistered (by the observer).
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::AUTHENTICATED,
       cryptauth::SecureChannel::Status::DISCONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE}});
  VerifyDeviceNotRegistered(test_devices_[0]);

  fake_secure_channel_factory_->created_channels()[0]->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});

  // Now, register the device again. This should cause a "disconnected =>
  // connecting" status change. This time, the multiple observers will respond
  // to a status change event instead of a message received event. This also
  // would have caused a crash before the fix for crbug.com/733360.
  manager_->RegisterRemoteDevice(test_devices_[0].GetDeviceId(),
                                 ConnectionReason::TETHER_AVAILABILITY_REQUEST);

  // We expect the device to be unregistered (by the observer).
  VerifyConnectionStateChanges(std::vector<SecureChannelStatusChange>{
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::DISCONNECTED,
       cryptauth::SecureChannel::Status::CONNECTING,
       BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE},
      {test_devices_[0].GetDeviceId(),
       cryptauth::SecureChannel::Status::CONNECTING,
       cryptauth::SecureChannel::Status::DISCONNECTED,
       BleConnectionManager::StateChangeDetail::
           STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED}});
  VerifyDeviceNotRegistered(test_devices_[0]);
}

}  // namespace tether

}  // namespace chromeos
