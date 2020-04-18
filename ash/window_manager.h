// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WINDOW_MANAGER_H_
#define ASH_WINDOW_MANAGER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ash/ash_export.h"
#include "ash/root_window_controller.h"
#include "ash/shell_delegate.h"
#include "base/macros.h"
#include "services/ui/common/types.h"
#include "services/ui/public/interfaces/display/display_controller.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_client_delegate.h"

namespace display {
class Display;
}

namespace service_manager {
class Connector;
}

namespace ui {
class InputDeviceClient;
}

namespace views {
class PointerWatcherEventRouter;
}

namespace wm {
class WMState;
}

namespace ash {
class AcceleratorHandler;
class AshTestHelper;

enum class Config;

// WindowManager serves as the WindowManagerDelegate and
// WindowTreeClientDelegate for mash. WindowManager takes ownership of
// the WindowTreeClient. This is used in not in classic.
class ASH_EXPORT WindowManager : public aura::WindowManagerDelegate,
                                 public aura::WindowTreeClientDelegate {
 public:
  // Set |show_primary_host_on_connect| to true if the initial display should
  // be made visible.  Generally tests should use false, other places use true.
  WindowManager(service_manager::Connector* connector,
                bool show_primary_host_on_connect);
  ~WindowManager() override;

  // |initial_display_prefs| contains a dictionary of initial display prefs to
  // pass to Shell::Init for synchronous initial display configuraiton.
  void Init(std::unique_ptr<aura::WindowTreeClient> window_tree_client,
            std::unique_ptr<base::Value> initial_display_prefs);

  aura::WindowTreeClient* window_tree_client() {
    return window_tree_client_.get();
  }

  aura::WindowManagerClient* window_manager_client() {
    return window_manager_client_;
  }

  service_manager::Connector* connector() { return connector_; }

  aura::PropertyConverter* property_converter() {
    return property_converter_.get();
  }

  // Returns the next accelerator namespace id by value in |id|. Returns true
  // if there is another slot available, false if all slots are taken up.
  bool GetNextAcceleratorNamespaceId(uint16_t* id);
  void AddAcceleratorHandler(uint16_t id_namespace,
                             AcceleratorHandler* handler);
  void RemoveAcceleratorHandler(uint16_t id_namespace);

  // Returns the DisplayController interface if available. Will be null if no
  // service_manager::Connector was available, for example in some tests.
  display::mojom::DisplayController* GetDisplayController();

 private:
  friend class ash::AshTestHelper;

  // Creates the Shell. This is done after the connection to mus is established.
  void CreateShell();

  // If Mash, tells the window server about keys we don't want to hide the
  // cursor on.
  void InitCursorOnKeyList();

  // Sets the frame decoration values on the server.
  void InstallFrameDecorationValues();

  void Shutdown();

  // WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) override;
  void OnLostConnection(aura::WindowTreeClient* client) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override;
  aura::PropertyConverter* GetPropertyConverter() override;

  // WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) override;
  void OnWmConnected() override;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override;
  void OnWmSetBounds(aura::Window* window, const gfx::Rect& bounds) override;
  bool OnWmSetProperty(
      aura::Window* window,
      const std::string& name,
      std::unique_ptr<std::vector<uint8_t>>* new_data) override;
  void OnWmSetModalType(aura::Window* window, ui::ModalType type) override;
  void OnWmSetCanFocus(aura::Window* window, bool can_focus) override;
  aura::Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) override;
  void OnWmClientJankinessChanged(const std::set<aura::Window*>& client_windows,
                                  bool not_responding) override;
  void OnWmBuildDragImage(const gfx::Point& screen_location,
                          const SkBitmap& drag_image,
                          const gfx::Vector2d& drag_image_offset,
                          ui::mojom::PointerKind source) override;
  void OnWmMoveDragImage(const gfx::Point& screen_location) override;
  void OnWmDestroyDragImage() override;
  void OnWmWillCreateDisplay(const display::Display& display) override;
  void OnWmNewDisplay(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
                      const display::Display& display) override;
  void OnWmDisplayRemoved(aura::WindowTreeHostMus* window_tree_host) override;
  void OnWmDisplayModified(const display::Display& display) override;
  void OnWmPerformMoveLoop(aura::Window* window,
                           ui::mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) override;
  void OnWmCancelMoveLoop(aura::Window* window) override;
  ui::mojom::EventResult OnAccelerator(
      uint32_t id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) override;
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override;
  bool IsWindowActive(aura::Window* window) override;
  void OnWmDeactivateWindow(aura::Window* window) override;
  void OnWmPerformAction(aura::Window* window,
                         const std::string& action) override;
  void OnEventBlockedByModalWindow(aura::Window* window) override;

  service_manager::Connector* connector_;
  display::mojom::DisplayControllerPtr display_controller_;

  const bool show_primary_host_on_connect_;

  std::unique_ptr<::wm::WMState> wm_state_;
  std::unique_ptr<aura::PropertyConverter> property_converter_;

  std::unique_ptr<aura::WindowTreeClient> window_tree_client_;

  aura::WindowManagerClient* window_manager_client_ = nullptr;

  std::unique_ptr<views::PointerWatcherEventRouter>
      pointer_watcher_event_router_;

  bool created_shell_ = false;

  std::map<uint16_t, AcceleratorHandler*> accelerator_handlers_;
  uint16_t next_accelerator_namespace_id_ = 0u;

  // The ShellDelegate to install. This may be null, in which case
  // ShellDelegateMus is used.
  // NOTE: AshTestHelper may set |shell_delegate_| directly.
  std::unique_ptr<ShellDelegate> shell_delegate_;

  std::unique_ptr<base::Value> initial_display_prefs_;

  // State that is only valid during a drag.
  struct DragState;
  std::unique_ptr<DragState> drag_state_;

  std::unique_ptr<ui::InputDeviceClient> input_device_client_;

  DISALLOW_COPY_AND_ASSIGN(WindowManager);
};

}  // namespace ash

#endif  // ASH_WINDOW_MANAGER_H_
