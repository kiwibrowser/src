// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_SIMPLE_WM_SIMPLE_WM_H_
#define MASH_SIMPLE_WM_SIMPLE_WM_H_

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/wm/core/base_focus_rules.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/wm_state.h"

namespace display {
class ScreenBase;
}

namespace views {
class AuraInit;
}

namespace wm {
class FocusController;
}

namespace simple_wm {

class SimpleWM : public service_manager::Service,
                 public aura::WindowTreeClientDelegate,
                 public aura::WindowManagerDelegate,
                 public wm::BaseFocusRules {
 public:
  SimpleWM();
  ~SimpleWM() override;

 private:
  class DisplayLayoutManager;
  class FrameView;
  class WindowListModel;
  class WindowListModelObserver;
  class WindowListView;
  class WorkspaceLayoutManager;

  // service_manager::Service:
  void OnStart() override;

  // aura::WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override;
  void OnLostConnection(aura::WindowTreeClient* client) override;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override;
  aura::PropertyConverter* GetPropertyConverter() override;

  // aura::WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) override;
  void OnWmConnected() override;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override {}
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
                                  bool janky) override;
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
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override;
  bool IsWindowActive(aura::Window* window) override;
  void OnWmDeactivateWindow(aura::Window* window) override;
  void OnWmPerformAction(aura::Window* window,
                         const std::string& action) override;

  // wm::BaseFocusRules:
  bool SupportsChildActivation(aura::Window* window) const override;
  bool IsWindowConsideredVisibleForActivation(
      aura::Window* window) const override;

  FrameView* GetFrameViewForClientWindow(aura::Window* client_window);

  void OnWindowListViewItemActivated(aura::Window* index);

  std::unique_ptr<views::AuraInit> aura_init_;
  wm::WMState wm_state_;
  std::unique_ptr<display::ScreenBase> screen_;
  aura::PropertyConverter property_converter_;
  std::unique_ptr<wm::FocusController> focus_controller_;
  std::unique_ptr<aura::WindowTreeHostMus> window_tree_host_;
  aura::Window* display_root_ = nullptr;
  aura::Window* window_root_ = nullptr;
  aura::WindowManagerClient* window_manager_client_ = nullptr;
  std::unique_ptr<aura::WindowTreeClient> window_tree_client_;
  std::map<aura::Window*, FrameView*> client_window_to_frame_view_;
  std::unique_ptr<WindowListModel> window_list_model_;
  std::unique_ptr<WorkspaceLayoutManager> workspace_layout_manager_;

  bool started_ = false;

  DISALLOW_COPY_AND_ASSIGN(SimpleWM);
};

}  // namespace simple_wm

#endif  // MASH_SIMPLE_WM_SIMPLE_WM_H_
