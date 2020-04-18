// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "device/fido/u2f_request.h"

#include <list>
#include <string>
#include <utility>

#include "base/test/scoped_task_environment.h"
#include "device/fido/fake_fido_discovery.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/mock_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace device {

namespace {

class FakeU2fRequest : public U2fRequest {
 public:
  explicit FakeU2fRequest(
      const base::flat_set<FidoTransportProtocol>& transports)
      : U2fRequest(nullptr /* connector */,
                   transports,
                   std::vector<uint8_t>(),
                   std::vector<uint8_t>(),
                   std::vector<std::vector<uint8_t>>()) {}
  ~FakeU2fRequest() override = default;

  void TryDevice() override {
    // Do nothing.
  }
};

using TestVersionCallback =
    ::device::test::ValueCallbackReceiver<ProtocolVersion>;

}  // namespace

class U2fRequestTest : public ::testing::Test {
 protected:
  base::test::ScopedTaskEnvironment& scoped_task_environment() {
    return scoped_task_environment_;
  }

  test::ScopedFakeFidoDiscoveryFactory& discovery_factory() {
    return discovery_factory_;
  }

  TestVersionCallback& version_callback_receiver() {
    return version_callback_receiver_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME};
  TestVersionCallback version_callback_receiver_;
  test::ScopedFakeFidoDiscoveryFactory discovery_factory_;
};

TEST_F(U2fRequestTest, TestIterateDevice) {
  auto* discovery = discovery_factory().ForgeNextHidDiscovery();

  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice});
  request.Start();

  auto device0 = std::make_unique<MockFidoDevice>();
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(::testing::Return("device0"));
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));

  // Add two U2F devices
  discovery->AddDevice(std::move(device0));
  discovery->AddDevice(std::move(device1));

  // Move first device to current.
  request.IterateDevice();
  ASSERT_NE(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());

  // Move second device to current, first to attempted.
  request.IterateDevice();
  ASSERT_NE(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(1), request.attempted_devices_.size());

  // Move second device from current to attempted, move attempted to devices as
  // all devices have been attempted.
  request.IterateDevice();

  ASSERT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(2), request.devices_.size());
  EXPECT_EQ(static_cast<size_t>(0), request.attempted_devices_.size());

  // Moving attempted devices results in a delayed retry, after which the first
  // device will be tried again. Check for the expected behavior here.
  auto* mock_device = static_cast<MockFidoDevice*>(request.devices_.front());
  EXPECT_CALL(*mock_device, TryWinkRef(_));
  scoped_task_environment().FastForwardUntilNoTasksRemain();

  EXPECT_EQ(mock_device, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());
  EXPECT_EQ(static_cast<size_t>(0), request.attempted_devices_.size());
}

TEST_F(U2fRequestTest, TestAbandonCurrentDeviceAndTransition) {
  auto* discovery = discovery_factory().ForgeNextHidDiscovery();

  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice});
  request.Start();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));

  discovery->AddDevice(std::move(device));

  // Move device to current.
  request.IterateDevice();
  EXPECT_NE(nullptr, request.current_device_);

  // Abandon device.
  request.AbandonCurrentDeviceAndTransition();
  EXPECT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(1u, request.abandoned_devices_.size());

  // Iterating through the device list should not change the state.
  request.IterateDevice();
  EXPECT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(1u, request.abandoned_devices_.size());

  // Removing the device from the discovery should clear it from the list.
  discovery->RemoveDevice("device");
  EXPECT_TRUE(request.abandoned_devices_.empty());
}

TEST_F(U2fRequestTest, TestBasicMachine) {
  auto* discovery = discovery_factory().ForgeNextHidDiscovery();
  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice});
  request.Start();

  ASSERT_NO_FATAL_FAILURE(discovery->WaitForCallToStartAndSimulateSuccess());

  // Add one U2F device
  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId());
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery->AddDevice(std::move(device));

  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);
}

TEST_F(U2fRequestTest, TestAlreadyPresentDevice) {
  auto* discovery = discovery_factory().ForgeNextHidDiscovery();

  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice});
  request.Start();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  discovery->AddDevice(std::move(device));

  ASSERT_NO_FATAL_FAILURE(discovery->WaitForCallToStartAndSimulateSuccess());
  EXPECT_NE(nullptr, request.current_device_);
}

