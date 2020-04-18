// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_
#define ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_

#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {
namespace tray {
class KeyboardBrightnessView;
}

// Manages a detailed view containing a readonly slider for the keyboard
// backlight brightness. This is used in a bubble and not displayed elsewhere.
class ASH_EXPORT TrayKeyboardBrightness
    : public SystemTrayItem,
      public chromeos::PowerManagerClient::Observer {
 public:
  explicit TrayKeyboardBrightness(SystemTray* system_tray);
  ~TrayKeyboardBrightness() override;

 private:
  friend class TrayKeyboardBrightnessTest;

  // Overridden from SystemTrayItem.
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDetailedViewDestroyed() override;
  void UpdateAfterLoginStatusChange(LoginStatus status) override;
  bool ShouldShowShelf() const override;

  // Overriden from PowerManagerClient::Observer.
  void KeyboardBrightnessChanged(
      const power_manager::BacklightBrightnessChange& change) override;

  tray::KeyboardBrightnessView* brightness_view_ = nullptr;

  // Keyboard brightness level in the range [0.0, 100.0] that we've heard about
  // most recently.
  double current_percent_ = 100.0;

  base::WeakPtrFactory<TrayKeyboardBrightness> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrayKeyboardBrightness);
};

}  // namespace ash

#endif  // ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_
