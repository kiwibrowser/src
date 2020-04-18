// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BLUETOOTH_BLUETOOTH_OBSERVER_H_
#define ASH_SYSTEM_BLUETOOTH_BLUETOOTH_OBSERVER_H_

namespace ash {

class BluetoothObserver {
 public:
  virtual ~BluetoothObserver() {}

  virtual void OnBluetoothRefresh() = 0;
  virtual void OnBluetoothDiscoveringChanged() = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_BLUETOOTH_BLUETOOTH_OBSERVER_H_
