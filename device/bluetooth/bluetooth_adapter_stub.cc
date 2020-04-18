// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace device {

// Return a null ptr. Link this when there is no suitable BluetoothAdapter for
// a particular platform.
// static
base::WeakPtr<BluetoothAdapter> BluetoothAdapter::CreateAdapter(
    InitCallback init_callback) {
  return base::WeakPtr<BluetoothAdapter>();
}

}  // namespace device
