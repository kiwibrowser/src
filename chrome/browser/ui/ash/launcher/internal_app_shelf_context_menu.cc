// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/internal_app_shelf_context_menu.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/shelf_item.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/ui_base_features.h"

InternalAppShelfContextMenu::InternalAppShelfContextMenu(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    int64_t display_id)
    : LauncherContextMenu(controller, item, display_id) {}

void InternalAppShelfContextMenu::GetMenuModel(GetMenuModelCallback callback) {
  auto menu_model = std::make_unique<ui::SimpleMenuModel>(this);
  BuildMenu(menu_model.get());
  std::move(callback).Run(std::move(menu_model));
}

void InternalAppShelfContextMenu::BuildMenu(ui::SimpleMenuModel* menu_model) {
  const bool app_is_open = controller()->IsOpen(item().id);
  if (!app_is_open) {
    AddContextMenuOption(menu_model, MENU_OPEN_NEW,
                         IDS_APP_CONTEXT_MENU_ACTIVATE_ARC);
    if (!features::IsTouchableAppContextMenuEnabled())
      menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  }

  const auto* internal_app = app_list::FindInternalApp(item().id.app_id);
  DCHECK(internal_app);
  if (internal_app->show_in_launcher)
    AddPinMenu(menu_model);

  if (app_is_open) {
    AddContextMenuOption(menu_model, MENU_CLOSE,
                         IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  }

  // Default menu items, e.g. "Autohide shelf" are appended. Adding a separator.
  if (!features::IsTouchableAppContextMenuEnabled())
    menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
}
