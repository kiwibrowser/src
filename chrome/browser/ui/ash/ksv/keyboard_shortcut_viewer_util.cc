// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/ksv/keyboard_shortcut_viewer_util.h"

#include "ash/components/shortcut_viewer/public/mojom/constants.mojom.h"
#include "ash/components/shortcut_viewer/views/keyboard_shortcut_view.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "base/command_line.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace keyboard_shortcut_viewer_util {

void ShowKeyboardShortcutViewer() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          ash::switches::kKeyboardShortcutViewerApp)) {
    service_manager::Connector* connector =
        content::ServiceManagerConnection::GetForProcess()->GetConnector();
    connector->StartService(shortcut_viewer::mojom::kServiceName);
  } else {
    // TODO(https://crbug.com/833673): Remove the dependency on aura::Window.
    keyboard_shortcut_viewer::KeyboardShortcutView::Toggle(
        ash::Shell::HasInstance() ? ash::Shell::GetRootWindowForNewWindows()
                                  : nullptr);
  }
}

}  // namespace keyboard_shortcut_viewer_util
