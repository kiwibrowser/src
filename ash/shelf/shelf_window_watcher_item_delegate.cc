// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_window_watcher_item_delegate.h"

#include <utility>

#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/shelf/shelf_context_menu_model.h"
#include "ash/shelf/shelf_controller.h"
#include "ash/shell.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "components/strings/grit/components_strings.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/events/event_constants.h"
#include "ui/wm/core/window_animations.h"

namespace ash {

namespace {

// Close command id; avoids colliding with ShelfContextMenuModel command ids.
const int kCloseCommandId = ShelfContextMenuModel::MENU_LOCAL_END + 1;

ShelfItemType GetShelfItemType(const ShelfID& id) {
  ShelfModel* model = Shell::Get()->shelf_controller()->model();
  ShelfItems::const_iterator item = model->ItemByID(id);
  return item == model->items().end() ? TYPE_UNDEFINED : item->type;
}

}  // namespace

ShelfWindowWatcherItemDelegate::ShelfWindowWatcherItemDelegate(
    const ShelfID& id,
    aura::Window* window)
    : ShelfItemDelegate(id), window_(window) {
  DCHECK(!id.IsNull());
  DCHECK(window_);
}

ShelfWindowWatcherItemDelegate::~ShelfWindowWatcherItemDelegate() = default;

void ShelfWindowWatcherItemDelegate::ItemSelected(
    std::unique_ptr<ui::Event> event,
    int64_t display_id,
    ShelfLaunchSource source,
    ItemSelectedCallback callback) {
  // Move panels attached on another display to the current display.
  if (GetShelfItemType(shelf_id()) == TYPE_APP_PANEL &&
      window_->GetProperty(kPanelAttachedKey) &&
      wm::MoveWindowToDisplay(window_, display_id)) {
    wm::ActivateWindow(window_);
    std::move(callback).Run(SHELF_ACTION_WINDOW_ACTIVATED, base::nullopt);
    return;
  }

  if (wm::IsActiveWindow(window_)) {
    if (event && event->type() == ui::ET_KEY_RELEASED) {
      ::wm::AnimateWindow(window_, ::wm::WINDOW_ANIMATION_TYPE_BOUNCE);
      std::move(callback).Run(SHELF_ACTION_NONE, base::nullopt);
      return;
    }
    window_->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
    std::move(callback).Run(SHELF_ACTION_WINDOW_MINIMIZED, base::nullopt);
    return;
  }
  wm::ActivateWindow(window_);
  std::move(callback).Run(SHELF_ACTION_WINDOW_ACTIVATED, base::nullopt);
}

void ShelfWindowWatcherItemDelegate::GetContextMenuItems(
    int64_t display_id,
    GetContextMenuItemsCallback callback) {
  // Show a default context menu with just an extra close item and separator.
  ash::MenuItemList items;
  ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
  close->type = ui::MenuModel::TYPE_COMMAND;
  close->command_id = kCloseCommandId;
  close->label = l10n_util::GetStringUTF16(IDS_CLOSE);
  close->enabled = true;
  items.push_back(std::move(close));
  if (!features::IsTouchableAppContextMenuEnabled()) {
    ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
    separator->type = ui::MenuModel::TYPE_SEPARATOR;
    items.push_back(std::move(separator));
  }
  std::move(callback).Run(std::move(items));
}

void ShelfWindowWatcherItemDelegate::ExecuteCommand(bool from_context_menu,
                                                    int64_t command_id,
                                                    int32_t event_flags,
                                                    int64_t display_id) {
  DCHECK_EQ(command_id, kCloseCommandId) << "Unknown ShelfItemDelegate command";
  Close();
}

void ShelfWindowWatcherItemDelegate::Close() {
  wm::CloseWidgetForWindow(window_);
}

}  // namespace ash
