// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_H_
#define ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_H_

#include "ash/system/bluetooth/bluetooth_observer.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"

namespace ash {
namespace tray {
class BluetoothDefaultView;
class BluetoothDetailedView;
}

class DetailedViewDelegate;

// Bluetooth section in the main system tray menu. Contains:
// * Toggle to turn Bluetooth on and off
// * Gear icon that takes the user to the web ui settings
// * List of paired devices
// * List of unpaired devices
class TrayBluetooth : public SystemTrayItem, public BluetoothObserver {
 public:
  explicit TrayBluetooth(SystemTray* system_tray);
  ~TrayBluetooth() override;

 private:
  // Overridden from SystemTrayItem.
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;
  void UpdateAfterLoginStatusChange(LoginStatus status) override;

  // Overridden from BluetoothObserver.
  void OnBluetoothRefresh() override;
  void OnBluetoothDiscoveringChanged() override;

  tray::BluetoothDefaultView* default_;
  tray::BluetoothDetailedView* detailed_;

  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TrayBluetooth);
};

}  // namespace ash

#endif  // ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_H_
