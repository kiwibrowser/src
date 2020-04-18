// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/mus/test_window_manager_client.h"

#include <algorithm>

namespace aura {

TestWindowManagerClient::TestWindowManagerClient() {}

TestWindowManagerClient::~TestWindowManagerClient() {}

size_t TestWindowManagerClient::GetChangeCountForType(
    WindowManagerClientChangeType type) {
  size_t count = 0;
  for (const auto change_type : changes_) {
    if (change_type == type)
      ++count;
  }
  return count;
}

size_t TestWindowManagerClient::IndexOfFirstChangeOfType(
    WindowManagerClientChangeType type) const {
  auto iter = std::find(changes_.begin(), changes_.end(), type);
  return iter == changes_.end() ? static_cast<size_t>(-1)
                                : iter - changes_.begin();
}

void TestWindowManagerClient::AddActivationParent(ui::Id transport_window_id) {
  changes_.push_back(WindowManagerClientChangeType::ADD_ACTIVATION_PARENT);
}

void TestWindowManagerClient::RemoveActivationParent(
    ui::Id transport_window_id) {}

void TestWindowManagerClient::SetExtendedHitRegionForChildren(
    ui::Id window_id,
    const gfx::Insets& mouse_insets,
    const gfx::Insets& touch_insets) {}

void TestWindowManagerClient::AddAccelerators(
    std::vector<ui::mojom::WmAcceleratorPtr> accelerators,
    AddAcceleratorsCallback callback) {}

void TestWindowManagerClient::RemoveAccelerator(uint32_t id) {}

void TestWindowManagerClient::SetKeyEventsThatDontHideCursor(
    std::vector<::ui::mojom::EventMatcherPtr> dont_hide_cursor_list) {}

void TestWindowManagerClient::SetDisplayRoot(
    const display::Display& display,
    ui::mojom::WmViewportMetricsPtr viewport_metrics,
    bool is_primary_display,
    ui::Id window_id,
    const std::vector<display::Display>& mirrors,
    SetDisplayRootCallback callback) {}

void TestWindowManagerClient::SetDisplayConfiguration(
    const std::vector<display::Display>& displays,
    std::vector<::ui::mojom::WmViewportMetricsPtr> viewport_metrics,
    int64_t primary_display_id,
    int64_t internal_display_id,
    const std::vector<display::Display>& mirrors,
    SetDisplayConfigurationCallback callback) {
  last_internal_display_id_ = internal_display_id;
  changes_.push_back(WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION);
}

void TestWindowManagerClient::SwapDisplayRoots(
    int64_t display_id1,
    int64_t display_id2,
    SwapDisplayRootsCallback callback) {}

void TestWindowManagerClient::SetBlockingContainers(
    std::vector<ui::mojom::BlockingContainersPtr> blocking_containers,
    SetBlockingContainersCallback callback) {}

void TestWindowManagerClient::WmResponse(uint32_t change_id, bool response) {}

void TestWindowManagerClient::WmSetBoundsResponse(uint32_t change_id) {}

void TestWindowManagerClient::WmRequestClose(ui::Id transport_window_id) {}

void TestWindowManagerClient::WmSetFrameDecorationValues(
    ui::mojom::FrameDecorationValuesPtr values) {
  changes_.push_back(WindowManagerClientChangeType::SET_FRAME_DECORATIONS);
}

void TestWindowManagerClient::WmSetNonClientCursor(ui::Id window_id,
                                                   ui::CursorData cursor_data) {
}

void TestWindowManagerClient::WmLockCursor() {}

void TestWindowManagerClient::WmUnlockCursor() {}

void TestWindowManagerClient::WmSetCursorVisible(bool visible) {}

void TestWindowManagerClient::WmSetCursorSize(ui::CursorSize cursor_size) {}

void TestWindowManagerClient::WmSetGlobalOverrideCursor(
    base::Optional<ui::CursorData> cursor) {}

void TestWindowManagerClient::WmMoveCursorToDisplayLocation(
    const gfx::Point& display_pixels,
    int64_t display_id) {}

void TestWindowManagerClient::WmConfineCursorToBounds(
    const gfx::Rect& bounds_in_pixles,
    int64_t display_id) {}

void TestWindowManagerClient::WmSetCursorTouchVisible(bool enabled) {}

void TestWindowManagerClient::OnWmCreatedTopLevelWindow(
    uint32_t change_id,
    ui::Id transport_window_id) {}

void TestWindowManagerClient::OnAcceleratorAck(
    uint32_t event_id,
    ui::mojom::EventResult result,
    const base::flat_map<std::string, std::vector<uint8_t>>& properties) {}

}  // namespace aura
