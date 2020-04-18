// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_EXTENSION_APP_WINDOW_LAUNCHER_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_EXTENSION_APP_WINDOW_LAUNCHER_CONTROLLER_H_

#include <list>
#include <map>
#include <string>

#include "ash/public/cpp/shelf_types.h"
#include "base/macros.h"
#include "chrome/browser/ui/ash/launcher/app_window_launcher_controller.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "ui/aura/window_observer.h"

namespace aura {
class Window;
}

namespace extensions {
class AppWindow;
}

class ChromeLauncherController;
class ExtensionAppWindowLauncherItemController;

// AppWindowLauncherController observes the app window registry and the
// aura window manager. It handles adding and removing launcher items from
// ChromeLauncherController.
class ExtensionAppWindowLauncherController
    : public AppWindowLauncherController,
      public extensions::AppWindowRegistry::Observer,
      public aura::WindowObserver {
 public:
  explicit ExtensionAppWindowLauncherController(
      ChromeLauncherController* owner);
  ~ExtensionAppWindowLauncherController() override;

  // AppWindowLauncherController:
  AppWindowLauncherItemController* ControllerForWindow(
      aura::Window* window) override;
  void OnItemDelegateDiscarded(ash::ShelfItemDelegate* delegate) override;

  // Overridden from AppWindowRegistry::Observer:
  void OnAppWindowAdded(extensions::AppWindow* app_window) override;
  void OnAppWindowShown(extensions::AppWindow* app_window,
                        bool was_hidden) override;
  void OnAppWindowHidden(extensions::AppWindow* app_window) override;

  // Overriden from aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

 protected:
  // Registers an app window with the shelf and this object.
  void RegisterApp(extensions::AppWindow* app_window);

  // Unregisters an app window with the shelf and this object.
  void UnregisterApp(aura::Window* window);

  // Check if a given window is known to the launcher controller.
  bool IsRegisteredApp(aura::Window* window);

 private:
  using AppControllerMap =
      std::map<ash::ShelfID, ExtensionAppWindowLauncherItemController*>;

  // The AppWindowRegistry for the active user (represented by |owner()|).
  extensions::AppWindowRegistry* registry_;

  // Map of shelf id to controller.
  AppControllerMap app_controller_map_;

  // Map of aura::Windows to shelf ids.
  std::map<aura::Window*, ash::ShelfID> window_to_shelf_id_map_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionAppWindowLauncherController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_EXTENSION_APP_WINDOW_LAUNCHER_CONTROLLER_H_
