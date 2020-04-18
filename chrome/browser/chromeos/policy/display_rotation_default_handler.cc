// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/display_rotation_default_handler.h"

#include <stddef.h>

#include "ash/shell.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/settings/cros_settings_names.h"
#include "ui/display/manager/display_manager.h"

namespace policy {

DisplayRotationDefaultHandler::DisplayRotationDefaultHandler() {
  ash::Shell::Get()->window_tree_host_manager()->AddObserver(this);
  ash::Shell::Get()->AddShellObserver(this);
  settings_observer_ = chromeos::CrosSettings::Get()->AddSettingsObserver(
      chromeos::kDisplayRotationDefault,
      base::Bind(&DisplayRotationDefaultHandler::OnCrosSettingsChanged,
                 base::Unretained(this)));
}

DisplayRotationDefaultHandler::~DisplayRotationDefaultHandler() {
}

void DisplayRotationDefaultHandler::OnShellInitialized() {
  UpdateFromCrosSettings();
  RotateDisplays();
}

void DisplayRotationDefaultHandler::OnDisplayConfigurationChanged() {
  RotateDisplays();
}

void DisplayRotationDefaultHandler::OnWindowTreeHostManagerShutdown() {
  ash::Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
  ash::Shell::Get()->RemoveShellObserver(this);
  settings_observer_.reset();
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void DisplayRotationDefaultHandler::OnCrosSettingsChanged() {
  if (!UpdateFromCrosSettings())
    return;
  // Policy changed, so reset all displays.
  rotated_displays_.clear();
  RotateDisplays();
}

void DisplayRotationDefaultHandler::RotateDisplays() {
  if (!policy_enabled_ || rotation_in_progress_)
    return;

  // Avoid nested calls of this function due to OnDisplayConfigurationChanged
  // being called by rotations here.
  rotation_in_progress_ = true;

  display::DisplayManager* const display_manager =
      ash::Shell::Get()->display_manager();
  const size_t num_displays = display_manager->GetNumDisplays();
  for (size_t i = 0; i < num_displays; ++i) {
    const display::Display& display = display_manager->GetDisplayAt(i);
    const int64_t id = display.id();
    if (rotated_displays_.find(id) == rotated_displays_.end()) {
      rotated_displays_.insert(id);
      if (display.rotation() != display_rotation_default_) {
        display_manager->SetDisplayRotation(
            id, display_rotation_default_,
            display::Display::RotationSource::ACTIVE);
      }
    }
  }
  rotation_in_progress_ = false;
}

bool DisplayRotationDefaultHandler::UpdateFromCrosSettings() {
  int new_rotation;
  bool new_policy_enabled = chromeos::CrosSettings::Get()->GetInteger(
      chromeos::kDisplayRotationDefault, &new_rotation);
  display::Display::Rotation new_display_rotation_default =
      display::Display::ROTATE_0;
  if (new_policy_enabled) {
    if (new_rotation >= display::Display::ROTATE_0 &&
        new_rotation <= display::Display::ROTATE_270) {
      new_display_rotation_default =
          static_cast<display::Display::Rotation>(new_rotation);
    } else {
      LOG(ERROR) << "CrosSettings contains invalid value " << new_rotation
                 << " for DisplayRotationDefault. Ignoring setting.";
      new_policy_enabled = false;
    }
  }
  if (new_policy_enabled != policy_enabled_ ||
      (new_policy_enabled &&
       new_display_rotation_default != display_rotation_default_)) {
    policy_enabled_ = new_policy_enabled;
    display_rotation_default_ = new_display_rotation_default;
    return true;
  }
  return false;
}

}  // namespace policy
