// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_CLIENT_H_
#define UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_CLIENT_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/mus/mus_types.h"

namespace aura {

enum class WindowManagerClientChangeType {
  ADD_ACTIVATION_PARENT,
  SET_DISPLAY_CONFIGURATION,
  SET_FRAME_DECORATIONS,
};

// WindowManagerClient implementation for tests.
class TestWindowManagerClient : public ui::mojom::WindowManagerClient {
 public:
  TestWindowManagerClient();
  ~TestWindowManagerClient() override;

  size_t GetChangeCountForType(WindowManagerClientChangeType type);
  int64_t last_internal_display_id() const { return last_internal_display_id_; }
  size_t IndexOfFirstChangeOfType(WindowManagerClientChangeType type) const;

 private:
  // ui::mojom::WindowManagerClient:
  void AddActivationParent(ui::Id transport_window_id) override;
  void RemoveActivationParent(ui::Id transport_window_id) override;
  void SetExtendedHitRegionForChildren(
      ui::Id window_id,
      const gfx::Insets& mouse_insets,
      const gfx::Insets& touch_insets) override;
  void AddAccelerators(std::vector<ui::mojom::WmAcceleratorPtr> accelerators,
                       AddAcceleratorsCallback callback) override;
  void RemoveAccelerator(uint32_t id) override;
  void SetKeyEventsThatDontHideCursor(
      std::vector<::ui::mojom::EventMatcherPtr> dont_hide_cursor_list) override;
  void SetDisplayRoot(const display::Display& display,
                      ui::mojom::WmViewportMetricsPtr viewport_metrics,
                      bool is_primary_display,
                      ui::Id window_id,
                      const std::vector<display::Display>& mirrors,
                      SetDisplayRootCallback callback) override;
  void SetDisplayConfiguration(
      const std::vector<display::Display>& displays,
      std::vector<::ui::mojom::WmViewportMetricsPtr> viewport_metrics,
      int64_t primary_display_id,
      int64_t internal_display_id,
      const std::vector<display::Display>& mirrors,
      SetDisplayConfigurationCallback callback) override;
  void SwapDisplayRoots(int64_t display_id1,
                        int64_t display_id2,
                        SwapDisplayRootsCallback callback) override;
  void SetBlockingContainers(
      std::vector<ui::mojom::BlockingContainersPtr> blocking_containers,
      SetBlockingContainersCallback callback) override;
  void WmResponse(uint32_t change_id, bool response) override;
  void WmSetBoundsResponse(uint32_t change_id) override;
  void WmRequestClose(ui::Id transport_window_id) override;
  void WmSetFrameDecorationValues(
      ui::mojom::FrameDecorationValuesPtr values) override;
  void WmSetNonClientCursor(ui::Id window_id,
                            ui::CursorData cursor_data) override;
  void WmLockCursor() override;
  void WmUnlockCursor() override;
  void WmSetCursorVisible(bool visible) override;
  void WmSetCursorSize(ui::CursorSize cursor_size) override;
  void WmSetGlobalOverrideCursor(
      base::Optional<ui::CursorData> cursor) override;
  void WmMoveCursorToDisplayLocation(const gfx::Point& display_pixels,
                                     int64_t display_id) override;
  void WmConfineCursorToBounds(const gfx::Rect& bounds_in_pixles,
                               int64_t display_id) override;
  void WmSetCursorTouchVisible(bool enabled) override;
  void OnWmCreatedTopLevelWindow(uint32_t change_id,
                                 ui::Id transport_window_id) override;
  void OnAcceleratorAck(uint32_t event_id,
                        ui::mojom::EventResult result,
                        const base::flat_map<std::string, std::vector<uint8_t>>&
                            properties) override;

  std::vector<WindowManagerClientChangeType> changes_;
  int64_t last_internal_display_id_ = -1;

  DISALLOW_COPY_AND_ASSIGN(TestWindowManagerClient);
};

}  // namespace aura

#endif  // UI_AURA_TEST_MUS_TEST_WINDOW_MANAGER_CLIENT_H_
