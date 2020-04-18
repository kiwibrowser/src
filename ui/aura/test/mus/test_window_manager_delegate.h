// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_DELEGATE_H_
#define UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "ui/aura/mus/window_manager_delegate.h"

namespace aura {

class TestWindowManagerDelegate : public aura::WindowManagerDelegate {
 public:
  TestWindowManagerDelegate();
  ~TestWindowManagerDelegate() override;

  // WindowManagerDelegate:
  void SetWindowManagerClient(aura::WindowManagerClient* client) override;
  void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) override;
  void OnWmConnected() override;
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
  ui::mojom::EventResult OnAccelerator(
      uint32_t accelerator_id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) override;
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnWmPerformMoveLoop(aura::Window* window,
                           ui::mojom::MoveLoopSource source,
                           const gfx::Point& cursor_location,
                           const base::Callback<void(bool)>& on_done) override;
  void OnWmCancelMoveLoop(aura::Window* window) override;
  void OnWmSetClientArea(
      aura::Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) override;
  bool IsWindowActive(aura::Window* window) override;
  void OnWmDeactivateWindow(aura::Window* window) override;

 private:
  std::vector<WindowTreeHostMus*> window_tree_hosts_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowManagerDelegate);
};

}  // namespace aura

#endif  // UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_DELEGATE_H_
