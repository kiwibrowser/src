// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_APP_LIST_VIEW_CONTEXT_MENU_H_
#define UI_APP_LIST_VIEWS_APP_LIST_VIEW_CONTEXT_MENU_H_

#include <memory>
#include <vector>

#include "ash/public/interfaces/menu.mojom.h"
#include "ui/app_list/app_list_export.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/controls/menu/menu_types.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class MenuButton;
class MenuRunner;
class Widget;
}  // namespace views

namespace app_list {

// A class wrapping context menu operations for app list views.
class APP_LIST_EXPORT AppListViewContextMenu
    : public ui::SimpleMenuModel::Delegate {
 public:
  // A delegate class of the context menu with implementation of menu behaviors,
  // which should be the view showing this context menu.
  class Delegate {
   public:
    virtual void ExecuteCommand(int command_id, int event_flags) {}
  };

  explicit AppListViewContextMenu(Delegate* delegate);
  ~AppListViewContextMenu() override;

  // Returns whether the context menu is running.
  bool IsRunning();

  // Builds the context menu model and runner.
  void Build(std::vector<ash::mojom::MenuItemPtr> items,
             int32_t run_types,
             const base::RepeatingClosure& on_menu_closed_callback =
                 base::RepeatingClosure());

  // Runs the menu.
  void Run(views::Widget* parent,
           views::MenuButton* button,
           const gfx::Rect& bounds,
           views::MenuAnchorPosition anchor,
           ui::MenuSourceType source_type);

  // Cancels the running menu.
  void Cancel();

  // Resets the menu runner.
  void Reset();

 private:
  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  std::unique_ptr<ui::SimpleMenuModel> menu_model_;
  std::vector<ash::mojom::MenuItemPtr> menu_items_;
  std::vector<std::unique_ptr<ui::MenuModel>> submenu_models_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  Delegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListViewContextMenu);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_APP_LIST_VIEW_CONTEXT_MENU_H_
