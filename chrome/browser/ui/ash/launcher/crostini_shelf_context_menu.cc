// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/crostini_shelf_context_menu.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/shelf_item.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/ui_base_features.h"

CrostiniShelfContextMenu::CrostiniShelfContextMenu(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    int64_t display_id)
    : LauncherContextMenu(controller, item, display_id) {}

CrostiniShelfContextMenu::~CrostiniShelfContextMenu() = default;

void CrostiniShelfContextMenu::GetMenuModel(GetMenuModelCallback callback) {
  auto menu_model = std::make_unique<ui::SimpleMenuModel>(this);
  BuildMenu(menu_model.get());
  std::move(callback).Run(std::move(menu_model));
}

void CrostiniShelfContextMenu::BuildMenu(ui::SimpleMenuModel* menu_model) {
  const crostini::CrostiniRegistryService* registry_service =
      crostini::CrostiniRegistryServiceFactory::GetForProfile(
          controller()->profile());
  std::unique_ptr<crostini::CrostiniRegistryService::Registration>
      registration = registry_service->GetRegistration(item().id.app_id);
  if (registration)
    AddPinMenu(menu_model);

  menu_model->AddItemWithStringId(MENU_NEW_WINDOW, IDS_APP_LIST_NEW_WINDOW);

  if (controller()->IsOpen(item().id)) {
    menu_model->AddItemWithStringId(MENU_CLOSE,
                                    IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  } else {
    menu_model->AddItemWithStringId(MENU_OPEN_NEW,
                                    IDS_APP_CONTEXT_MENU_ACTIVATE_ARC);
  }

  if (!features::IsTouchableAppContextMenuEnabled())
    menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
}

void CrostiniShelfContextMenu::ExecuteCommand(int command_id, int event_flags) {
  if (ExecuteCommonCommand(command_id, event_flags))
    return;

  if (command_id == MENU_NEW_WINDOW) {
    LaunchCrostiniApp(controller()->profile(), item().id.app_id);
    return;
  }
  NOTREACHED();
}
