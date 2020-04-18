// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell_port_mash.h"

#include <memory>
#include <utility>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/accelerators/accelerator_controller_registrar.h"
#include "ash/keyboard/keyboard_ui_mash.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/shell.h"
#include "ash/window_manager.h"
#include "ash/wm/drag_window_resizer_mash.h"
#include "ash/wm/immersive_handler_factory_mash.h"
#include "ash/wm/tablet_mode/tablet_mode_event_handler.h"
#include "ash/wm/window_cycle_event_filter.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/workspace/workspace_event_handler_mash.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/views/mus/pointer_watcher_event_router.h"

namespace ash {

ShellPortMash::ShellPortMash(
    WindowManager* window_manager,
    views::PointerWatcherEventRouter* pointer_watcher_event_router)
    : ShellPortMus(window_manager),
      pointer_watcher_event_router_(pointer_watcher_event_router),
      immersive_handler_factory_(
          std::make_unique<ImmersiveHandlerFactoryMash>()) {
  DCHECK(pointer_watcher_event_router_);
  DCHECK_EQ(Config::MASH, GetAshConfig());
}

ShellPortMash::~ShellPortMash() = default;

// static
ShellPortMash* ShellPortMash::Get() {
  const ash::Config config = ShellPort::Get()->GetAshConfig();
  CHECK_EQ(Config::MASH, config);
  return static_cast<ShellPortMash*>(ShellPort::Get());
}

Config ShellPortMash::GetAshConfig() const {
  return Config::MASH;
}

void ShellPortMash::LockCursor() {
  // When we are running in mus, we need to keep track of state not just in the
  // window server, but also locally in ash because ash treats the cursor
  // manager as the canonical state for now. NativeCursorManagerAsh will keep
  // this state, while also forwarding it to the window manager for us.
  window_manager_->window_manager_client()->LockCursor();
}

void ShellPortMash::UnlockCursor() {
  window_manager_->window_manager_client()->UnlockCursor();
}

void ShellPortMash::ShowCursor() {
  window_manager_->window_manager_client()->SetCursorVisible(true);
}

void ShellPortMash::HideCursor() {
  window_manager_->window_manager_client()->SetCursorVisible(false);
}

void ShellPortMash::SetCursorSize(ui::CursorSize cursor_size) {
  window_manager_->window_manager_client()->SetCursorSize(cursor_size);
}

void ShellPortMash::SetGlobalOverrideCursor(
    base::Optional<ui::CursorData> cursor) {
  window_manager_->window_manager_client()->SetGlobalOverrideCursor(
      std::move(cursor));
}

bool ShellPortMash::IsMouseEventsEnabled() {
  return cursor_touch_visible_;
}

void ShellPortMash::SetCursorTouchVisible(bool enabled) {
  window_manager_->window_manager_client()->SetCursorTouchVisible(enabled);
}

void ShellPortMash::OnCursorTouchVisibleChanged(bool enabled) {
  cursor_touch_visible_ = enabled;
}

std::unique_ptr<WindowResizer> ShellPortMash::CreateDragWindowResizer(
    std::unique_ptr<WindowResizer> next_window_resizer,
    wm::WindowState* window_state) {
  return std::make_unique<ash::DragWindowResizerMash>(
      std::move(next_window_resizer), window_state);
}

std::unique_ptr<WindowCycleEventFilter>
ShellPortMash::CreateWindowCycleEventFilter() {
  // TODO: implement me, http://crbug.com/629191.
  return nullptr;
}

std::unique_ptr<wm::TabletModeEventHandler>
ShellPortMash::CreateTabletModeEventHandler() {
  // TODO: need support for window manager to get events before client:
  // http://crbug.com/624157.
  NOTIMPLEMENTED_LOG_ONCE();
  return nullptr;
}

std::unique_ptr<WorkspaceEventHandler>
ShellPortMash::CreateWorkspaceEventHandler(aura::Window* workspace_window) {
  return std::make_unique<WorkspaceEventHandlerMash>(workspace_window);
}

std::unique_ptr<KeyboardUI> ShellPortMash::CreateKeyboardUI() {
  return KeyboardUIMash::Create(window_manager_->connector());
}

void ShellPortMash::AddPointerWatcher(views::PointerWatcher* watcher,
                                      views::PointerWatcherEventTypes events) {
  // TODO: implement drags for mus pointer watcher, http://crbug.com/641164.
  // NOTIMPLEMENTED_LOG_ONCE drags for mus pointer watcher.
  pointer_watcher_event_router_->AddPointerWatcher(
      watcher, events == views::PointerWatcherEventTypes::MOVES);
}

void ShellPortMash::RemovePointerWatcher(views::PointerWatcher* watcher) {
  pointer_watcher_event_router_->RemovePointerWatcher(watcher);
}

bool ShellPortMash::IsTouchDown() {
  // TODO: implement me, http://crbug.com/634967.
  return false;
}

void ShellPortMash::ToggleIgnoreExternalKeyboard() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void ShellPortMash::CreatePointerWatcherAdapter() {
  // In Config::MUS PointerWatcherAdapterClassic must be created when this
  // function is called (it is order dependent), that is not the case with
  // Config::MASH.
}

std::unique_ptr<AcceleratorController>
ShellPortMash::CreateAcceleratorController() {
  uint16_t accelerator_namespace_id = 0u;
  const bool add_result =
      window_manager_->GetNextAcceleratorNamespaceId(&accelerator_namespace_id);
  // ShellPortMash is created early on, so that GetNextAcceleratorNamespaceId()
  // should always succeed.
  DCHECK(add_result);

  accelerator_controller_registrar_ =
      std::make_unique<AcceleratorControllerRegistrar>(
          window_manager_, accelerator_namespace_id);
  return std::make_unique<AcceleratorController>(
      accelerator_controller_registrar_.get());
}

}  // namespace ash
