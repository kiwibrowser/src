// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_MULTI_PROFILE_APP_WINDOW_LAUNCHER_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_MULTI_PROFILE_APP_WINDOW_LAUNCHER_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/ash/launcher/extension_app_window_launcher_controller.h"

// Inherits from AppWindowLauncherController and overwrites the AppWindow
// observing functions to switch between users dynamically.
class MultiProfileAppWindowLauncherController
    : public ExtensionAppWindowLauncherController {
 public:
  explicit MultiProfileAppWindowLauncherController(
      ChromeLauncherController* owner);
  ~MultiProfileAppWindowLauncherController() override;

  // Overridden from AppWindowLauncherController:
  void ActiveUserChanged(const std::string& user_email) override;
  void AdditionalUserAddedToSession(Profile* profile) override;

  // Overridden from AppWindowRegistry::Observer:
  void OnAppWindowAdded(extensions::AppWindow* app_window) override;
  void OnAppWindowRemoved(extensions::AppWindow* app_window) override;
  void OnAppWindowShown(extensions::AppWindow* app_window,
                        bool was_hidden) override;
  void OnAppWindowHidden(extensions::AppWindow* app_window) override;

 private:
  typedef std::vector<extensions::AppWindow*> AppWindowList;
  typedef std::vector<extensions::AppWindowRegistry*> AppWindowRegistryList;

  // Returns true if the owner of the given |app_window| has a window teleported
  // of the |app_window|'s application type to the current desktop.
  bool UserHasAppOnActiveDesktop(extensions::AppWindow* app_window);

  // A list of all app windows for all users.
  AppWindowList app_window_list_;

  // A list of the app window registries which we additionally observe.
  AppWindowRegistryList multi_user_registry_;

  DISALLOW_COPY_AND_ASSIGN(MultiProfileAppWindowLauncherController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_MULTI_PROFILE_APP_WINDOW_LAUNCHER_CONTROLLER_H_
