// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DEMO_MUS_DEMO_INTERNAL_H_
#define SERVICES_UI_DEMO_MUS_DEMO_INTERNAL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "services/ui/demo/mus_demo.h"
#include "ui/aura/mus/window_manager_delegate.h"

namespace ui {
namespace demo {

// MusDemoInternal demonstrates Mus operating in "internal" mode: There is a
// single acceleratedWidget for a single display and all aura windows are
// contained in that display.
class MusDemoInternal : public MusDemo, public aura::WindowManagerDelegate {
 public:
  MusDemoInternal();
  ~MusDemoInternal() final;

 private:
  // ui::demo::MusDemo:
  void OnStartImpl() final;
  std::unique_ptr<aura::WindowTreeClient> CreateWindowTreeClient() final;

  // aura::WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) final;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) final {}
  void OnWmSetBounds(aura::Window* window, const gfx::Rect& bounds) final;
  bool OnWmSetProperty(aura::Window* window,
                       const std::string& name,
                       std::unique_ptr<std::vector<uint8_t>>* new_data) final;
  void OnWmSetModalType(aura::Window* window, ModalType type) final;
  void OnWmSetCanFocus(aura::Window* window, bool can_focus) final;
  aura::Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) final;
  void OnWmClientJankinessChanged(const std::set<aura::Window*>& client_windows,
                                  bool janky) final;
  void OnWmBuildDragImage(const gfx::Point& screen_location,
                          const SkBitmap& drag_image,
                          const gfx::Vector2d& drag_image_offset,
                          ui::mojom::PointerKind source) final;
  void OnWmMoveDragImage(const gfx::Point& screen_location) final;
  void OnWmDestroyDragImage() final;
  void OnWmWillCreateDisplay(const display::Display& display) final;
  void OnWmNewDisplay(std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
                      const display::Display& display) final;
  void OnWmDisplayRemoved(aura::WindowTreeHostMus* window_tree_host) final;
  void OnWmDisplayModified(const display::Display& display) final;
  ui::mojom::EventResult OnAccelerator(
      uint32_t id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) final;
  void OnCursorTouchVisibleChanged(bool enabled) final;
  void OnWmPerformMoveLoop(aura::Window* window,
                           ui::mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) final;
  void OnWmCancelMoveLoop(aura::Window* window) final;
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) final;
  bool IsWindowActive(aura::Window* window) final;
  void OnWmDeactivateWindow(aura::Window* window) final;

  DISALLOW_COPY_AND_ASSIGN(MusDemoInternal);
};

}  // namespace demo
}  // namespace ui

#endif  // SERVICES_UI_DEMO_MUS_DEMO_INTERNAL_H_
