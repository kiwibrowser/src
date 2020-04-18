// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/apps/chrome_app_window_client.h"

#include "base/command_line.h"
#import "chrome/browser/ui/cocoa/apps/native_app_window_cocoa.h"
#include "chrome/browser/ui/views/apps/chrome_native_app_window_views_mac.h"
#include "chrome/common/chrome_switches.h"
#include "ui/base/ui_features.h"

namespace {

bool UseMacViewsNativeAppWindows() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnableMacViewsNativeAppWindows))
    return true;
  if (command_line->HasSwitch(switches::kDisableMacViewsNativeAppWindows))
    return false;
  return false;  // Current default.
}

}  // namespace

// static
extensions::NativeAppWindow*
ChromeAppWindowClient::CreateNativeAppWindowImplCocoa(
    extensions::AppWindow* app_window,
    const extensions::AppWindow::CreateParams& params) {
  if (UseMacViewsNativeAppWindows()) {
    ChromeNativeAppWindowViewsMac* window = new ChromeNativeAppWindowViewsMac;
    window->Init(app_window, params);
    return window;
  }

  return new NativeAppWindowCocoa(app_window, params);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
extensions::NativeAppWindow* ChromeAppWindowClient::CreateNativeAppWindowImpl(
    extensions::AppWindow* app_window,
    const extensions::AppWindow::CreateParams& params) {
  return CreateNativeAppWindowImplCocoa(app_window, params);
}
#endif
