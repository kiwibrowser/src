// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_CAPS_LOCK_H_
#define ASH_SYSTEM_TRAY_CAPS_LOCK_H_

#include "ash/ime/ime_controller.h"
#include "ash/system/tray/tray_image_item.h"
#include "base/macros.h"

namespace views {
class View;
}

namespace ash {
class CapsLockDefaultView;

// Shows a status area icon and a system tray menu item when caps lock is on.
class TrayCapsLock : public TrayImageItem, public ImeController::Observer {
 public:
  explicit TrayCapsLock(SystemTray* system_tray);
  ~TrayCapsLock() override;

  // Overridden from ImeController::Observer:
  void OnCapsLockChanged(bool enabled) override;
  void OnKeyboardLayoutNameChanged(const std::string&) override {}

  // Overridden from TrayImageItem.
  bool GetInitialVisibility() override;
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;

 private:
  CapsLockDefaultView* default_;

  bool caps_lock_enabled_;

  DISALLOW_COPY_AND_ASSIGN(TrayCapsLock);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_CAPS_LOCK_H_
