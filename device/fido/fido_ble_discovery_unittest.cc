// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_discovery.h"

#include <string>

#include "base/bind.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "device/fido/fido_ble_device.h"
#include "device/fido/mock_fido_discovery_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "device/bluetooth/test/bluetooth_test_android.h"
#elif defined(OS_MACOSX)
#include "device/bluetooth/test/bluetooth_test_mac.h"
#elif defined(OS_WIN)
#include "device/bluetooth/test/bluetooth_test_win.h"
#elif defined(OS_CHROMEOS) || defined(OS_LINUX)
#include "device/bluetooth/test/bluetooth_test_bluez.h"
#endif

using ::testing::_;

namespace device {

ACTION_P(ReturnFromAsyncCall, closure) {
  closure.Run();
}

MATCHER_P(IdMatches, id, "") {
  return arg->GetId() == std::string("ble:") + id;
}

TEST_F(BluetoothTest, FidoBleDiscoveryNoAdapter) {
  // We purposefully construct a temporary and provide no fake adapter,
  // simulating cases where the discovery is destroyed before obtaining a handle
  // to an adapter. This should be handled gracefully and not result in a crash.
  FidoBleDiscovery discovery;

  // We don't expect any calls to the notification methods.
  MockFidoDiscoveryObserver observer;
  discovery.set_observer(&observer);
  EXPECT_CALL(observer, DiscoveryStarted(&discovery, _)).Times(0);
  EXPECT_CALL(observer, DeviceAdded(&discovery, _)).Times(0);
  EXPECT_CALL(observer, DeviceRemoved(&discovery, _)).Times(0);
}

TEST_F(BluetoothTest, FidoBleDiscoveryFindsKnownDevice) {
  if (!PlatformSupportsLowEnergy()) {
    LOG(WARNING) << "Low Energy Bluetooth unavailable, skipping unit test.";
    return;
  }
  InitWithFakeAdapter();

  SimulateLowEnergyDevice(4);  // This device should be ignored.
  SimulateLowEnergyDevice(7);

  FidoBleDiscovery discovery;

  MockFidoDiscoveryObserver observer;
  discovery.set_observer(&observer);

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer,
                DeviceAdded(&discovery,
                            IdMatches(BluetoothTestBase::kTestDeviceAddress1)));
    EXPECT_CALL(observer, DiscoveryStarted(&discovery, true))
        .WillOnce(ReturnFromAsyncCall(quit));

    discovery.Start();
    run_loop.Run();
  }
}

TEST_F(BluetoothTest, FidoBleDiscoveryFindsNewDevice) {
  if (!PlatformSupportsLowEnergy()) {
    LOG(WARNING) << "Low Energy Bluetooth unavailable, skipping unit test.";
    return;
  }
  InitWithFakeAdapter();

  FidoBleDiscovery discovery;
  MockFidoDiscoveryObserver observer;
  discovery.set_observer(&observer);

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer, DiscoveryStarted(&discovery, true))
        .WillOnce(ReturnFromAsyncCall(quit));

    discovery.Start();
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer,
                DeviceAdded(&discovery,
                            IdMatches(BluetoothTestBase::kTestDeviceAddress1)))
        .WillOnce(ReturnFromAsyncCall(quit));

    SimulateLowEnergyDevice(4);  // This device should be ignored.
    SimulateLowEnergyDevice(7);

    run_loop.Run();
  }
}

// Simulate the scenario where the BLE device is already known at start-up time,
// but no service advertisements have been received from the device yet, so we
// do not know if it is a CTAP2/U2F device or not. As soon as it is discovered
// that the device supports the FIDO service, the observer should be notified of
// a new FidoBleDevice.
TEST_F(BluetoothTest, FidoBleDiscoveryFindsUpdatedDevice) {
  if (!PlatformSupportsLowEnergy()) {
    LOG(WARNING) << "Low Energy Bluetooth unavailable, skipping unit test.";
    return;
  }
  InitWithFakeAdapter();

  SimulateLowEnergyDevice(3);

  FidoBleDiscovery discovery;
  MockFidoDiscoveryObserver observer;
  discovery.set_observer(&observer);

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer, DiscoveryStarted(&discovery, true))
        .WillOnce(ReturnFromAsyncCall(quit));

    discovery.Start();
    run_loop.Run();

    EXPECT_THAT(discovery.GetDevices(), ::testing::IsEmpty());
  }

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer,
                DeviceAdded(&discovery,
                            IdMatches(BluetoothTestBase::kTestDeviceAddress1)))
        .WillOnce(ReturnFromAsyncCall(quit));

    // This will update properties for device 3.
    SimulateLowEnergyDevice(7);

    run_loop.Run();

    const auto devices = discovery.GetDevices();
    ASSERT_THAT(devices, ::testing::SizeIs(1u));
    EXPECT_EQ(FidoBleDevice::GetId(BluetoothTestBase::kTestDeviceAddress1),
              devices[0]->GetId());
  }
}

}  // namespace device
