// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_app_window_client.h"

#include "extensions/browser/app_window/app_window.h"
#include "extensions/shell/browser/desktop_controller.h"
#include "extensions/shell/browser/shell_native_app_window_aura.h"

namespace extensions {

NativeAppWindow* ShellAppWindowClient::CreateNativeAppWindow(
    AppWindow* window,
    AppWindow::CreateParams* params) {
  ShellNativeAppWindow* native_app_window =
      new ShellNativeAppWindowAura(window, *params);
  DesktopController::instance()->AddAppWindow(
      window, native_app_window->GetNativeWindow());
  return native_app_window;
}

}  // namespace extensions
