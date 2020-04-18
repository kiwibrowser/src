// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_item_delegate.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/menu_utils.h"
#include "base/bind.h"
#include "ui/base/models/menu_model.h"

namespace ash {

ShelfItemDelegate::ShelfItemDelegate(const ShelfID& shelf_id)
    : shelf_id_(shelf_id), binding_(this), weak_ptr_factory_(this) {}

ShelfItemDelegate::~ShelfItemDelegate() = default;

mojom::ShelfItemDelegatePtr ShelfItemDelegate::CreateInterfacePtrAndBind() {
  mojom::ShelfItemDelegatePtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

MenuItemList ShelfItemDelegate::GetAppMenuItems(int event_flags) {
  return MenuItemList();
}

void ShelfItemDelegate::GetContextMenu(int64_t display_id,
                                       GetMenuModelCallback callback) {
  // Shelf items do not have any custom context menu entries by default.
  std::move(callback).Run(nullptr);
}

AppWindowLauncherItemController*
ShelfItemDelegate::AsAppWindowLauncherItemController() {
  return nullptr;
}

bool ShelfItemDelegate::ExecuteContextMenuCommand(int64_t command_id,
                                                  int32_t event_flags) {
  DCHECK(context_menu_);
  // Help subclasses execute context menu items, which may be on a sub-menu.
  ui::MenuModel* model = context_menu_.get();
  int index = -1;
  if (!ui::MenuModel::GetModelAndIndexForCommandId(command_id, &model, &index))
    return false;

  model->ActivatedAt(index, event_flags);
  return true;
}

void ShelfItemDelegate::GetContextMenuItems(
    int64_t display_id,
    GetContextMenuItemsCallback callback) {
  GetContextMenu(
      display_id,
      base::BindOnce(&ShelfItemDelegate::OnGetContextMenu,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ShelfItemDelegate::OnGetContextMenu(
    GetContextMenuItemsCallback callback,
    std::unique_ptr<ui::MenuModel> menu_model) {
  context_menu_ = std::move(menu_model);
  std::move(callback).Run(
      menu_utils::GetMojoMenuItemsFromModel(context_menu_.get()));
}

}  // namespace ash
