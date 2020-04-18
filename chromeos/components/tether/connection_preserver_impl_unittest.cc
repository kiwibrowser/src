// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/connection_preserver_impl.h"

#include <memory>

#include "base/base64.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/timer/mock_timer.h"
#include "chromeos/components/tether/connection_reason.h"
#include "chromeos/components/tether/fake_active_host.h"
#include "chromeos/components/tether/fake_ble_connection_manager.h"
#include "chromeos/components/tether/mock_tether_host_response_recorder.h"
#include "chromeos/components/tether/timer_factory.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace chromeos {

namespace tether {

namespace {

const char kWifiNetworkGuid[] = "wifiNetworkGuid";
const char kTetherNetworkGuid[] = "tetherNetworkGuid";
const char kDeviceId1[] = "deviceId1";
const char kDeviceId2[] = "deviceId2";
const char kDeviceId3[] = "deviceId3";

std::string CreateConfigurationJsonString(const std::string& guid,
                                          const std::string& type) {
  std::stringstream ss;
  ss << "{"
     << "  \"GUID\": \"" << guid << "\","
     << "  \"Type\": \"" << type << "\","
     << "  \"State\": \"" << shill::kStateReady << "\""
     << "}";
  return ss.str();
}

}  // namespace

class ConnectionPreserverImplTest : public NetworkStateTest {
 protected:
  ConnectionPreserverImplTest() {}

  void SetUp() override {
    DBusThreadManager::Initialize();
    NetworkStateTest::SetUp();

    fake_ble_connection_manager_ = std::make_unique<FakeBleConnectionManager>();
    fake_active_host_ = std::make_unique<FakeActiveHost>();

    previously_connected_host_ids_.clear();
    mock_tether_host_response_recorder_ =
        std::make_unique<NiceMock<MockTetherHostResponseRecorder>>();
    ON_CALL(*mock_tether_host_response_recorder_,
            GetPreviouslyConnectedHostIds())
        .WillByDefault(Invoke(
            this, &ConnectionPreserverImplTest::GetPreviouslyConnectedHostIds));

    connection_preserver_ = std::make_unique<ConnectionPreserverImpl>(
        fake_ble_connection_manager_.get(), network_state_handler(),
        fake_active_host_.get(), mock_tether_host_response_recorder_.get());

    mock_timer_ = new base::MockTimer(false /* retain_user_task */,
                                      false /* is_repeating */);
    connection_preserver_->SetTimerForTesting(base::WrapUnique(mock_timer_));
  }

  void TearDown() override {
    connection_preserver_.reset();

    ShutdownNetworkState();
    NetworkStateTest::TearDown();
    DBusThreadManager::Shutdown();
  }

  void SimulateSuccessfulHostScan(const std::string& device_id,
                                  bool should_remain_registered) {
    fake_ble_connection_manager_->RegisterRemoteDevice(
        device_id, ConnectionReason::TETHER_AVAILABILITY_REQUEST);
    EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(device_id));

    connection_preserver_->HandleSuccessfulTetherAvailabilityResponse(
        device_id);
    EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(device_id));

    fake_ble_connection_manager_->UnregisterRemoteDevice(
        device_id, ConnectionReason::TETHER_AVAILABILITY_REQUEST);
    EXPECT_EQ(should_remain_registered,
              fake_ble_connection_manager_->IsRegistered(device_id));
  }

  void ConnectToWifi() {
    std::string wifi_service_path = ConfigureService(
        CreateConfigurationJsonString(kWifiNetworkGuid, shill::kTypeWifi));
  }

  std::vector<std::string> GetPreviouslyConnectedHostIds() {
    return previously_connected_host_ids_;
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<FakeBleConnectionManager> fake_ble_connection_manager_;
  std::unique_ptr<FakeActiveHost> fake_active_host_;
  std::unique_ptr<NiceMock<MockTetherHostResponseRecorder>>
      mock_tether_host_response_recorder_;
  base::MockTimer* mock_timer_;

  std::unique_ptr<ConnectionPreserverImpl> connection_preserver_;

  std::vector<std::string> previously_connected_host_ids_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectionPreserverImplTest);
};

TEST_F(ConnectionPreserverImplTest,
       TestHandleSuccessfulTetherAvailabilityResponse_NoPreservedConnection) {
  SimulateSuccessfulHostScan(kDeviceId1, true /* should_remain_registered */);
}

TEST_F(ConnectionPreserverImplTest,
       TestHandleSuccessfulTetherAvailabilityResponse_HasInternet) {
  ConnectToWifi();

  SimulateSuccessfulHostScan(kDeviceId1, false /* should_remain_registered */);
}

TEST_F(
    ConnectionPreserverImplTest,
    TestHandleSuccessfulTetherAvailabilityResponse_PreservedConnectionExists_NoPreviouslyConnectedHosts) {
  SimulateSuccessfulHostScan(kDeviceId1, true /* should_remain_registered */);
  SimulateSuccessfulHostScan(kDeviceId2, true /* should_remain_registered */);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(kDeviceId1));
}

TEST_F(ConnectionPreserverImplTest,
       TestHandleSuccessfulTetherAvailabilityResponse_TimesOut) {
  SimulateSuccessfulHostScan(kDeviceId1, true /* should_remain_registered */);

  mock_timer_->Fire();
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(kDeviceId1));
}

TEST_F(ConnectionPreserverImplTest,
       TestHandleSuccessfulTetherAvailabilityResponse_PreserverDestroyed) {
  SimulateSuccessfulHostScan(kDeviceId1, true /* should_remain_registered */);

  connection_preserver_.reset();
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(kDeviceId1));
}

TEST_F(
    ConnectionPreserverImplTest,
    TestHandleSuccessfulTetherAvailabilityResponse_ActiveHostBecomesConnected) {
  // FakeActiveHost internally expects a Base64 encoded string.
  std::string encoded_device_id;
  base::Base64Encode(kDeviceId1, &encoded_device_id);

  SimulateSuccessfulHostScan(encoded_device_id,
                             true /* should_remain_registered */);

  fake_active_host_->SetActiveHostConnecting(encoded_device_id,
                                             kTetherNetworkGuid);
  fake_active_host_->SetActiveHostConnected(
      encoded_device_id, kTetherNetworkGuid, kWifiNetworkGuid);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(encoded_device_id));
}

TEST_F(
    ConnectionPreserverImplTest,
    TestHandleSuccessfulTetherAvailabilityResponse_PreviouslyConnectedHostsExist) {
  // |kDeviceId1| is the most recently connected device, and should be preferred
  // over any other device.
  previously_connected_host_ids_.push_back(kDeviceId1);
  previously_connected_host_ids_.push_back(kDeviceId2);

  SimulateSuccessfulHostScan(kDeviceId3, true /* should_remain_registered */);

  SimulateSuccessfulHostScan(kDeviceId2, true /* should_remain_registered */);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(kDeviceId3));

  SimulateSuccessfulHostScan(kDeviceId1, true /* should_remain_registered */);
  EXPECT_FALSE(fake_ble_connection_manager_->IsRegistered(kDeviceId2));

  SimulateSuccessfulHostScan(kDeviceId2, false /* should_remain_registered */);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(kDeviceId1));

  SimulateSuccessfulHostScan(kDeviceId3, false /* should_remain_registered */);
  EXPECT_TRUE(fake_ble_connection_manager_->IsRegistered(kDeviceId1));
}

}  // namespace tether

}  // namespace chromeos
