// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/components/shortcut_viewer/shortcut_viewer_application.h"

#include "ash/components/shortcut_viewer/views/keyboard_shortcut_view.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/views/mus/aura_init.h"

namespace keyboard_shortcut_viewer {

ShortcutViewerApplication::ShortcutViewerApplication() = default;
ShortcutViewerApplication::~ShortcutViewerApplication() = default;

void ShortcutViewerApplication::OnStart() {
  aura_init_ = views::AuraInit::Create(
      context()->connector(), context()->identity(), "views_mus_resources.pak",
      std::string(), nullptr, views::AuraInit::Mode::AURA_MUS2,
      false /*register_path_provider*/);
  if (!aura_init_) {
    context()->QuitNow();
    return;
  }

  // This app needs InputDeviceManager information that loads asynchronously via
  // InputDeviceClient. If the device list is incomplete, wait for it to load.
  DCHECK(ui::InputDeviceManager::HasInstance());
  if (ui::InputDeviceManager::GetInstance()->AreDeviceListsComplete()) {
    // TODO(crbug.com/841020): Place the new app window on the correct display.
    KeyboardShortcutView::Toggle(nullptr);
  } else {
    ui::InputDeviceManager::GetInstance()->AddObserver(this);
  }
}

void ShortcutViewerApplication::OnDeviceListsComplete() {
  ui::InputDeviceManager::GetInstance()->RemoveObserver(this);
  // TODO(crbug.com/841020): Place the new app window on the correct display.
  KeyboardShortcutView::Toggle(nullptr);
}

}  // namespace keyboard_shortcut_viewer
