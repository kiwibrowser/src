// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_REMOTE_ATTRIBUTE_H_
#define COMPONENTS_CRYPTAUTH_BLE_REMOTE_ATTRIBUTE_H_

#include <string>

#include "device/bluetooth/bluetooth_uuid.h"

namespace cryptauth {

// Represents an attribute in the peripheral (service or characteristic).
struct RemoteAttribute {
  device::BluetoothUUID uuid;
  std::string id;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_REMOTE_ATTRIBUTE_H_
