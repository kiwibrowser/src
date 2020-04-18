// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_window_watcher.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_window_watcher_item_delegate.h"
#include "ash/shell.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/strings/string_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/wm/public/activation_client.h"

namespace ash {
namespace {

// Returns the window's shelf item type property value.
// Mash returns the dialog type for normal windows without shelf item types.
// TODO(msw): Extend this Mash behavior to all Ash configs.
ShelfItemType GetShelfItemType(aura::Window* window) {
  if (Shell::GetAshConfig() == Config::MASH &&
      window->GetProperty(kShelfItemTypeKey) == TYPE_UNDEFINED &&
      window->type() == aura::client::WINDOW_TYPE_NORMAL &&
      !wm::GetWindowState(window)->ignored_by_shelf()) {
    return TYPE_DIALOG;
  }
  return static_cast<ShelfItemType>(window->GetProperty(kShelfItemTypeKey));
}

// Returns the window's shelf id property value, or provides a default value.
// Mash sets and returns an initial default shelf id for unidentified windows.
// TODO(msw): Extend this Mash behavior to all Ash configs.
ShelfID GetShelfID(aura::Window* window) {
  if (Shell::GetAshConfig() == Config::MASH &&
      !window->GetProperty(kShelfIDKey) &&
      !wm::GetWindowState(window)->ignored_by_shelf()) {
    static int id = 0;
    const ash::ShelfID shelf_id("ShelfWindowWatcher" + std::to_string(id++));
    window->SetProperty(kShelfIDKey, new std::string(shelf_id.Serialize()));
    return shelf_id;
  }
  return ShelfID::Deserialize(window->GetProperty(kShelfIDKey));
}

// Update the ShelfItem from relevant window properties.
void UpdateShelfItemForWindow(ShelfItem* item, aura::Window* window) {
  DCHECK(item->id.IsNull() || item->id == GetShelfID(window));
  item->id = GetShelfID(window);
  item->type = GetShelfItemType(window);
  item->title = window->GetTitle();

  // Active windows don't draw attention because the user is looking at them.
  if (window->GetProperty(aura::client::kDrawAttentionKey) &&
      !wm::IsActiveWindow(window)) {
    item->status = STATUS_ATTENTION;
  } else {
    item->status = STATUS_RUNNING;
  }

  // Prefer app icons over window icons, they're typically larger.
  gfx::ImageSkia* image = window->GetProperty(aura::client::kAppIconKey);
  if (!image || image->isNull())
    image = window->GetProperty(aura::client::kWindowIconKey);
  if (!image || image->isNull()) {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    item->image = rb.GetImageNamed(IDR_DEFAULT_FAVICON_32).AsImageSkia();
  } else {
    item->image = *image;
  }

  // Do not show tooltips for visible attached app panel windows.
  item->shows_tooltip = item->type != TYPE_APP_PANEL || !window->IsVisible() ||
                        !window->GetProperty(kPanelAttachedKey);
}

}  // namespace

ShelfWindowWatcher::ContainerWindowObserver::ContainerWindowObserver(
    ShelfWindowWatcher* window_watcher)
    : window_watcher_(window_watcher) {}

ShelfWindowWatcher::ContainerWindowObserver::~ContainerWindowObserver() =
    default;

void ShelfWindowWatcher::ContainerWindowObserver::OnWindowHierarchyChanged(
    const HierarchyChangeParams& params) {
  if (!params.old_parent && params.new_parent &&
      (params.new_parent->id() == kShellWindowId_DefaultContainer ||
       params.new_parent->id() == kShellWindowId_PanelContainer)) {
    // A new window was created in the default container or the panel container.
    window_watcher_->OnUserWindowAdded(params.target);
  }
}

void ShelfWindowWatcher::ContainerWindowObserver::OnWindowDestroying(
    aura::Window* window) {
  window_watcher_->OnContainerWindowDestroying(window);
}

////////////////////////////////////////////////////////////////////////////////

ShelfWindowWatcher::UserWindowObserver::UserWindowObserver(
    ShelfWindowWatcher* window_watcher)
    : window_watcher_(window_watcher) {}

ShelfWindowWatcher::UserWindowObserver::~UserWindowObserver() = default;

void ShelfWindowWatcher::UserWindowObserver::OnWindowPropertyChanged(
    aura::Window* window,
    const void* key,
    intptr_t old) {
  // ShelfIDs should never change except when replacing Mash temporary defaults.
  // TODO(msw): Extend this Mash behavior to all Ash configs.
  if (Shell::GetAshConfig() == Config::MASH && key == kShelfIDKey &&
      window_watcher_->user_windows_with_items_.count(window) > 0) {
    ShelfID old_id = ShelfID::Deserialize(reinterpret_cast<std::string*>(old));
    ShelfID new_id = ShelfID::Deserialize(window->GetProperty(kShelfIDKey));
    if (old_id != new_id && !old_id.IsNull() && !new_id.IsNull() &&
        window_watcher_->model_->ItemIndexByID(old_id) >= 0) {
      // Id changing is not supported; remove the item and it will be re-added.
      window_watcher_->user_windows_with_items_.erase(window);
      const int index = window_watcher_->model_->ItemIndexByID(old_id);
      window_watcher_->model_->RemoveItemAt(index);
    }
  }

  if (key == aura::client::kAppIconKey || key == aura::client::kWindowIconKey ||
      key == aura::client::kDrawAttentionKey || key == kPanelAttachedKey ||
      key == kShelfItemTypeKey || key == kShelfIDKey) {
    window_watcher_->OnUserWindowPropertyChanged(window);
  }
}

void ShelfWindowWatcher::UserWindowObserver::OnWindowDestroying(
    aura::Window* window) {
  window_watcher_->OnUserWindowDestroying(window);
}

void ShelfWindowWatcher::UserWindowObserver::OnWindowVisibilityChanged(
    aura::Window* window,
    bool visible) {
  // This is also called for descendants; check that the window is observed.
  if (window_watcher_->observed_user_windows_.IsObserving(window))
    window_watcher_->OnUserWindowPropertyChanged(window);
}

void ShelfWindowWatcher::UserWindowObserver::OnWindowTitleChanged(
    aura::Window* window) {
  window_watcher_->OnUserWindowPropertyChanged(window);
}

////////////////////////////////////////////////////////////////////////////////

ShelfWindowWatcher::ShelfWindowWatcher(ShelfModel* model)
    : model_(model),
      container_window_observer_(this),
      user_window_observer_(this),
      observed_container_windows_(&container_window_observer_),
      observed_user_windows_(&user_window_observer_) {
  Shell::Get()->activation_client()->AddObserver(this);
  Shell::Get()->AddShellObserver(this);
  for (aura::Window* window : Shell::GetAllRootWindows())
    OnRootWindowAdded(window);
}

ShelfWindowWatcher::~ShelfWindowWatcher() {
  Shell::Get()->RemoveShellObserver(this);
  Shell::Get()->activation_client()->RemoveObserver(this);
}

void ShelfWindowWatcher::AddShelfItem(aura::Window* window) {
  user_windows_with_items_.insert(window);
  ShelfItem item;
  UpdateShelfItemForWindow(&item, window);

  model_->SetShelfItemDelegate(
      item.id,
      std::make_unique<ShelfWindowWatcherItemDelegate>(item.id, window));

  // Panels are inserted on the left so as not to push all existing panels over.
  model_->AddAt(item.type == TYPE_APP_PANEL ? 0 : model_->item_count(), item);
}

void ShelfWindowWatcher::RemoveShelfItem(aura::Window* window) {
  user_windows_with_items_.erase(window);
  const ShelfID shelf_id = GetShelfID(window);
  DCHECK(!shelf_id.IsNull());
  const int index = model_->ItemIndexByID(shelf_id);
  DCHECK_GE(index, 0);
  model_->RemoveItemAt(index);
}

void ShelfWindowWatcher::OnContainerWindowDestroying(aura::Window* container) {
  observed_container_windows_.Remove(container);
}

void ShelfWindowWatcher::OnUserWindowAdded(aura::Window* window) {
  // The window may already be tracked from a prior display or parent container.
  if (observed_user_windows_.IsObserving(window))
    return;

  observed_user_windows_.Add(window);

  // Add, update, or remove a ShelfItem for |window|, as needed.
  OnUserWindowPropertyChanged(window);
}

void ShelfWindowWatcher::OnUserWindowDestroying(aura::Window* window) {
  if (observed_user_windows_.IsObserving(window))
    observed_user_windows_.Remove(window);

  if (user_windows_with_items_.count(window) > 0)
    RemoveShelfItem(window);
  DCHECK_EQ(0u, user_windows_with_items_.count(window));
}

void ShelfWindowWatcher::OnUserWindowPropertyChanged(aura::Window* window) {
  // ShelfWindowWatcher only handles panels and dialogs for now, all other shelf
  // item types are handled by ChromeLauncherController.
  const ShelfItemType item_type = GetShelfItemType(window);
  if ((item_type != TYPE_APP_PANEL && item_type != TYPE_DIALOG) ||
      GetShelfID(window).IsNull()) {
    // Remove |window|'s ShelfItem if it was added by ShelfWindowWatcher.
    if (user_windows_with_items_.count(window) > 0)
      RemoveShelfItem(window);
    return;
  }

  // Update an existing ShelfWindowWatcher item when a window property changes.
  int index = model_->ItemIndexByID(GetShelfID(window));
  if (index > 0 && user_windows_with_items_.count(window) > 0) {
    ShelfItem item = model_->items()[index];
    UpdateShelfItemForWindow(&item, window);
    model_->Set(index, item);
    return;
  }

  // Create a new item for |window|, if it is visible or a [minimized] panel.
  if (index < 0 && (window->IsVisible() || item_type == TYPE_APP_PANEL))
    AddShelfItem(window);
}

void ShelfWindowWatcher::OnWindowActivated(ActivationReason reason,
                                           aura::Window* gained_active,
                                           aura::Window* lost_active) {
  if (gained_active && user_windows_with_items_.count(gained_active) > 0)
    OnUserWindowPropertyChanged(gained_active);
  if (lost_active && user_windows_with_items_.count(lost_active) > 0)
    OnUserWindowPropertyChanged(lost_active);
}

void ShelfWindowWatcher::OnRootWindowAdded(aura::Window* root_window) {
  constexpr int container_ids[] = {kShellWindowId_DefaultContainer,
                                   kShellWindowId_PanelContainer};
  for (const int container_id : container_ids) {
    aura::Window* container = root_window->GetChildById(container_id);
    for (aura::Window* window : container->children())
      OnUserWindowAdded(window);
    observed_container_windows_.Add(container);
  }
}

}  // namespace ash
