// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_PORT_MASH_H_
#define ASH_SHELL_PORT_MASH_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "ash/shell_port_mus.h"
#include "base/macros.h"

namespace views {
class PointerWatcherEventRouter;
}

namespace ash {

class AcceleratorControllerRegistrar;
class ImmersiveHandlerFactoryMash;
class WindowManager;

// ShellPort implementation for mash. See ash/README.md for more. Subclass of
// ShellPortMus because both configurations talk to the same UI service for
// things like display management.
class ShellPortMash : public ShellPortMus {
 public:
  ShellPortMash(WindowManager* window_manager,
                views::PointerWatcherEventRouter* pointer_watcher_event_router);
  ~ShellPortMash() override;

  static ShellPortMash* Get();

  // Called when the window server has changed the mouse enabled state.
  void OnCursorTouchVisibleChanged(bool enabled);

  // ShellPort:
  Config GetAshConfig() const override;
  void LockCursor() override;
  void UnlockCursor() override;
  void ShowCursor() override;
  void HideCursor() override;
  void SetCursorSize(ui::CursorSize cursor_size) override;
  void SetGlobalOverrideCursor(base::Optional<ui::CursorData> cursor) override;
  bool IsMouseEventsEnabled() override;
  void SetCursorTouchVisible(bool enabled) override;
  std::unique_ptr<WindowResizer> CreateDragWindowResizer(
      std::unique_ptr<WindowResizer> next_window_resizer,
      wm::WindowState* window_state) override;
  std::unique_ptr<WindowCycleEventFilter> CreateWindowCycleEventFilter()
      override;
  std::unique_ptr<wm::TabletModeEventHandler> CreateTabletModeEventHandler()
      override;
  std::unique_ptr<WorkspaceEventHandler> CreateWorkspaceEventHandler(
      aura::Window* workspace_window) override;
  std::unique_ptr<KeyboardUI> CreateKeyboardUI() override;
  void AddPointerWatcher(views::PointerWatcher* watcher,
                         views::PointerWatcherEventTypes events) override;
  void RemovePointerWatcher(views::PointerWatcher* watcher) override;
  bool IsTouchDown() override;
  void ToggleIgnoreExternalKeyboard() override;
  void CreatePointerWatcherAdapter() override;
  std::unique_ptr<AcceleratorController> CreateAcceleratorController() override;

 private:
  views::PointerWatcherEventRouter* pointer_watcher_event_router_ = nullptr;
  std::unique_ptr<AcceleratorControllerRegistrar>
      accelerator_controller_registrar_;
  std::unique_ptr<ImmersiveHandlerFactoryMash> immersive_handler_factory_;

  bool cursor_touch_visible_ = true;

  DISALLOW_COPY_AND_ASSIGN(ShellPortMash);
};

}  // namespace ash

#endif  // ASH_SHELL_PORT_MASH_H_
