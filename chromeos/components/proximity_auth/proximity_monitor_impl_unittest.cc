// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/proximity_monitor_impl.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/proximity_auth_profile_pref_manager.h"
#include "chromeos/components/proximity_auth/proximity_monitor_observer.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "components/cryptauth/software_feature_state.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using device::BluetoothDevice;
using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;

namespace proximity_auth {
namespace {

const char kRemoteDeviceUserId[] = "example@gmail.com";
const char kRemoteDeviceName[] = "LGE Nexus 5";

// The proximity threshold corresponds to a RSSI of -70.
const ProximityAuthPrefManager::ProximityThreshold
    kProximityThresholdPrefValue =
        ProximityAuthPrefManager::ProximityThreshold::kFar;
const int kRssiThreshold = -70;

class MockProximityMonitorObserver : public ProximityMonitorObserver {
 public:
  MockProximityMonitorObserver() {}
  ~MockProximityMonitorObserver() override {}

  MOCK_METHOD0(OnProximityStateChanged, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProximityMonitorObserver);
};

class MockProximityAuthPrefManager : public ProximityAuthProfilePrefManager {
 public:
  MockProximityAuthPrefManager() : ProximityAuthProfilePrefManager(nullptr) {}
  ~MockProximityAuthPrefManager() override {}

  MOCK_CONST_METHOD0(GetProximityThreshold,
                     ProximityAuthPrefManager::ProximityThreshold(void));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProximityAuthPrefManager);
};

// Creates a mock Bluetooth adapter and sets it as the global adapter for
// testing.
scoped_refptr<device::MockBluetoothAdapter>
CreateAndRegisterMockBluetoothAdapter() {
  scoped_refptr<device::MockBluetoothAdapter> adapter =
      new NiceMock<device::MockBluetoothAdapter>();
  device::BluetoothAdapterFactory::SetAdapterForTesting(adapter);
  return adapter;
}

}  // namespace

class ProximityAuthProximityMonitorImplTest : public testing::Test {
 public:
  ProximityAuthProximityMonitorImplTest()
      : bluetooth_adapter_(CreateAndRegisterMockBluetoothAdapter()),
        remote_bluetooth_device_(&*bluetooth_adapter_,
                                 0,
                                 kRemoteDeviceName,
                                 "",
                                 false /* paired */,
                                 true /* connected */),
        remote_device_(cryptauth::RemoteDeviceRefBuilder()
                           .SetUserId(kRemoteDeviceUserId)
                           .SetName(kRemoteDeviceName)
                           .Build()),
        connection_(remote_device_),
        pref_manager_(new NiceMock<MockProximityAuthPrefManager>()),
        monitor_(&connection_, pref_manager_.get()),
        task_runner_(new base::TestSimpleTaskRunner()),
        thread_task_runner_handle_(task_runner_) {
    ON_CALL(*bluetooth_adapter_, GetDevice(std::string()))
        .WillByDefault(Return(&remote_bluetooth_device_));
    ON_CALL(remote_bluetooth_device_, GetConnectionInfo(_))
        .WillByDefault(SaveArg<0>(&connection_info_callback_));
    monitor_.AddObserver(&observer_);
    ON_CALL(*pref_manager_, GetProximityThreshold())
        .WillByDefault(Return(kProximityThresholdPrefValue));
  }

  ~ProximityAuthProximityMonitorImplTest() override {}

  void RunPendingTasks() { task_runner_->RunPendingTasks(); }

  void ProvideConnectionInfo(
      const BluetoothDevice::ConnectionInfo& connection_info) {
    RunPendingTasks();
    connection_info_callback_.Run(connection_info);

    // Reset the callback to ensure that tests correctly only respond at most
    // once per call to GetConnectionInfo().
    connection_info_callback_ = BluetoothDevice::ConnectionInfoCallback();
  }

 protected:
  // Mock for verifying interactions with the proximity monitor's observer.
  NiceMock<MockProximityMonitorObserver> observer_;