TEST_F(U2fRequestTest, TestMultipleDiscoveries) {
  auto* discovery_1 = discovery_factory().ForgeNextHidDiscovery();
  auto* discovery_2 = discovery_factory().ForgeNextBleDiscovery();

  // Create a fake request with two different discoveries that both start up
  // successfully.
  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice,
                          FidoTransportProtocol::kBluetoothLowEnergy});
  request.Start();

  ASSERT_NO_FATAL_FAILURE(discovery_1->WaitForCallToStartAndSimulateSuccess());
  ASSERT_NO_FATAL_FAILURE(discovery_2->WaitForCallToStartAndSimulateSuccess());

  // Let each discovery find a device.
  auto device_1 = std::make_unique<MockFidoDevice>();
  auto device_2 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device_1, GetId()).WillRepeatedly(::testing::Return("device_1"));
  EXPECT_CALL(*device_2, GetId()).WillRepeatedly(::testing::Return("device_2"));

  auto* device_1_ptr = device_1.get();
  auto* device_2_ptr = device_2.get();
  discovery_1->AddDevice(std::move(device_1));
  discovery_2->AddDevice(std::move(device_2));

  // Iterate through the devices and make sure they are considered in the same
  // order as they were added.
  EXPECT_EQ(device_1_ptr, request.current_device_);
  request.IterateDevice();

  EXPECT_EQ(device_2_ptr, request.current_device_);
  request.IterateDevice();

  EXPECT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(2u, request.devices_.size());

  // Add a third device.
  auto device_3 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device_3, GetId()).WillRepeatedly(::testing::Return("device_3"));
  auto* device_3_ptr = device_3.get();
  discovery_1->AddDevice(std::move(device_3));

  // Exhaust the timeout and remove the first two devices, making sure the just
  // added one is the only device considered.
  scoped_task_environment().FastForwardUntilNoTasksRemain();
  discovery_2->RemoveDevice("device_2");
  discovery_1->RemoveDevice("device_1");
  EXPECT_EQ(device_3_ptr, request.current_device_);

  // Finally remove the last remaining device.
  discovery_1->RemoveDevice("device_3");
  EXPECT_EQ(nullptr, request.current_device_);
}

TEST_F(U2fRequestTest, TestSlowDiscovery) {
  auto* fast_discovery = discovery_factory().ForgeNextHidDiscovery();
  auto* slow_discovery = discovery_factory().ForgeNextBleDiscovery();

  // Create a fake request with two different discoveries that start at
  // different times.
  FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice,
                          FidoTransportProtocol::kBluetoothLowEnergy});

  auto fast_device = std::make_unique<MockFidoDevice>();
  auto slow_device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*fast_device, GetId())
      .WillRepeatedly(::testing::Return("fast_device"));
  EXPECT_CALL(*slow_device, GetId())
      .WillRepeatedly(::testing::Return("slow_device"));

  bool fast_winked = false;
  EXPECT_CALL(*fast_device, TryWinkRef(_))
      .WillOnce(
          ::testing::DoAll(::testing::Assign(&fast_winked, true),
                           ::testing::Invoke(MockFidoDevice::WinkDoNothing)))
      .WillRepeatedly(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  bool slow_winked = false;
  EXPECT_CALL(*slow_device, TryWinkRef(_))
      .WillOnce(testing::DoAll(testing::Assign(&slow_winked, true),
                               testing::Invoke(MockFidoDevice::WinkDoNothing)));

  auto* fast_device_ptr = fast_device.get();
  auto* slow_device_ptr = slow_device.get();

  EXPECT_EQ(nullptr, request.current_device_);
  request.state_ = U2fRequest::State::INIT;

  // The discoveries will be started and |fast_discovery| will succeed
  // immediately with a device already found.
  request.Start();

  EXPECT_FALSE(fast_winked);

  ASSERT_NO_FATAL_FAILURE(fast_discovery->WaitForCallToStart());
  fast_discovery->AddDevice(std::move(fast_device));
  ASSERT_NO_FATAL_FAILURE(fast_discovery->SimulateStarted(true /* success */));

  EXPECT_TRUE(fast_winked);
  EXPECT_EQ(fast_device_ptr, request.current_device_);
  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);

  // There are no more devices at this time.
  request.state_ = U2fRequest::State::IDLE;
  request.Transition();
  EXPECT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(U2fRequest::State::OFF, request.state_);

  // All devices have been tried and have been re-enqueued to try again in the
  // future. Now |slow_discovery| starts:
  ASSERT_TRUE(slow_discovery->is_start_requested());
  ASSERT_FALSE(slow_discovery->is_running());
  ASSERT_NO_FATAL_FAILURE(slow_discovery->WaitForCallToStart());
  slow_discovery->AddDevice(std::move(slow_device));
  ASSERT_NO_FATAL_FAILURE(slow_discovery->SimulateStarted(true /* success */));

  // |fast_device| is already enqueued and will be retried immediately.
  EXPECT_EQ(fast_device_ptr, request.current_device_);
  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);

  // Next the newly found |slow_device| will be tried.
  request.state_ = U2fRequest::State::IDLE;
  EXPECT_FALSE(slow_winked);
  request.Transition();
  EXPECT_TRUE(slow_winked);
  EXPECT_EQ(slow_device_ptr, request.current_device_);
  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);

  // All discoveries are complete so the request transitions to |OFF|.
  request.state_ = U2fRequest::State::IDLE;
  request.Transition();
  EXPECT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(U2fRequest::State::OFF, request.state_);
}

