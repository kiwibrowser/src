// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/extension_app_window_launcher_item_controller.h"

#include "ash/public/cpp/shelf_item_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/gfx/image/image.h"

ExtensionAppWindowLauncherItemController::
    ExtensionAppWindowLauncherItemController(const ash::ShelfID& shelf_id)
    : AppWindowLauncherItemController(shelf_id) {}

ExtensionAppWindowLauncherItemController::
    ~ExtensionAppWindowLauncherItemController() {}

void ExtensionAppWindowLauncherItemController::AddAppWindow(
    extensions::AppWindow* app_window) {
  DCHECK(!app_window->window_type_is_panel());
  AddWindow(app_window->GetBaseWindow());
}

ash::MenuItemList ExtensionAppWindowLauncherItemController::GetAppMenuItems(
    int event_flags) {
  ash::MenuItemList items;
  extensions::AppWindowRegistry* app_window_registry =
      extensions::AppWindowRegistry::Get(
          ChromeLauncherController::instance()->profile());

  uint32_t window_index = 0;
  for (const ui::BaseWindow* window : windows()) {
    extensions::AppWindow* app_window =
        app_window_registry->GetAppWindowForNativeWindow(
            window->GetNativeWindow());
    DCHECK(app_window);

    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->command_id = window_index;
    item->label = app_window->GetTitle();

    // Use the app's web contents favicon, or the app window's icon.
    favicon::FaviconDriver* favicon_driver =
        favicon::ContentFaviconDriver::FromWebContents(
            app_window->web_contents());
    item->image = favicon_driver->GetFavicon().AsImageSkia();
    if (item->image.isNull()) {
      const gfx::ImageSkia* app_icon = nullptr;
      if (app_window->GetNativeWindow()) {
        app_icon = app_window->GetNativeWindow()->GetProperty(
            aura::client::kAppIconKey);
      }
      if (app_icon && !app_icon->isNull())
        item->image = *app_icon;
    }

    items.push_back(std::move(item));
    ++window_index;
  }
  return items;
}

void ExtensionAppWindowLauncherItemController::ExecuteCommand(
    bool from_context_menu,
    int64_t command_id,
    int32_t event_flags,
    int64_t display_id) {
  if (from_context_menu && ExecuteContextMenuCommand(command_id, event_flags))
    return;

  ChromeLauncherController::instance()->ActivateShellApp(app_id(), command_id);
}

void ExtensionAppWindowLauncherItemController::OnWindowTitleChanged(
    aura::Window* window) {
  ui::BaseWindow* base_window = GetAppWindow(window);
  extensions::AppWindowRegistry* app_window_registry =
      extensions::AppWindowRegistry::Get(
          ChromeLauncherController::instance()->profile());
  extensions::AppWindow* app_window =
      app_window_registry->GetAppWindowForNativeWindow(
          base_window->GetNativeWindow());

  // Use the window title (if set) to differentiate show_in_shelf window shelf
  // items instead of the default behavior of using the app name.
  if (app_window->show_in_shelf()) {
    base::string16 title = window->GetTitle();
    if (!title.empty())
      ChromeLauncherController::instance()->SetItemTitle(shelf_id(), title);
  }
}