  // Mocks used for verifying interactions with the Bluetooth subsystem.
  scoped_refptr<device::MockBluetoothAdapter> bluetooth_adapter_;
  NiceMock<device::MockBluetoothDevice> remote_bluetooth_device_;
  cryptauth::RemoteDeviceRef remote_device_;
  cryptauth::FakeConnection connection_;

  // ProximityAuthPrefManager mock.
  std::unique_ptr<NiceMock<MockProximityAuthPrefManager>> pref_manager_;

  // The proximity monitor under test.
  ProximityMonitorImpl monitor_;

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle thread_task_runner_handle_;
  BluetoothDevice::ConnectionInfoCallback connection_info_callback_;
  ScopedDisableLoggingForTesting disable_logging_;
};

TEST_F(ProximityAuthProximityMonitorImplTest, IsUnlockAllowed_NeverStarted) {
  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_Started_NoConnectionInfoReceivedYet) {
  monitor_.Start();
  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest, IsUnlockAllowed_RssiInRange) {
  monitor_.Start();
  ProvideConnectionInfo({0, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest, IsUnlockAllowed_UnknownRssi) {
  monitor_.Start();

  ProvideConnectionInfo({0, 0, 4});
  ProvideConnectionInfo({BluetoothDevice::kUnknownPower, 0, 4});

  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_InformsObserverOfChanges) {
  // Initially, the device is not in proximity.
  monitor_.Start();
  EXPECT_FALSE(monitor_.IsUnlockAllowed());

  // Simulate receiving an RSSI reading in proximity.
  EXPECT_CALL(observer_, OnProximityStateChanged()).Times(1);
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  // Simulate a reading indicating non-proximity.
  EXPECT_CALL(observer_, OnProximityStateChanged()).Times(1);
  ProvideConnectionInfo({2 * kRssiThreshold, 4, 4});
  ProvideConnectionInfo({2 * kRssiThreshold, 4, 4});
  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest, IsUnlockAllowed_StartThenStop) {
  monitor_.Start();

  ProvideConnectionInfo({0, 0, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  monitor_.Stop();
  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_StartThenStopThenStartAgain) {
  monitor_.Start();
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());
  monitor_.Stop();

  // Restarting the monitor should immediately reset the proximity state, rather
  // than building on the previous rolling average.
  monitor_.Start();
  ProvideConnectionInfo({kRssiThreshold - 1, 4, 4});

  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_RemoteDeviceRemainsInProximity) {
  monitor_.Start();
  ProvideConnectionInfo({kRssiThreshold / 2 + 1, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2 - 1, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2 + 2, 4, 4});
  ProvideConnectionInfo({kRssiThreshold / 2 - 3, 4, 4});

  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  // Brief drops in RSSI should be handled by weighted averaging.
  ProvideConnectionInfo({kRssiThreshold - 5, 4, 4});

  EXPECT_TRUE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_RemoteDeviceLeavesProximity) {
  monitor_.Start();

  // Start with a device in proximity.
  ProvideConnectionInfo({0, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  // Simulate readings for the remote device leaving proximity.
  ProvideConnectionInfo({-1, 4, 4});
  ProvideConnectionInfo({-4, 4, 4});
  ProvideConnectionInfo({0, 4, 4});
  ProvideConnectionInfo({-10, 4, 4});
  ProvideConnectionInfo({-15, 4, 4});
  ProvideConnectionInfo({-20, 4, 4});
  ProvideConnectionInfo({kRssiThreshold, 4, 4});
  ProvideConnectionInfo({kRssiThreshold - 10, 4, 4});
  ProvideConnectionInfo({kRssiThreshold - 20, 4, 4});
  ProvideConnectionInfo({kRssiThreshold - 20, 4, 4});
  ProvideConnectionInfo({kRssiThreshold - 20, 4, 4});
  ProvideConnectionInfo({kRssiThreshold - 20, 4, 4});

  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_RemoteDeviceEntersProximity) {
  monitor_.Start();

  // Start with a device out of proximity.
  ProvideConnectionInfo({2 * kRssiThreshold, 4, 4});
  EXPECT_FALSE(monitor_.IsUnlockAllowed());

  // Simulate readings for the remote device entering proximity.
  ProvideConnectionInfo({-15, 4, 4});
  ProvideConnectionInfo({-8, 4, 4});
  ProvideConnectionInfo({-12, 4, 4});
  ProvideConnectionInfo({-18, 4, 4});
  ProvideConnectionInfo({-7, 4, 4});
  ProvideConnectionInfo({-3, 4, 4});
  ProvideConnectionInfo({-2, 4, 4});
  ProvideConnectionInfo({0, 4, 4});
  ProvideConnectionInfo({0, 4, 4});

  EXPECT_TRUE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_DeviceNotKnownToAdapter) {
  monitor_.Start();

  // Start with the device known to the adapter and in proximity.
  ProvideConnectionInfo({0, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  // Simulate it being forgotten.
  ON_CALL(*bluetooth_adapter_, GetDevice(std::string()))
      .WillByDefault(Return(nullptr));
  EXPECT_CALL(observer_, OnProximityStateChanged());
  RunPendingTasks();

  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_DeviceNotConnected) {
  monitor_.Start();

  // Start with the device connected and in proximity.
  ProvideConnectionInfo({0, 4, 4});
  EXPECT_TRUE(monitor_.IsUnlockAllowed());

  // Simulate it disconnecting.
  ON_CALL(remote_bluetooth_device_, IsConnected()).WillByDefault(Return(false));
  EXPECT_CALL(observer_, OnProximityStateChanged());
  RunPendingTasks();

  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       IsUnlockAllowed_ConnectionInfoReceivedAfterStopping) {
  monitor_.Start();
  monitor_.Stop();
  ProvideConnectionInfo({0, 4, 4});
  EXPECT_FALSE(monitor_.IsUnlockAllowed());
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       RecordProximityMetricsOnAuthSuccess_NormalValues) {
  monitor_.Start();
  ProvideConnectionInfo({0, 0, 4});

  ProvideConnectionInfo({-20, 3, 4});

  base::HistogramTester histogram_tester;
  monitor_.RecordProximityMetricsOnAuthSuccess();
  histogram_tester.ExpectUniqueSample("EasyUnlock.AuthProximity.RollingRssi",
                                      -6, 1);
  histogram_tester.ExpectUniqueSample(
      "EasyUnlock.AuthProximity.RemoteDeviceModelHash",
      1881443083 /* hash of "LGE Nexus 5" */, 1);
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       RecordProximityMetricsOnAuthSuccess_ClampedValues) {
  monitor_.Start();
  ProvideConnectionInfo({-99999, 99999, 12345});

  base::HistogramTester histogram_tester;
  monitor_.RecordProximityMetricsOnAuthSuccess();
  histogram_tester.ExpectUniqueSample("EasyUnlock.AuthProximity.RollingRssi",
                                      -100, 1);
}

TEST_F(ProximityAuthProximityMonitorImplTest,
       RecordProximityMetricsOnAuthSuccess_UnknownValues) {
  // Note: A device without a recorded name will have "Unknown" as its name.
  cryptauth::FakeConnection connection(cryptauth::RemoteDeviceRefBuilder()
                                           .SetUserId(kRemoteDeviceUserId)
                                           .SetName(std::string())
                                           .Build());

  ProximityMonitorImpl monitor(&connection, pref_manager_.get());
  monitor.AddObserver(&observer_);
  monitor.Start();
  ProvideConnectionInfo({127, 127, 127});

  base::HistogramTester histogram_tester;
  monitor.RecordProximityMetricsOnAuthSuccess();
  histogram_tester.ExpectUniqueSample("EasyUnlock.AuthProximity.RollingRssi",
                                      127, 1);
  histogram_tester.ExpectUniqueSample(
      "EasyUnlock.AuthProximity.RemoteDeviceModelHash",
      -1808066424 /* hash of "Unknown" */, 1);
}

}  // namespace proximity_auth
