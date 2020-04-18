// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DISPLAY_ROTATION_DEFAULT_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DISPLAY_ROTATION_DEFAULT_HANDLER_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "ash/display/window_tree_host_manager.h"
#include "ash/shell_observer.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "ui/display/display.h"

namespace policy {

// Enforces the device policy DisplayRotationDefault.
// This class is created in ChromeShellDelegate::PreInit() and registers
// itself with WindowTreeHostManager as Observer for display changes, and
// with CrosSettings as Observer for setting changes. Whenever there is a change
// in the display configuration, any new display with an id that is not already
// in rotated_displays_ will be rotated according to the policy. When there is a
// change to the CrosSettings, the new policy is applied to all connected
// displays.
// This class owns itself and destroys itself when OnShutdown is called by
// WindowTreeHostManager.
class DisplayRotationDefaultHandler
    : public ash::WindowTreeHostManager::Observer,
      public ash::ShellObserver {
 public:
  DisplayRotationDefaultHandler();
  ~DisplayRotationDefaultHandler() override;

  // ash::ShellObserver:
  void OnShellInitialized() override;

  // ash::WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;
  void OnWindowTreeHostManagerShutdown() override;

 private:
  // Callback function for settings_observer_.
  void OnCrosSettingsChanged();

  // Applies the policy to all connected displays as necessary.
  void RotateDisplays();

  // Reads |chromeos::kDisplayRotationDefault| from CrosSettings and stores
  // its value, and whether it has a value, in member variables
  // |display_rotation_default_| and |policy_enabled_|. Returns true if the
  // setting changed.
  bool UpdateFromCrosSettings();

  bool policy_enabled_ = false;
  display::Display::Rotation display_rotation_default_ =
      display::Display::ROTATE_0;
  std::set<int64_t> rotated_displays_;
  bool rotation_in_progress_ = false;

  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      settings_observer_;

  DISALLOW_COPY_AND_ASSIGN(DisplayRotationDefaultHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DISPLAY_ROTATION_DEFAULT_HANDLER_H_
