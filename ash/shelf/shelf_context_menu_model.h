// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_
#define ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"

namespace ash {

class ShelfItemDelegate;

// A context menu shown for shelf items, the shelf itself, or the desktop area.
class ASH_EXPORT ShelfContextMenuModel : public ui::SimpleMenuModel,
                                         public ui::SimpleMenuModel::Delegate {
 public:
  // The command ids for locally-handled shelf and wallpaper context menu items.
  // These are used in histograms, do not remove/renumber entries. Only add at
  // the end just before MENU_LOCAL_END. If you're adding to this enum with the
  // intention that it will be logged, add checks to ensure stability of the
  // enum and update the ChromeOSUICommands enum listing in
  // tools/metrics/histograms/enums.xml.
  enum CommandId {
    MENU_LOCAL_START = 500,  // Offset to avoid conflicts with other menus.
    MENU_AUTO_HIDE = MENU_LOCAL_START,
    MENU_ALIGNMENT_MENU = 501,
    MENU_ALIGNMENT_LEFT = 502,
    MENU_ALIGNMENT_RIGHT = 503,
    MENU_ALIGNMENT_BOTTOM = 504,
    MENU_CHANGE_WALLPAPER = 505,
    MENU_LOCAL_END
  };

  // Creates a context menu model with |menu_items| from the given |delegate|.
  // This menu appends and handles additional shelf and wallpaper menu items.
  ShelfContextMenuModel(std::vector<mojom::MenuItemPtr> menu_items,
                        ShelfItemDelegate* delegate,
                        int64_t display_id);
  ~ShelfContextMenuModel() override;

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  std::vector<mojom::MenuItemPtr> menu_items_;
  ShelfItemDelegate* delegate_;
  std::vector<std::unique_ptr<ui::MenuModel>> submenus_;
  const int64_t display_id_;

  DISALLOW_COPY_AND_ASSIGN(ShelfContextMenuModel);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_