TEST_F(U2fRequestTest, TestMultipleDiscoveriesWithFailures) {
  // Create a fake request with two different discoveries that both start up
  // unsuccessfully.
  {
    auto* discovery_1 = discovery_factory().ForgeNextHidDiscovery();
    auto* discovery_2 = discovery_factory().ForgeNextBleDiscovery();

    FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice,
                            FidoTransportProtocol::kBluetoothLowEnergy});
    request.Start();

    ASSERT_NO_FATAL_FAILURE(discovery_1->WaitForCallToStart());
    ASSERT_NO_FATAL_FAILURE(discovery_1->SimulateStarted(false /* success */));
    ASSERT_NO_FATAL_FAILURE(discovery_2->WaitForCallToStart());
    ASSERT_NO_FATAL_FAILURE(discovery_2->SimulateStarted(false /* success */));

    EXPECT_EQ(U2fRequest::State::OFF, request.state_);
  }

  // Create a fake request with two different discoveries, where only one starts
  // up successfully.
  {
    auto* discovery_1 = discovery_factory().ForgeNextHidDiscovery();
    auto* discovery_2 = discovery_factory().ForgeNextBleDiscovery();

    FakeU2fRequest request({FidoTransportProtocol::kUsbHumanInterfaceDevice,
                            FidoTransportProtocol::kBluetoothLowEnergy});
    request.Start();

    ASSERT_NO_FATAL_FAILURE(discovery_1->WaitForCallToStart());
    ASSERT_NO_FATAL_FAILURE(discovery_1->SimulateStarted(false /* success */));
    ASSERT_NO_FATAL_FAILURE(discovery_2->WaitForCallToStart());
    ASSERT_NO_FATAL_FAILURE(discovery_2->SimulateStarted(true /* success */));

    auto device0 = std::make_unique<MockFidoDevice>();
    EXPECT_CALL(*device0, GetId())
        .WillRepeatedly(::testing::Return("device_0"));
    EXPECT_CALL(*device0, TryWinkRef(_))
        .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
    discovery_2->AddDevice(std::move(device0));

    EXPECT_EQ(U2fRequest::State::BUSY, request.state_);

    // Simulate an action that sets the request state to idle.
    // This and the call to Transition() below is necessary to trigger iterating
    // and trying the new device.
    request.state_ = U2fRequest::State::IDLE;

    // Adding another device should trigger examination and a busy state.
    auto device1 = std::make_unique<MockFidoDevice>();
    EXPECT_CALL(*device1, GetId())
        .WillRepeatedly(::testing::Return("device_1"));
    EXPECT_CALL(*device1, TryWinkRef(_))
        .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
    discovery_2->AddDevice(std::move(device1));

    request.Transition();
    EXPECT_EQ(U2fRequest::State::BUSY, request.state_);
  }
}

}  // namespace device
