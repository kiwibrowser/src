// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_context_menu.h"

#include <utility>

#include "base/bind.h"
#include "chrome/browser/chromeos/arc/app_shortcuts/arc_app_shortcuts_menu_builder.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_context_menu_delegate.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/arc/arc_app_dialog.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/arc_app_window_launcher_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/ui_base_features.h"

ArcAppContextMenu::ArcAppContextMenu(app_list::AppContextMenuDelegate* delegate,
                                     Profile* profile,
                                     const std::string& app_id,
                                     AppListControllerDelegate* controller)
    : app_list::AppContextMenu(delegate, profile, app_id, controller) {}

ArcAppContextMenu::~ArcAppContextMenu() = default;

void ArcAppContextMenu::GetMenuModel(GetMenuModelCallback callback) {
  auto menu_model = std::make_unique<ui::SimpleMenuModel>(this);
  menu_model->set_histogram_name("Apps.ContextMenuExecuteCommand.FromApp");
  BuildMenu(menu_model.get());
  if (!features::IsTouchableAppContextMenuEnabled()) {
    std::move(callback).Run(std::move(menu_model));
    return;
  }
  BuildAppShortcutsMenu(std::move(menu_model), std::move(callback));
}

void ArcAppContextMenu::BuildMenu(ui::SimpleMenuModel* menu_model) {
  const ArcAppListPrefs* arc_prefs = ArcAppListPrefs::Get(profile());
  DCHECK(arc_prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      arc_prefs->GetApp(app_id());
  if (!app_info) {
    LOG(ERROR) << "App " << app_id() << " is not available.";
    return;
  }

  if (!controller()->IsAppOpen(app_id())) {
    AddContextMenuOption(menu_model, LAUNCH_NEW,
                         IDS_APP_CONTEXT_MENU_ACTIVATE_ARC);
    if (!features::IsTouchableAppContextMenuEnabled())
      menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  }
  // Create default items.
  app_list::AppContextMenu::BuildMenu(menu_model);

  if (!features::IsTouchableAppContextMenuEnabled())
    menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  if (arc_prefs->IsShortcut(app_id()))
    AddContextMenuOption(menu_model, UNINSTALL, IDS_APP_LIST_REMOVE_SHORTCUT);
  else if (!app_info->sticky)
    AddContextMenuOption(menu_model, UNINSTALL, IDS_APP_LIST_UNINSTALL_ITEM);

  // App Info item.
  AddContextMenuOption(menu_model, SHOW_APP_INFO,
                       IDS_APP_CONTEXT_MENU_SHOW_INFO);
}

bool ArcAppContextMenu::IsCommandIdEnabled(int command_id) const {
  const ArcAppListPrefs* arc_prefs = ArcAppListPrefs::Get(profile());
  DCHECK(arc_prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      arc_prefs->GetApp(app_id());

  switch (command_id) {
    case UNINSTALL:
      return app_info &&
          !app_info->sticky &&
          (app_info->ready || app_info->shortcut);
    case SHOW_APP_INFO:
      return app_info && app_info->ready;
    default:
      return app_list::AppContextMenu::IsCommandIdEnabled(command_id);
  }

  return false;
}

void ArcAppContextMenu::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == LAUNCH_NEW) {
    delegate()->ExecuteLaunchCommand(event_flags);
  } else if (command_id == UNINSTALL) {
    arc::ShowArcAppUninstallDialog(profile(), controller(), app_id());
  } else if (command_id == SHOW_APP_INFO) {
    ShowPackageInfo();
  } else if (command_id >= LAUNCH_APP_SHORTCUT_FIRST &&
             command_id <= LAUNCH_APP_SHORTCUT_LAST) {
    DCHECK(app_shortcuts_menu_builder_);
    app_shortcuts_menu_builder_->ExecuteCommand(command_id);
  } else {
    app_list::AppContextMenu::ExecuteCommand(command_id, event_flags);
  }
}

void ArcAppContextMenu::BuildAppShortcutsMenu(
    std::unique_ptr<ui::SimpleMenuModel> menu_model,
    GetMenuModelCallback callback) {
  const ArcAppListPrefs* arc_prefs = ArcAppListPrefs::Get(profile());
  DCHECK(arc_prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      arc_prefs->GetApp(app_id());
  if (!app_info) {
    LOG(ERROR) << "App " << app_id() << " is not available.";
    std::move(callback).Run(std::move(menu_model));
    return;
  }

  DCHECK(!app_shortcuts_menu_builder_);
  app_shortcuts_menu_builder_ =
      std::make_unique<arc::ArcAppShortcutsMenuBuilder>(
          profile(), app_id(), controller()->GetAppListDisplayId(),
          LAUNCH_APP_SHORTCUT_FIRST, LAUNCH_APP_SHORTCUT_LAST);
  app_shortcuts_menu_builder_->BuildMenu(
      app_info->package_name, std::move(menu_model), std::move(callback));
}

void ArcAppContextMenu::ShowPackageInfo() {
  const ArcAppListPrefs* arc_prefs = ArcAppListPrefs::Get(profile());
  DCHECK(arc_prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      arc_prefs->GetApp(app_id());
  if (!app_info) {
    VLOG(2) << "Requesting AppInfo for package that does not exist: "
            << app_id() << ".";
    return;
  }
  if (arc::ShowPackageInfo(app_info->package_name,
                           arc::mojom::ShowPackageInfoPage::MAIN,
                           controller()->GetAppListDisplayId()) &&
      !controller()->IsHomeLauncherEnabledInTabletMode()) {
    controller()->DismissView();
  }
}
