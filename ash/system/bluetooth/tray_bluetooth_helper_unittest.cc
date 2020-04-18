// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/tray_bluetooth_helper.h"

#include <vector>

#include "ash/test/ash_test_base.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"
#include "device/bluetooth/dbus/fake_bluetooth_device_client.h"

using bluez::BluezDBusManager;
using bluez::FakeBluetoothAdapterClient;
using bluez::FakeBluetoothDeviceClient;

namespace ash {
namespace {

// Returns true if device with |address| exists in the filtered device list.
// Returns false otherwise.
bool ExistInFilteredDevices(const std::string& address,
                            BluetoothDeviceList filtered_devices) {
  for (const auto& device : filtered_devices) {
    if (device.address == address)
      return true;
  }
  return false;
}

using TrayBluetoothHelperTest = AshTestBase;

// Tests basic functionality.
TEST_F(TrayBluetoothHelperTest, Basics) {
  // Set Bluetooth discovery simulation delay to 0 so the test doesn't have to
  // wait or use timers.
  FakeBluetoothAdapterClient* adapter_client =
      static_cast<FakeBluetoothAdapterClient*>(
          BluezDBusManager::Get()->GetBluetoothAdapterClient());
  adapter_client->SetSimulationIntervalMs(0);

  FakeBluetoothDeviceClient* device_client =
      static_cast<FakeBluetoothDeviceClient*>(
          BluezDBusManager::Get()->GetBluetoothDeviceClient());
  // A classic bluetooth keyboard device shouldn't be filtered out.
  device_client->CreateDevice(
      dbus::ObjectPath(FakeBluetoothAdapterClient::kAdapterPath),
      dbus::ObjectPath(FakeBluetoothDeviceClient::kDisplayPinCodePath));
  // A low energy bluetooth heart rate monitor should be filtered out.
  device_client->CreateDevice(
      dbus::ObjectPath(FakeBluetoothAdapterClient::kAdapterPath),
      dbus::ObjectPath(FakeBluetoothDeviceClient::kLowEnergyPath));

  TrayBluetoothHelper helper;
  helper.Initialize();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothAvailable());
  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(helper.HasBluetoothDiscoverySession());

  BluetoothDeviceList devices = helper.GetAvailableBluetoothDevices();
  // The devices are fake in tests, so don't assume any particular number.
  EXPECT_FALSE(devices.empty());
  EXPECT_TRUE(ExistInFilteredDevices(
      FakeBluetoothDeviceClient::kDisplayPinCodeAddress, devices));
  EXPECT_FALSE(ExistInFilteredDevices(
      FakeBluetoothDeviceClient::kLowEnergyAddress, devices));

  helper.StartBluetoothDiscovering();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.HasBluetoothDiscoverySession());

  helper.StopBluetoothDiscovering();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(helper.HasBluetoothDiscoverySession());
}

}  // namespace
}  // namespace ash
