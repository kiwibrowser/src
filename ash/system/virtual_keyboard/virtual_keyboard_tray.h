// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_TRAY_H_
#define ASH_SYSTEM_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_TRAY_H_

#include "ash/keyboard/keyboard_ui_observer.h"
#include "ash/shell_observer.h"
#include "ash/system/tray/tray_background_view.h"
#include "base/macros.h"
#include "ui/keyboard/keyboard_controller_observer.h"

namespace views {
class ImageView;
}

namespace ash {

// TODO(sky): make this visible on non-chromeos platforms.
class VirtualKeyboardTray : public TrayBackgroundView,
                            public KeyboardUIObserver,
                            public keyboard::KeyboardControllerObserver,
                            public ShellObserver {
 public:
  explicit VirtualKeyboardTray(Shelf* shelf);
  ~VirtualKeyboardTray() override;

  // TrayBackgroundView:
  base::string16 GetAccessibleNameForTray() override;
  void HideBubbleWithView(const views::TrayBubbleView* bubble_view) override;
  void ClickedOutsideBubble() override;
  bool PerformAction(const ui::Event& event) override;

  // KeyboardUIObserver:
  void OnKeyboardEnabledStateChanged(bool new_enabled) override;

  // keyboard::KeyboardControllerObserver:
  void OnKeyboardAvailabilityChanged(const bool is_available) override;

  // ShellObserver:
  void OnKeyboardControllerCreated() override;

 private:
  void ObserveKeyboardController();
  void UnobserveKeyboardController();

  // Weak pointer, will be parented by TrayContainer for its lifetime.
  views::ImageView* icon_;

  Shelf* shelf_;

  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardTray);
};

}  // namespace ash

#endif  // ASH_SYSTEM_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_TRAY_H_
