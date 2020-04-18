// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_APP_WINDOW_LAUNCHER_ITEM_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_APP_WINDOW_LAUNCHER_ITEM_CONTROLLER_H_

#include <string>
#include <unordered_set>

#include "base/macros.h"
#include "chrome/browser/ui/ash/launcher/app_window_launcher_item_controller.h"

class ArcAppWindow;

// Shelf item delegate for ARC app windows.
class ArcAppWindowLauncherItemController
    : public AppWindowLauncherItemController {
 public:
  explicit ArcAppWindowLauncherItemController(const std::string& arc_app_id);

  ~ArcAppWindowLauncherItemController() override;

  // AppWindowLauncherItemController overrides:
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ash::ShelfLaunchSource source,
                    ItemSelectedCallback callback) override;

  void AddTaskId(int task_id);
  void RemoveTaskId(int task_id);
  bool HasAnyTasks() const;

 private:
  // Update the shelf item's icon for the active window.
  void UpdateIcon(ArcAppWindow* arc_app_window);

  std::unordered_set<int> task_ids_;

  DISALLOW_COPY_AND_ASSIGN(ArcAppWindowLauncherItemController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_APP_WINDOW_LAUNCHER_ITEM_CONTROLLER_H_
