// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/multi_profile_app_window_launcher_controller.h"

#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/window_properties.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "components/account_id/account_id.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "ui/aura/window.h"

MultiProfileAppWindowLauncherController::
    MultiProfileAppWindowLauncherController(ChromeLauncherController* owner)
    : ExtensionAppWindowLauncherController(owner) {
  // We might have already active windows.
  extensions::AppWindowRegistry* registry =
      extensions::AppWindowRegistry::Get(owner->profile());
  app_window_list_.insert(app_window_list_.end(),
                          registry->app_windows().begin(),
                          registry->app_windows().end());
}

MultiProfileAppWindowLauncherController::
    ~MultiProfileAppWindowLauncherController() {
  // We need to remove all Registry observers for added users.
  for (extensions::AppWindowRegistry* registry : multi_user_registry_)
    registry->RemoveObserver(this);
}

void MultiProfileAppWindowLauncherController::ActiveUserChanged(
    const std::string& user_email) {
  // The active user has changed and we need to traverse our list of items to
  // show / hide them one by one. To avoid that a user dependent state
  // "survives" in a launcher item, we first delete all items making sure that
  // nothing remains and then re-create them again.
  for (extensions::AppWindow* app_window : app_window_list_) {
    Profile* profile =
        Profile::FromBrowserContext(app_window->browser_context());
    if (!multi_user_util::IsProfileFromActiveUser(profile)) {
      if (IsRegisteredApp(app_window->GetNativeWindow())) {
        UnregisterApp(app_window->GetNativeWindow());
      } else if (app_window->window_type_is_panel()) {
        // Panels are not registered; hide by clearing the shelf item type.
        app_window->GetNativeWindow()->SetProperty<int>(ash::kShelfItemTypeKey,
                                                        ash::TYPE_UNDEFINED);
      }
    }
  }
  for (extensions::AppWindow* app_window : app_window_list_) {
    Profile* profile =
        Profile::FromBrowserContext(app_window->browser_context());
    if (multi_user_util::IsProfileFromActiveUser(profile) &&
        !IsRegisteredApp(app_window->GetNativeWindow()) &&
        (app_window->GetBaseWindow()->IsMinimized() ||
         app_window->GetNativeWindow()->IsVisible())) {
      if (app_window->window_type_is_panel()) {
        // Panels are not registered; show by restoring the shelf item type.
        app_window->GetNativeWindow()->SetProperty<int>(ash::kShelfItemTypeKey,
                                                        ash::TYPE_APP_PANEL);
      }
      RegisterApp(app_window);
    }
  }
}

void MultiProfileAppWindowLauncherController::AdditionalUserAddedToSession(
    Profile* profile) {
  // Each users AppWindowRegistry needs to be observed.
  extensions::AppWindowRegistry* registry =
      extensions::AppWindowRegistry::Get(profile);
  DCHECK(registry->app_windows().empty());
  multi_user_registry_.push_back(registry);
  registry->AddObserver(this);
}

void MultiProfileAppWindowLauncherController::OnAppWindowAdded(
    extensions::AppWindow* app_window) {
  app_window_list_.push_back(app_window);
  Profile* profile = Profile::FromBrowserContext(app_window->browser_context());
  // If the window was created for an inactive user, but the user allowed the
  // app to teleport to the current user's desktop, teleport this window now.
  if (!multi_user_util::IsProfileFromActiveUser(profile) &&
      UserHasAppOnActiveDesktop(app_window)) {
    MultiUserWindowManager::GetInstance()->ShowWindowForUser(
        app_window->GetNativeWindow(), multi_user_util::GetCurrentAccountId());
  }

  // If the window was created for the active user or it has been teleported to
  // the current user's desktop, register it to show an item on the shelf.
  if (multi_user_util::IsProfileFromActiveUser(profile) ||
      UserHasAppOnActiveDesktop(app_window)) {
    RegisterApp(app_window);
  }
}

void MultiProfileAppWindowLauncherController::OnAppWindowShown(
    extensions::AppWindow* app_window,
    bool was_hidden) {
  Profile* profile = Profile::FromBrowserContext(app_window->browser_context());

  if (multi_user_util::IsProfileFromActiveUser(profile) &&
      !IsRegisteredApp(app_window->GetNativeWindow())) {
    RegisterApp(app_window);
    return;
  }

  // The panel layout manager only manages windows which are anchored.
  // Since this window did never had an anchor, it would stay hidden. We
  // therefore make it visible now.
  if (UserHasAppOnActiveDesktop(app_window) &&
      app_window->GetNativeWindow()->type() ==
          aura::client::WINDOW_TYPE_PANEL &&
      !app_window->GetNativeWindow()->layer()->GetTargetOpacity()) {
    app_window->GetNativeWindow()->layer()->SetOpacity(1.0f);
  }
}

void MultiProfileAppWindowLauncherController::OnAppWindowHidden(
    extensions::AppWindow* app_window) {
  Profile* profile = Profile::FromBrowserContext(app_window->browser_context());
  if (multi_user_util::IsProfileFromActiveUser(profile) &&
      IsRegisteredApp(app_window->GetNativeWindow())) {
    UnregisterApp(app_window->GetNativeWindow());
  }
}

void MultiProfileAppWindowLauncherController::OnAppWindowRemoved(
    extensions::AppWindow* app_window) {
  // If the application is registered with AppWindowLauncher (because the user
  // is currently active), the OnWindowDestroying observer has already (or will
  // soon) unregister it independently from the shelf. If it was not registered
  // we don't need to do anything anyways. As such, all which is left to do here
  // is to get rid of our own reference.
  AppWindowList::iterator it =
      std::find(app_window_list_.begin(), app_window_list_.end(), app_window);
  DCHECK(it != app_window_list_.end());
  app_window_list_.erase(it);
}

bool MultiProfileAppWindowLauncherController::UserHasAppOnActiveDesktop(
    extensions::AppWindow* app_window) {
  const std::string& app_id = app_window->extension_id();
  content::BrowserContext* app_context = app_window->browser_context();
  DCHECK(!app_context->IsOffTheRecord());
  const AccountId current_account_id = multi_user_util::GetCurrentAccountId();
  MultiUserWindowManager* manager = MultiUserWindowManager::GetInstance();
  for (extensions::AppWindow* other_window : app_window_list_) {
    DCHECK(!other_window->browser_context()->IsOffTheRecord());
    if (manager->IsWindowOnDesktopOfUser(other_window->GetNativeWindow(),
                                         current_account_id) &&
        app_id == other_window->extension_id() &&
        app_context == other_window->browser_context()) {
      return true;
    }
  }
  return false;
}
