// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/menu_utils.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "ui/gfx/image/image.h"

namespace ash {
namespace menu_utils {

MenuItemList GetMojoMenuItemsFromModel(ui::MenuModel* model) {
  MenuItemList items;
  if (!model)
    return items;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    mojom::MenuItemPtr item(mojom::MenuItem::New());
    DCHECK_NE(ui::MenuModel::TYPE_BUTTON_ITEM, model->GetTypeAt(i));
    item->type = model->GetTypeAt(i);
    item->command_id = model->GetCommandIdAt(i);
    item->label = model->GetLabelAt(i);
    item->checked = model->IsItemCheckedAt(i);
    item->enabled = model->IsEnabledAt(i);
    item->radio_group_id = model->GetGroupIdAt(i);
    if (item->type == ui::MenuModel::TYPE_SUBMENU ||
        item->type == ui::MenuModel::TYPE_ACTIONABLE_SUBMENU) {
      item->submenu = GetMojoMenuItemsFromModel(model->GetSubmenuModelAt(i));
    }
    item->separator_type = model->GetSeparatorTypeAt(i);
    gfx::Image icon;
    if (model->GetIconAt(i, &icon))
      item->image = icon.AsImageSkia();
    items.push_back(std::move(item));
  }
  return items;
}

void PopulateMenuFromMojoMenuItems(ui::SimpleMenuModel* model,
                                   ui::SimpleMenuModel::Delegate* delegate,
                                   const MenuItemList& items,
                                   SubmenuList* submenus) {
  for (const mojom::MenuItemPtr& item : items) {
    switch (item->type) {
      case ui::MenuModel::TYPE_COMMAND:
        model->AddItem(item->command_id, item->label);
        break;
      case ui::MenuModel::TYPE_CHECK:
        model->AddCheckItem(item->command_id, item->label);
        break;
      case ui::MenuModel::TYPE_RADIO:
        model->AddRadioItem(item->command_id, item->label,
                            item->radio_group_id);
        break;
      case ui::MenuModel::TYPE_SEPARATOR:
        model->AddSeparator(item->separator_type);
        break;
      case ui::MenuModel::TYPE_BUTTON_ITEM:
        NOTREACHED() << "TYPE_BUTTON_ITEM is not yet supported.";
        break;
      case ui::MenuModel::TYPE_SUBMENU:
      case ui::MenuModel::TYPE_ACTIONABLE_SUBMENU:
        if (item->submenu.has_value()) {
          std::unique_ptr<ui::SimpleMenuModel> submenu =
              std::make_unique<ui::SimpleMenuModel>(delegate);
          PopulateMenuFromMojoMenuItems(submenu.get(), delegate,
                                        item->submenu.value(), submenus);
          if (item->type == ui::MenuModel::TYPE_SUBMENU) {
            model->AddSubMenu(item->command_id, item->label, submenu.get());
          } else {
            model->AddActionableSubMenu(item->command_id, item->label,
                                        submenu.get());
          }
          submenus->push_back(std::move(submenu));
        }
        break;
    }
    if (!item->image.isNull()) {
      model->SetIcon(model->GetIndexOfCommandId(item->command_id),
                     gfx::Image(item->image));
    }
  }
}

const mojom::MenuItemPtr& GetMenuItemByCommandId(const MenuItemList& items,
                                                 int command_id) {
  static const base::NoDestructor<mojom::MenuItemPtr> item_not_found(
      mojom::MenuItem::New());
  for (const mojom::MenuItemPtr& item : items) {
    if (item->command_id == command_id)
      return item;
    if ((item->type == ui::MenuModel::TYPE_SUBMENU ||
         (item->type == ui::MenuModel::TYPE_ACTIONABLE_SUBMENU)) &&
        item->submenu.has_value()) {
      const mojom::MenuItemPtr& submenu_item =
          GetMenuItemByCommandId(item->submenu.value(), command_id);
      if (submenu_item->command_id == command_id)
        return submenu_item;
    }
  }
  return *item_not_found;
}

}  // namespace menu_utils
}  // namespace ash
