// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/ash_util.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/event_properties.mojom.h"
#include "ash/shell.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace ash_util {

bool ShouldOpenAshOnStartup() {
  return !IsRunningInMash();
}

bool IsRunningInMash() {
  return chromeos::GetAshConfig() == ash::Config::MASH;
}

bool IsAcceleratorDeprecated(const ui::Accelerator& accelerator) {
  // When running in mash the browser doesn't handle ash accelerators.
  if (IsRunningInMash())
    return false;

  return ash::Shell::Get()->accelerator_controller()->IsDeprecated(accelerator);
}

bool WillAshProcessAcceleratorForEvent(const ui::KeyEvent& key_event) {
  return key_event.properties() &&
         key_event.properties()->count(
             ash::mojom::kWillProcessAccelerator_KeyEventProperty);
}

void SetupWidgetInitParamsForContainer(views::Widget::InitParams* params,
                                       int container_id) {
  DCHECK_GE(container_id, ash::kShellWindowId_MinContainer);
  DCHECK_LE(container_id, ash::kShellWindowId_MaxContainer);

  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    using ui::mojom::WindowManager;
    params->mus_properties[WindowManager::kContainerId_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(container_id);
    params->mus_properties[WindowManager::kDisplayId_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(
            display::Screen::GetScreen()->GetPrimaryDisplay().id());
  } else {
    params->parent = ash::Shell::GetContainer(
        ash::Shell::GetPrimaryRootWindow(), container_id);
  }
}

}  // namespace ash_util
