// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_PORT_MUS_H_
#define ASH_SHELL_PORT_MUS_H_

#include <stdint.h>

#include <memory>

#include "ash/shell_port.h"
#include "base/macros.h"

namespace aura {
class WindowTreeClient;
}

namespace ash {

class DisplaySynchronizer;
class PointerWatcherAdapterClassic;
class RootWindowController;
class WindowManager;

// ShellPort implementation for mus (and parts of mash). See ash/README.md.
// Often uses "classic" ash implementations because only the window server
// pieces are in a different mojo service.
// TODO(crbug.com/841941): Fold into ShellPortMash. Config::MUS is deprecated.
class ShellPortMus : public ShellPort {
 public:
  explicit ShellPortMus(WindowManager* window_manager);
  ~ShellPortMus() override;

  static ShellPortMus* Get();

  ash::RootWindowController* GetRootWindowControllerWithDisplayId(int64_t id);

  aura::WindowTreeClient* window_tree_client();

  WindowManager* window_manager() { return window_manager_; }

  // ShellPort:
  void Shutdown() override;
  Config GetAshConfig() const override;
  std::unique_ptr<display::TouchTransformSetter> CreateTouchTransformDelegate()
      override;
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
  std::unique_ptr<AshWindowTreeHost> CreateAshWindowTreeHost(
      const AshWindowTreeHostInitParams& init_params) override;
  void OnCreatedRootWindowContainers(
      RootWindowController* root_window_controller) override;
  void UpdateSystemModalAndBlockingContainers() override;
  void OnHostsInitialized() override;
  std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate()
      override;
  std::unique_ptr<AcceleratorController> CreateAcceleratorController() override;
  void AddVideoDetectorObserver(
      viz::mojom::VideoDetectorObserverPtr observer) override;

 protected:
  WindowManager* window_manager_;

 private:
  std::unique_ptr<PointerWatcherAdapterClassic> pointer_watcher_adapter_;

  std::unique_ptr<DisplaySynchronizer> display_synchronizer_;

  DISALLOW_COPY_AND_ASSIGN(ShellPortMus);
};

}  // namespace ash

#endif  // ASH_SHELL_PORT_MUS_H_
