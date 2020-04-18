// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/ui/ash/launcher/internal_app_window_shelf_controller.h"

#include "ash/public/cpp/app_list/internal_app_id_constants.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/window_properties.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"
#include "chrome/browser/ui/ash/launcher/app_window_base.h"
#include "chrome/browser/ui/ash/launcher/app_window_launcher_item_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/base/base_window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/widget/widget.h"

InternalAppWindowShelfController::InternalAppWindowShelfController(
    ChromeLauncherController* owner)
    : AppWindowLauncherController(owner) {
  aura::Env* env = aura::Env::GetInstanceDontCreate();
  if (env)
    env->AddObserver(this);
}

InternalAppWindowShelfController::~InternalAppWindowShelfController() {
  for (auto* window : observed_windows_)
    window->RemoveObserver(this);

  aura::Env* env = aura::Env::GetInstanceDontCreate();
  if (env)
    env->RemoveObserver(this);
}

void InternalAppWindowShelfController::OnWindowInitialized(
    aura::Window* window) {
  // An internal app window has type WINDOW_TYPE_NORMAL, a WindowDelegate and
  // is a top level views widget.
  if (window->type() != aura::client::WINDOW_TYPE_NORMAL || !window->delegate())
    return;
  views::Widget* widget = views::Widget::GetWidgetForNativeWindow(window);
  if (!widget || !widget->is_top_level())
    return;
  observed_windows_.push_back(window);

  window->AddObserver(this);
}

void InternalAppWindowShelfController::OnWindowPropertyChanged(
    aura::Window* window,
    const void* key,
    intptr_t old) {
  if (key != ash::kShelfIDKey)
    return;

  ash::ShelfID old_shelf_id =
      ash::ShelfID::Deserialize(reinterpret_cast<std::string*>(old));
  if (!old_shelf_id.IsNull() && app_list::IsInternalApp(old_shelf_id.app_id))
    DeleteAppWindow(old_shelf_id);

  ash::ShelfID shelf_id =
      ash::ShelfID::Deserialize(window->GetProperty(ash::kShelfIDKey));
  if (!shelf_id.IsNull() && app_list::IsInternalApp(shelf_id.app_id))
    RegisterAppWindow(window, shelf_id);
}

void InternalAppWindowShelfController::OnWindowVisibilityChanged(
    aura::Window* window,
    bool visible) {
  if (!visible)
    return;

  ash::ShelfID shelf_id =
      ash::ShelfID::Deserialize(window->GetProperty(ash::kShelfIDKey));
  if (shelf_id.IsNull() || !app_list::IsInternalApp(shelf_id.app_id))
    return;

  RegisterAppWindow(window, shelf_id);
}

void InternalAppWindowShelfController::OnWindowDestroying(
    aura::Window* window) {
  auto it =
      std::find(observed_windows_.begin(), observed_windows_.end(), window);
  DCHECK(it != observed_windows_.end());
  observed_windows_.erase(it);
  window->RemoveObserver(this);

  ash::ShelfID shelf_id =
      ash::ShelfID::Deserialize(window->GetProperty(ash::kShelfIDKey));
  if (!DeleteAppWindow(shelf_id))
    return;

  // Check if we may close controller now, at this point we can safely remove
  // controllers without window.
  AppWindowLauncherItemController* item_controller =
      owner()->shelf_model()->GetAppWindowLauncherItemController(shelf_id);
  if (item_controller != nullptr && item_controller->window_count() == 0)
    owner()->CloseLauncherItem(shelf_id);
}

void InternalAppWindowShelfController::RegisterAppWindow(
    aura::Window* window,
    const ash::ShelfID& shelf_id) {
  // Skip when this window has been handled. This can happen when the window
  // becomes visible again.
  if (shelf_id_to_app_window_.find(shelf_id) != shelf_id_to_app_window_.end())
    return;

  views::Widget* widget = views::Widget::GetWidgetForNativeWindow(window);
  auto app_window_ptr = std::make_unique<AppWindowBase>(shelf_id, widget);
  AppWindowBase* app_window = app_window_ptr.get();
  shelf_id_to_app_window_.insert(
      std::make_pair(shelf_id, std::move(app_window_ptr)));

  AppWindowLauncherItemController* item_controller =
      owner()->shelf_model()->GetAppWindowLauncherItemController(shelf_id);

  if (item_controller == nullptr) {
    auto controller =
        std::make_unique<AppWindowLauncherItemController>(shelf_id);
    item_controller = controller.get();
    if (!owner()->GetItem(shelf_id)) {
      owner()->CreateAppLauncherItem(std::move(controller),
                                     ash::STATUS_RUNNING);
    } else {
      owner()->shelf_model()->SetShelfItemDelegate(shelf_id,
                                                   std::move(controller));
      owner()->SetItemStatus(shelf_id, ash::STATUS_RUNNING);
    }
  }

  item_controller->AddWindow(app_window);
  app_window->SetController(item_controller);
}

void InternalAppWindowShelfController::UnregisterAppWindow(
    AppWindowBase* app_window) {
  if (!app_window)
    return;

  AppWindowLauncherItemController* controller = app_window->controller();
  if (controller)
    controller->RemoveWindow(app_window);

  app_window->SetController(nullptr);
}

bool InternalAppWindowShelfController::DeleteAppWindow(
    const ash::ShelfID& shelf_id) {
  auto app_window_it = shelf_id_to_app_window_.find(shelf_id);
  if (app_window_it == shelf_id_to_app_window_.end())
    return false;

  UnregisterAppWindow(app_window_it->second.get());
  shelf_id_to_app_window_.erase(app_window_it);
  return true;
}

AppWindowLauncherItemController*
InternalAppWindowShelfController::ControllerForWindow(aura::Window* window) {
  if (!window)
    return nullptr;

  ash::ShelfID shelf_id =
      ash::ShelfID::Deserialize(window->GetProperty(ash::kShelfIDKey));
  if (shelf_id.IsNull())
    return nullptr;

  auto app_window_it = shelf_id_to_app_window_.find(shelf_id);
  if (app_window_it == shelf_id_to_app_window_.end())
    return nullptr;

  AppWindowBase* app_window = app_window_it->second.get();
  if (app_window == nullptr)
    return nullptr;

  return app_window->controller();
}

void InternalAppWindowShelfController::OnItemDelegateDiscarded(
    ash::ShelfItemDelegate* delegate) {
  for (auto& it : shelf_id_to_app_window_) {
    AppWindowBase* app_window = it.second.get();
    if (!app_window || app_window->controller() != delegate)
      continue;

    VLOG(1) << "Item controller was released externally for the app "
            << delegate->shelf_id().app_id << ".";

    UnregisterAppWindow(it.second.get());
  }
}
