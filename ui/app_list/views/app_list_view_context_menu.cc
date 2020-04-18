// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_view_context_menu.h"

#include <utility>

#include "ash/public/cpp/menu_utils.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view.h"

namespace app_list {

AppListViewContextMenu::AppListViewContextMenu(Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
}

AppListViewContextMenu::~AppListViewContextMenu() = default;

bool AppListViewContextMenu::IsRunning() {
  return menu_runner_ && menu_runner_->IsRunning();
}

void AppListViewContextMenu::Build(
    std::vector<ash::mojom::MenuItemPtr> items,
    int32_t run_types,
    const base::RepeatingClosure& on_menu_closed_callback) {
  DCHECK(!items.empty() && !IsRunning());

  // Reset and populate the context menu model.
  menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  ash::menu_utils::PopulateMenuFromMojoMenuItems(menu_model_.get(), this, items,
                                                 &submenu_models_);
  menu_items_ = std::move(items);
  menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_model_.get(), run_types, on_menu_closed_callback);
}

void AppListViewContextMenu::Run(views::Widget* parent,
                                 views::MenuButton* button,
                                 const gfx::Rect& bounds,
                                 views::MenuAnchorPosition anchor,
                                 ui::MenuSourceType source_type) {
  DCHECK(menu_runner_);
  menu_runner_->RunMenuAt(parent, button, bounds, anchor, source_type);
}

void AppListViewContextMenu::Cancel() {
  if (menu_runner_)
    menu_runner_->Cancel();
}

void AppListViewContextMenu::Reset() {
  menu_runner_ = nullptr;
}

bool AppListViewContextMenu::IsCommandIdChecked(int command_id) const {
  return ash::menu_utils::GetMenuItemByCommandId(menu_items_, command_id)
      ->checked;
}

bool AppListViewContextMenu::IsCommandIdEnabled(int command_id) const {
  return ash::menu_utils::GetMenuItemByCommandId(menu_items_, command_id)
      ->enabled;
}

void AppListViewContextMenu::ExecuteCommand(int command_id, int event_flags) {
  delegate_->ExecuteCommand(command_id, event_flags);
}

}  // namespace app_list
