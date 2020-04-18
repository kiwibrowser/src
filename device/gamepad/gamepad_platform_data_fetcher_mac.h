// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_
#define DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stddef.h>

#include <memory>

#include "base/mac/scoped_cftyperef.h"
#include "base/macros.h"
#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/gamepad/gamepad_device_mac.h"
#include "device/gamepad/public/cpp/gamepad.h"

namespace device {

class GamepadPlatformDataFetcherMac : public GamepadDataFetcher {
 public:
  using Factory = GamepadDataFetcherFactoryImpl<GamepadPlatformDataFetcherMac,
                                                GAMEPAD_SOURCE_MAC_HID>;

  GamepadPlatformDataFetcherMac();
  ~GamepadPlatformDataFetcherMac() override;

  GamepadSource source() override;

  void GetGamepadData(bool devices_changed_hint) override;
  void PauseHint(bool paused) override;
  void PlayEffect(
      int source_id,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback) override;
  void ResetVibration(
      int source_id,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback) override;

 private:
  bool enabled_;
  bool paused_;
  base::ScopedCFTypeRef<IOHIDManagerRef> hid_manager_ref_;

  static GamepadPlatformDataFetcherMac* InstanceFromContext(void* context);
  static void DeviceAddCallback(void* context,
                                IOReturn result,
                                void* sender,
                                IOHIDDeviceRef ref);
  static void DeviceRemoveCallback(void* context,
                                   IOReturn result,
                                   void* sender,
                                   IOHIDDeviceRef ref);
  static void ValueChangedCallback(void* context,
                                   IOReturn result,
                                   void* sender,
                                   IOHIDValueRef ref);

  void OnAddedToProvider() override;

  // Returns the index of the first empty slot, or Gamepads::kItemsLengthCap if
  // there are no empty slots.
  size_t GetEmptySlot();

  // Returns the index of the slot allocated for this device, or the first empty
  // slot if none is yet allocated. If there is no allocated or empty slots,
  // returns Gamepads::kItemsLengthCap.
  size_t GetSlotForDevice(IOHIDDeviceRef device);

  // Returns the index of the slot allocated for the device with the specified
  // |location_id|, or Gamepads::kItemsLengthCap if none is yet allocated.
  size_t GetSlotForLocation(int location_id);

  // Query device info for |device| and add it to |devices_| if it is a
  // gamepad.
  void DeviceAdd(IOHIDDeviceRef device);

  // Remove |device| from the set of connected devices.
  void DeviceRemove(IOHIDDeviceRef device);

  // Update the gamepad state for the button or axis referred to by |value|.
  void ValueChanged(IOHIDValueRef value);

  // Register for connection events and value change notifications for HID
  // devices.
  void RegisterForNotifications();

  // Unregister from connection events and value change notifications.
  void UnregisterFromNotifications();

  std::unique_ptr<GamepadDeviceMac> devices_[Gamepads::kItemsLengthCap];

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherMac);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_MAC_H_
