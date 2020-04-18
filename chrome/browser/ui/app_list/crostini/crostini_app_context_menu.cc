// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/crostini/crostini_app_context_menu.h"

#include "chrome/browser/chromeos/crostini/crostini_manager.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/ui_base_features.h"

CrostiniAppContextMenu::CrostiniAppContextMenu(
    Profile* profile,
    const std::string& app_id,
    AppListControllerDelegate* controller)
    : app_list::AppContextMenu(nullptr, profile, app_id, controller) {}

CrostiniAppContextMenu::~CrostiniAppContextMenu() = default;

// TODO(timloh): Add support for "App Info", "Uninstall", and possibly actions
// defined in .desktop files.
void CrostiniAppContextMenu::BuildMenu(ui::SimpleMenuModel* menu_model) {
  app_list::AppContextMenu::BuildMenu(menu_model);

  if (app_id() == kCrostiniTerminalId) {
    if (!features::IsTouchableAppContextMenuEnabled())
      menu_model->AddSeparator(ui::NORMAL_SEPARATOR);

    AddContextMenuOption(menu_model, UNINSTALL, IDS_APP_LIST_UNINSTALL_ITEM);
  }
}

bool CrostiniAppContextMenu::IsCommandIdEnabled(int command_id) const {
  if (command_id == UNINSTALL) {
    if (app_id() == kCrostiniTerminalId) {
      return IsCrostiniEnabled(profile());
    }
  }
  return app_list::AppContextMenu::IsCommandIdEnabled(command_id);
}

void CrostiniAppContextMenu::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case UNINSTALL:
      if (app_id() == kCrostiniTerminalId) {
        ShowCrostiniUninstallerView(profile());
        return;
      }
      break;
  }
  app_list::AppContextMenu::ExecuteCommand(command_id, event_flags);
}
