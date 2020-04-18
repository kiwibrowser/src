// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/arc_app_window_launcher_item_controller.h"

#include <utility>

#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/launcher_controller_helper.h"
#include "ui/aura/window.h"
#include "ui/base/base_window.h"

ArcAppWindowLauncherItemController::ArcAppWindowLauncherItemController(
    const std::string& arc_app_id)
    : AppWindowLauncherItemController(ash::ShelfID(arc_app_id)) {}

ArcAppWindowLauncherItemController::~ArcAppWindowLauncherItemController() {}

void ArcAppWindowLauncherItemController::AddTaskId(int task_id) {
  task_ids_.insert(task_id);
}

void ArcAppWindowLauncherItemController::RemoveTaskId(int task_id) {
  task_ids_.erase(task_id);
}

bool ArcAppWindowLauncherItemController::HasAnyTasks() const {
  return !task_ids_.empty();
}

void ArcAppWindowLauncherItemController::ItemSelected(
    std::unique_ptr<ui::Event> event,
    int64_t display_id,
    ash::ShelfLaunchSource source,
    ItemSelectedCallback callback) {
  if (window_count()) {
    AppWindowLauncherItemController::ItemSelected(std::move(event), display_id,
                                                  source, std::move(callback));
    return;
  }

  if (task_ids_.empty()) {
    NOTREACHED();
    std::move(callback).Run(ash::SHELF_ACTION_NONE, base::nullopt);
    return;
  }
  arc::SetTaskActive(*task_ids_.begin());
  std::move(callback).Run(ash::SHELF_ACTION_NEW_WINDOW_CREATED, base::nullopt);
}
