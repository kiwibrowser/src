// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/arc_launcher_context_menu.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/shelf_item.h"
#include "chrome/browser/chromeos/arc/app_shortcuts/arc_app_shortcuts_menu_builder.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/arc_app_shelf_id.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/ui_base_features.h"

ArcLauncherContextMenu::ArcLauncherContextMenu(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    int64_t display_id)
    : LauncherContextMenu(controller, item, display_id) {}

ArcLauncherContextMenu::~ArcLauncherContextMenu() = default;

void ArcLauncherContextMenu::GetMenuModel(GetMenuModelCallback callback) {
  BuildMenu(std::make_unique<ui::SimpleMenuModel>(this), std::move(callback));
}

void ArcLauncherContextMenu::ExecuteCommand(int command_id, int event_flags) {
  if (command_id >= LAUNCH_APP_SHORTCUT_FIRST &&
      command_id <= LAUNCH_APP_SHORTCUT_LAST) {
    DCHECK(app_shortcuts_menu_builder_);
    app_shortcuts_menu_builder_->ExecuteCommand(command_id);
    return;
  }

  LauncherContextMenu::ExecuteCommand(command_id, event_flags);
}

void ArcLauncherContextMenu::BuildMenu(
    std::unique_ptr<ui::SimpleMenuModel> menu_model,
    GetMenuModelCallback callback) {
  const ArcAppListPrefs* arc_list_prefs =
      ArcAppListPrefs::Get(controller()->profile());
  DCHECK(arc_list_prefs);

  const arc::ArcAppShelfId& app_id =
      arc::ArcAppShelfId::FromString(item().id.app_id);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      arc_list_prefs->GetApp(app_id.app_id());
  if (!app_info && !app_id.has_shelf_group_id()) {
    NOTREACHED();
    std::move(callback).Run(std::move(menu_model));
    return;
  }

  const bool app_is_open = controller()->IsOpen(item().id);
  if (!app_is_open) {
    DCHECK(app_info->launchable);
    AddContextMenuOption(menu_model.get(), MENU_OPEN_NEW,
                         IDS_APP_CONTEXT_MENU_ACTIVATE_ARC);
    if (!features::IsTouchableAppContextMenuEnabled())
      menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  }

  if (!app_id.has_shelf_group_id() && app_info->launchable)
    AddPinMenu(menu_model.get());

  if (app_is_open) {
    AddContextMenuOption(menu_model.get(), MENU_CLOSE,
                         IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  }
  if (!features::IsTouchableAppContextMenuEnabled())
    menu_model->AddSeparator(ui::NORMAL_SEPARATOR);

  // App shortcuts from Android are shown on touchable context menu only.
  if (!features::IsTouchableAppContextMenuEnabled()) {
    std::move(callback).Run(std::move(menu_model));
    return;
  }

  DCHECK(!app_shortcuts_menu_builder_);
  app_shortcuts_menu_builder_ =
      std::make_unique<arc::ArcAppShortcutsMenuBuilder>(
          controller()->profile(), item().id.app_id, display_id(),
          LAUNCH_APP_SHORTCUT_FIRST, LAUNCH_APP_SHORTCUT_LAST);
  app_shortcuts_menu_builder_->BuildMenu(
      app_info->package_name, std::move(menu_model), std::move(callback));
}
