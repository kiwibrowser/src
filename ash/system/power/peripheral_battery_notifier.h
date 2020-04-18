// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_PERIPHERAL_BATTERY_NOTIFIER_H_
#define ASH_SYSTEM_POWER_PERIPHERAL_BATTERY_NOTIFIER_H_

#include <map>

#include "ash/ash_export.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/tick_clock.h"
#include "chromeos/dbus/power_manager_client.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace ash {

class BluetoothDevice;
class PeripheralBatteryNotifierTest;

// This class listens for peripheral device battery status and shows
// notifications for low battery conditions.
class ASH_EXPORT PeripheralBatteryNotifier
    : public chromeos::PowerManagerClient::Observer,
      public device::BluetoothAdapter::Observer {
 public:
  static const char kStylusNotificationId[];

  // This class registers/unregisters itself as an observer in ctor/dtor.
  PeripheralBatteryNotifier();
  ~PeripheralBatteryNotifier() override;

  void set_testing_clock(const base::TickClock* clock) {
    testing_clock_ = clock;
  }

  // chromeos::PowerManagerClient::Observer:
  void PeripheralBatteryStatusReceived(const std::string& path,
                                       const std::string& name,
                                       int level) override;

  // device::BluetoothAdapter::Observer:
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

 private:
  friend class PeripheralBatteryNotifierTest;
  FRIEND_TEST_ALL_PREFIXES(PeripheralBatteryNotifierTest, Basic);
  FRIEND_TEST_ALL_PREFIXES(PeripheralBatteryNotifierTest, InvalidBatteryInfo);
  FRIEND_TEST_ALL_PREFIXES(PeripheralBatteryNotifierTest,
                           ExtractBluetoothAddress);
  FRIEND_TEST_ALL_PREFIXES(PeripheralBatteryNotifierTest, DeviceRemove);

  struct BatteryInfo {
    // Human readable name for the device. It is changeable.
    std::string name;
    // Battery level within range [0, 100], and -1 for unknown level.
    int level = -1;
    base::TimeTicks last_notification_timestamp;
    bool is_stylus = false;
    // Peripheral's Bluetooth address. Empty for non-Bluetooth devices.
    std::string bluetooth_address;
  };

  void InitializeOnBluetoothReady(
      scoped_refptr<device::BluetoothAdapter> adapter);

  // Removes the Bluetooth battery with address |bluetooth_address|, as well as
  // the associated notification. Called when a bluetooth device has been
  // changed or removed.
  void RemoveBluetoothBattery(const std::string& bluetooth_address);

  // Posts a low battery notification with unique id |path|. Returns true
  // if the notification is posted, false if not.
  bool PostNotification(const std::string& path, const BatteryInfo& battery);

  void CancelNotification(const std::string& path);

  // Record of existing battery infomation. The key is the device path.
  std::map<std::string, BatteryInfo> batteries_;

  // PeripheralBatteryNotifier is an observer of |bluetooth_adapter_| for
  // bluetooth device change/remove events.
  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;

  // Used only for helping test. Not owned and can be nullptr.
  const base::TickClock* testing_clock_ = nullptr;

  std::unique_ptr<base::WeakPtrFactory<PeripheralBatteryNotifier>>
      weakptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PeripheralBatteryNotifier);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_PERIPHERAL_BATTERY_NOTIFIER_H_
