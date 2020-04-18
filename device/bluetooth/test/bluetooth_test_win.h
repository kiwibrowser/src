// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_WIN_H_
#define DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_WIN_H_

#include "device/bluetooth/test/bluetooth_test.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_pending_task.h"
#include "base/test/test_simple_task_runner.h"
#include "base/win/scoped_winrt_initializer.h"
#include "device/bluetooth/bluetooth_classic_win_fake.h"
#include "device/bluetooth/bluetooth_low_energy_win_fake.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"

namespace device {

// Windows implementation of BluetoothTestBase.
class BluetoothTestWin : public BluetoothTestBase,
                         public win::BluetoothLowEnergyWrapperFake::Observer {
 public:
  BluetoothTestWin();
  ~BluetoothTestWin() override;

  // BluetoothTestBase overrides
  bool PlatformSupportsLowEnergy() override;
  void InitWithDefaultAdapter() override;
  void InitWithoutDefaultAdapter() override;
  void InitWithFakeAdapter() override;
  bool DenyPermission() override;
  void StartLowEnergyDiscoverySession() override;
  BluetoothDevice* SimulateLowEnergyDevice(int device_ordinal) override;
  void SimulateGattConnection(BluetoothDevice* device) override;
  void SimulateGattServicesDiscovered(
      BluetoothDevice* device,
      const std::vector<std::string>& uuids) override;
  void SimulateGattServiceRemoved(BluetoothRemoteGattService* service) override;
  void SimulateGattCharacteristic(BluetoothRemoteGattService* service,
                                  const std::string& uuid,
                                  int properties) override;
  void SimulateGattCharacteristicRemoved(
      BluetoothRemoteGattService* service,
      BluetoothRemoteGattCharacteristic* characteristic) override;
  void RememberCharacteristicForSubsequentAction(
      BluetoothRemoteGattCharacteristic* characteristic) override;
  void SimulateGattCharacteristicRead(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;
  void SimulateGattCharacteristicReadError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode error_code) override;
  void SimulateGattCharacteristicWrite(
      BluetoothRemoteGattCharacteristic* characteristic) override;
  void SimulateGattCharacteristicWriteError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode error_code) override;
  void RememberDeviceForSubsequentAction(BluetoothDevice* device) override;
  void DeleteDevice(BluetoothDevice* device) override;
  void SimulateGattDescriptor(BluetoothRemoteGattCharacteristic* characteristic,
                              const std::string& uuid) override;
  void SimulateGattNotifySessionStarted(
      BluetoothRemoteGattCharacteristic* characteristic) override;
  void SimulateGattNotifySessionStartError(
      BluetoothRemoteGattCharacteristic* characteristic,
      BluetoothRemoteGattService::GattErrorCode error_code) override;
  void SimulateGattCharacteristicChanged(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;

  // win::BluetoothLowEnergyWrapperFake::Observer overrides.
  void OnReadGattCharacteristicValue() override;
  void OnWriteGattCharacteristicValue(
      const PBTH_LE_GATT_CHARACTERISTIC_VALUE value) override;
  void OnStartCharacteristicNotification() override;
  void OnWriteGattDescriptorValue(const std::vector<uint8_t>& value) override;

 private:
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> bluetooth_task_runner_;

  win::BluetoothClassicWrapperFake* fake_bt_classic_wrapper_;
  win::BluetoothLowEnergyWrapperFake* fake_bt_le_wrapper_;

  // This is used for retaining access to a single deleted device.
  std::string remembered_device_address_;

  void AdapterInitCallback();
  win::GattService* GetSimulatedService(win::BLEDevice* device,
                                        BluetoothRemoteGattService* service);
  win::GattCharacteristic* GetSimulatedCharacteristic(
      BluetoothRemoteGattCharacteristic* characteristic);

  // Run pending Bluetooth tasks until the first callback that the test fixture
  // tracks is called.
  void RunPendingTasksUntilCallback();
  void ForceRefreshDevice();
  void FinishPendingTasks();
};

// Defines common test fixture name. Use TEST_F(BluetoothTest, YourTestName).
typedef BluetoothTestWin BluetoothTest;

// This test suite represents tests that should run with the new BLE
// implementation both enabled and disabled. This requires declaring tests
// in the following way: TEST_P(BluetoothTestWinrt, YourTestName).
class BluetoothTestWinrt : public BluetoothTestWin,
                           public ::testing::WithParamInterface<bool> {
 public:
  BluetoothTestWinrt();
  ~BluetoothTestWinrt();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::Optional<base::win::ScopedWinrtInitializer> scoped_winrt_initializer_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothTestWinrt);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_BLUETOOTH_TEST_WIN_H_
