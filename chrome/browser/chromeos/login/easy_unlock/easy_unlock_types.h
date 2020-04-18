// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_TYPES_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_TYPES_H_

#include <string>
#include <vector>
#include "base/values.h"

namespace chromeos {

extern const char kEasyUnlockKeyMetaNameBluetoothAddress[];
extern const char kEasyUnlockKeyMetaNameBluetoothType[];
extern const char kEasyUnlockKeyMetaNamePsk[];
extern const char kEasyUnlockKeyMetaNamePubKey[];
extern const char kEasyUnlockKeyMetaNameChallenge[];
extern const char kEasyUnlockKeyMetaNameWrappedSecret[];
extern const char kEasyUnlockKeyMetaNameSerializedBeaconSeeds[];

// Device data that is stored with cryptohome keys.
struct EasyUnlockDeviceKeyData {
  // The Bluetooth type. By default, the assumed type is BLUETOOTH_CLASSIC.
  enum BluetoothType { BLUETOOTH_CLASSIC, BLUETOOTH_LE, NUM_BLUETOOTH_TYPES };

  EasyUnlockDeviceKeyData();
  EasyUnlockDeviceKeyData(const EasyUnlockDeviceKeyData& other);
  ~EasyUnlockDeviceKeyData();

  // Bluetooth address of the remote device.
  std::string bluetooth_address;
  // The Bluetooth type to connect to the device.
  BluetoothType bluetooth_type;
  // Public key of the remote device.
  std::string public_key;
  // Key to establish a secure channel with the remote device.
  std::string psk;
  // Challenge bytes to be sent to the phone.
  std::string challenge;
  // Wrapped secret to mount cryptohome home.
  std::string wrapped_secret;
  // Serialized BeaconSeeds used to identify this device.
  std::string serialized_beacon_seeds;
};
typedef std::vector<EasyUnlockDeviceKeyData> EasyUnlockDeviceKeyDataList;

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_TYPES_H_
