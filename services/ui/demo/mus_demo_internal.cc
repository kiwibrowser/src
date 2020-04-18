// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/demo/mus_demo_internal.h"

#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/demo/window_tree_data.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"

namespace ui {
namespace demo {

namespace {

// Size of square in pixels to draw.
const int kSquareSize = 300;
}

MusDemoInternal::MusDemoInternal() {}

MusDemoInternal::~MusDemoInternal() {}

std::unique_ptr<aura::WindowTreeClient>
MusDemoInternal::CreateWindowTreeClient() {
  return aura::WindowTreeClient::CreateForWindowManager(context()->connector(),
                                                        this, this);
}

void MusDemoInternal::OnStartImpl() {
  // The demo will actually start when the window server creates the display,
  // causing OnWmNewDisplay to be called.
}

void MusDemoInternal::SetWindowManagerClient(
    aura::WindowManagerClient* client) {}

void MusDemoInternal::OnWmSetBounds(aura::Window* window,
                                    const gfx::Rect& bounds) {}

bool MusDemoInternal::OnWmSetProperty(
    aura::Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  return true;
}

void MusDemoInternal::OnWmSetModalType(aura::Window* window, ModalType type) {}

void MusDemoInternal::OnWmSetCanFocus(aura::Window* window, bool can_focus) {}

aura::Window* MusDemoInternal::OnWmCreateTopLevelWindow(
    mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  NOTREACHED();
  return nullptr;
}

void MusDemoInternal::OnWmClientJankinessChanged(
    const std::set<aura::Window*>& client_windows,
    bool janky) {
  // Don't care
}

void MusDemoInternal::OnWmBuildDragImage(const gfx::Point& screen_location,
                                         const SkBitmap& drag_image,
                                         const gfx::Vector2d& drag_image_offset,
                                         ui::mojom::PointerKind source) {}

void MusDemoInternal::OnWmMoveDragImage(const gfx::Point& screen_location) {}

void MusDemoInternal::OnWmDestroyDragImage() {}

void MusDemoInternal::OnWmWillCreateDisplay(const display::Display& display) {
  AddPrimaryDisplay(display);
}

void MusDemoInternal::OnWmNewDisplay(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  AppendWindowTreeData(std::make_unique<WindowTreeData>(kSquareSize));
  InitWindowTreeData(std::move(window_tree_host));
}

void MusDemoInternal::OnWmDisplayRemoved(
    aura::WindowTreeHostMus* window_tree_host) {
  RemoveWindowTreeData(window_tree_host);
}

void MusDemoInternal::OnWmDisplayModified(const display::Display& display) {}

mojom::EventResult MusDemoInternal::OnAccelerator(
    uint32_t id,
    const Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  return mojom::EventResult::UNHANDLED;
}

void MusDemoInternal::OnCursorTouchVisibleChanged(bool enabled) {}

void MusDemoInternal::OnWmPerformMoveLoop(
    aura::Window* window,
    mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {
  // Don't care
}

void MusDemoInternal::OnWmCancelMoveLoop(aura::Window* window) {}

void MusDemoInternal::OnWmSetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {}

bool MusDemoInternal::IsWindowActive(aura::Window* window) {
  return false;
}

void MusDemoInternal::OnWmDeactivateWindow(aura::Window* window) {}

}  // namespace demo
}  // namespace ui
