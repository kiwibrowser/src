// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/mus/test_window_manager_delegate.h"

#include "ui/aura/mus/window_tree_host_mus.h"

namespace aura {

TestWindowManagerDelegate::TestWindowManagerDelegate() = default;

TestWindowManagerDelegate::~TestWindowManagerDelegate() = default;

void TestWindowManagerDelegate::SetWindowManagerClient(
    aura::WindowManagerClient* client) {}

void TestWindowManagerDelegate::OnWmAcceleratedWidgetAvailableForDisplay(
    int64_t display_id,
    gfx::AcceleratedWidget widget) {}

void TestWindowManagerDelegate::OnWmConnected() {}

void TestWindowManagerDelegate::OnWmSetBounds(aura::Window* window,
                                              const gfx::Rect& bounds) {}

bool TestWindowManagerDelegate::OnWmSetProperty(
    aura::Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  return false;
}

void TestWindowManagerDelegate::OnWmSetModalType(aura::Window* window,
                                                 ui::ModalType type) {}

void TestWindowManagerDelegate::OnWmSetCanFocus(aura::Window* window,
                                                bool can_focus) {}

aura::Window* TestWindowManagerDelegate::OnWmCreateTopLevelWindow(
    ui::mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  return nullptr;
}

void TestWindowManagerDelegate::OnWmClientJankinessChanged(
    const std::set<aura::Window*>& client_windows,
    bool not_responding) {}

void TestWindowManagerDelegate::OnWmBuildDragImage(
    const gfx::Point& screen_location,
    const SkBitmap& drag_image,
    const gfx::Vector2d& drag_image_offset,
    ui::mojom::PointerKind source) {}

void TestWindowManagerDelegate::OnWmMoveDragImage(
    const gfx::Point& screen_location) {}

void TestWindowManagerDelegate::OnWmDestroyDragImage() {}

void TestWindowManagerDelegate::OnWmWillCreateDisplay(
    const display::Display& display) {}

void TestWindowManagerDelegate::OnWmNewDisplay(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  // We assume someone else is taking ownership (which is the case for
  // AuraTestHelper).
  window_tree_hosts_.push_back(window_tree_host.release());
}

void TestWindowManagerDelegate::OnWmDisplayRemoved(
    aura::WindowTreeHostMus* window_tree_host) {}

void TestWindowManagerDelegate::OnWmDisplayModified(
    const display::Display& display) {}

ui::mojom::EventResult TestWindowManagerDelegate::OnAccelerator(
    uint32_t accelerator_id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  return ui::mojom::EventResult::UNHANDLED;
}

void TestWindowManagerDelegate::OnCursorTouchVisibleChanged(bool enabled) {}

void TestWindowManagerDelegate::OnWmPerformMoveLoop(
    aura::Window* window,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {}

void TestWindowManagerDelegate::OnWmCancelMoveLoop(aura::Window* window) {}

void TestWindowManagerDelegate::OnWmSetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {}

bool TestWindowManagerDelegate::IsWindowActive(aura::Window* window) {
  return true;
}

void TestWindowManagerDelegate::OnWmDeactivateWindow(aura::Window* window) {}

}  // namespace aura
