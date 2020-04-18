// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_DESKTOP_CONTROLLER_H_
#define EXTENSIONS_SHELL_BROWSER_DESKTOP_CONTROLLER_H_

#include "ui/gfx/native_widget_types.h"

namespace extensions {
class AppWindow;

// DesktopController is an interface to construct the window environment in
// extensions shell. ShellDesktopControllerAura provides a default
// implementation for app_shell, and other embedders can provide their own.
// TODO(jamescook|oshima): Clean up this interface now that there is only one
// way to create an app window.
class DesktopController {
 public:
  DesktopController();
  virtual ~DesktopController();

  // Returns the single instance of the desktop. (Stateless functions like
  // ShellAppWindowCreateFunction need to be able to access the desktop, so
  // we need a singleton somewhere).
  static DesktopController* instance();

  // Runs the desktop and quits when finished.
  virtual void Run() = 0;

  // Attaches the window to our window hierarchy.
  virtual void AddAppWindow(AppWindow* app_window,
                            gfx::NativeWindow window) = 0;

  // Closes and destroys the app windows.
  virtual void CloseAppWindows() = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_DESKTOP_CONTROLLER_H_
