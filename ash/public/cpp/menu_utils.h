// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_MENU_UTILS_H_
#define ASH_PUBLIC_CPP_MENU_UTILS_H_

#include <memory>
#include <vector>

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/interfaces/menu.mojom.h"
#include "ui/base/models/simple_menu_model.h"

namespace ash {
namespace menu_utils {

using MenuItemList = std::vector<mojom::MenuItemPtr>;
using SubmenuList = std::vector<std::unique_ptr<ui::MenuModel>>;

// Gets a serialized list of mojo MenuItemPtr objects to transport a menu model.
// NOTE: This does not support button items, some separator types, sublabels,
// minor text, dynamic items, label fonts, accelerators, visibility, etc.
ASH_PUBLIC_EXPORT MenuItemList GetMojoMenuItemsFromModel(ui::MenuModel* model);

// Populates a simple menu model with a list of mojo menu items. This can be
// considered as an inverse operation of |GetMojoMenuItemsFromModel|.
ASH_PUBLIC_EXPORT void PopulateMenuFromMojoMenuItems(
    ui::SimpleMenuModel* model,
    ui::SimpleMenuModel::Delegate* delegate,
    const MenuItemList& items,
    SubmenuList* submenus);

// Finds a menu item by command id; returns a stub item if no match was found.
ASH_PUBLIC_EXPORT const mojom::MenuItemPtr& GetMenuItemByCommandId(
    const MenuItemList& items,
    int command_id);

}  // namespace menu_utils
}  // namespace ash

#endif  // ASH_PUBLIC_CPP_MENU_UTILS_H_
