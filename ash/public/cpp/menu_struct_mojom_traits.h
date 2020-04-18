// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_MENU_STRUCT_MOJOM_TRAITS_H_
#define ASH_PUBLIC_CPP_MENU_STRUCT_MOJOM_TRAITS_H_

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/interfaces/menu.mojom-shared.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/models/menu_separator_types.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::MenuItemType, ui::MenuModel::ItemType> {
  static ash::mojom::MenuItemType ToMojom(ui::MenuModel::ItemType input) {
    switch (input) {
      case ui::MenuModel::TYPE_COMMAND:
        return ash::mojom::MenuItemType::COMMAND;
      case ui::MenuModel::TYPE_CHECK:
        return ash::mojom::MenuItemType::CHECK;
      case ui::MenuModel::TYPE_RADIO:
        return ash::mojom::MenuItemType::RADIO;
      case ui::MenuModel::TYPE_SEPARATOR:
        return ash::mojom::MenuItemType::SEPARATOR;
      case ui::MenuModel::TYPE_BUTTON_ITEM:
        NOTREACHED() << "TYPE_BUTTON_ITEM is not yet supported.";
        return ash::mojom::MenuItemType::COMMAND;
      case ui::MenuModel::TYPE_SUBMENU:
        return ash::mojom::MenuItemType::SUBMENU;
      case ui::MenuModel::TYPE_ACTIONABLE_SUBMENU:
        return ash::mojom::MenuItemType::ACTIONABLE_SUBMENU;
    }
    NOTREACHED();
    return ash::mojom::MenuItemType::COMMAND;
  }

  static bool FromMojom(ash::mojom::MenuItemType input,
                        ui::MenuModel::ItemType* out) {
    switch (input) {
      case ash::mojom::MenuItemType::COMMAND:
        *out = ui::MenuModel::TYPE_COMMAND;
        return true;
      case ash::mojom::MenuItemType::CHECK:
        *out = ui::MenuModel::TYPE_CHECK;
        return true;
      case ash::mojom::MenuItemType::RADIO:
        *out = ui::MenuModel::TYPE_RADIO;
        return true;
      case ash::mojom::MenuItemType::SEPARATOR:
        *out = ui::MenuModel::TYPE_SEPARATOR;
        return true;
      case ash::mojom::MenuItemType::SUBMENU:
        *out = ui::MenuModel::TYPE_SUBMENU;
        return true;
      case ash::mojom::MenuItemType::ACTIONABLE_SUBMENU:
        *out = ui::MenuModel::TYPE_ACTIONABLE_SUBMENU;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<ash::mojom::MenuSeparatorType, ui::MenuSeparatorType> {
  static ash::mojom::MenuSeparatorType ToMojom(ui::MenuSeparatorType input) {
    switch (input) {
      case ui::MenuSeparatorType::NORMAL_SEPARATOR:
        return ash::mojom::MenuSeparatorType::NORMAL_SEPARATOR;
      case ui::MenuSeparatorType::DOUBLE_SEPARATOR:
        return ash::mojom::MenuSeparatorType::DOUBLE_SEPARATOR;
      case ui::MenuSeparatorType::UPPER_SEPARATOR:
        return ash::mojom::MenuSeparatorType::UPPER_SEPARATOR;
      case ui::MenuSeparatorType::LOWER_SEPARATOR:
        return ash::mojom::MenuSeparatorType::LOWER_SEPARATOR;
      case ui::MenuSeparatorType::SPACING_SEPARATOR:
        return ash::mojom::MenuSeparatorType::SPACING_SEPARATOR;
      case ui::MenuSeparatorType::VERTICAL_SEPARATOR:
        return ash::mojom::MenuSeparatorType::VERTICAL_SEPARATOR;
      case ui::MenuSeparatorType::PADDED_SEPARATOR:
        return ash::mojom::MenuSeparatorType::PADDED_SEPARATOR;
    }
    NOTREACHED();
    return ash::mojom::MenuSeparatorType::NORMAL_SEPARATOR;
  }

  static bool FromMojom(ash::mojom::MenuSeparatorType input,
                        ui::MenuSeparatorType* out) {
    switch (input) {
      case ash::mojom::MenuSeparatorType::NORMAL_SEPARATOR:
        *out = ui::MenuSeparatorType::NORMAL_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::DOUBLE_SEPARATOR:
        *out = ui::MenuSeparatorType::DOUBLE_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::UPPER_SEPARATOR:
        *out = ui::MenuSeparatorType::UPPER_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::LOWER_SEPARATOR:
        *out = ui::MenuSeparatorType::LOWER_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::SPACING_SEPARATOR:
        *out = ui::MenuSeparatorType::SPACING_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::VERTICAL_SEPARATOR:
        *out = ui::MenuSeparatorType::VERTICAL_SEPARATOR;
        return true;
      case ash::mojom::MenuSeparatorType::PADDED_SEPARATOR:
        *out = ui::MenuSeparatorType::PADDED_SEPARATOR;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_MENU_STRUCT_MOJOM_TRAITS_H_
