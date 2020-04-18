// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/shelf_spinner_item_controller.h"

#include <utility>

#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/launcher_context_menu.h"
#include "chrome/browser/ui/ash/launcher/shelf_spinner_controller.h"

ShelfSpinnerItemController::ShelfSpinnerItemController(
    const std::string& app_id)
    : ash::ShelfItemDelegate(ash::ShelfID(app_id)),
      start_time_(base::Time::Now()) {}

ShelfSpinnerItemController::~ShelfSpinnerItemController() {
  if (host_)
    host_->Remove(app_id());
}

void ShelfSpinnerItemController::SetHost(
    const base::WeakPtr<ShelfSpinnerController>& host) {
  DCHECK(!host_);
  host_ = host;
}

base::TimeDelta ShelfSpinnerItemController::GetActiveTime() const {
  return base::Time::Now() - start_time_;
}

void ShelfSpinnerItemController::ItemSelected(std::unique_ptr<ui::Event> event,
                                              int64_t display_id,
                                              ash::ShelfLaunchSource source,
                                              ItemSelectedCallback callback) {
  std::move(callback).Run(ash::SHELF_ACTION_NONE, base::nullopt);
}

void ShelfSpinnerItemController::ExecuteCommand(bool from_context_menu,
                                                int64_t command_id,
                                                int32_t event_flags,
                                                int64_t display_id) {
  if (from_context_menu && ExecuteContextMenuCommand(command_id, event_flags))
    return;

  NOTIMPLEMENTED();
}

void ShelfSpinnerItemController::GetContextMenu(int64_t display_id,
                                                GetMenuModelCallback callback) {
  ChromeLauncherController* controller = ChromeLauncherController::instance();
  const ash::ShelfItem* item = controller->GetItem(shelf_id());
  context_menu_ = LauncherContextMenu::Create(controller, item, display_id);
  context_menu_->GetMenuModel(std::move(callback));
}

void ShelfSpinnerItemController::Close() {
  if (host_)
    host_->Close(app_id());
}
