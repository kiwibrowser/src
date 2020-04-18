// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_PLATFORM_DISPLAY_DEFAULT_H_
#define SERVICES_UI_WS_PLATFORM_DISPLAY_DEFAULT_H_

#include <memory>

#include "base/macros.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/platform_display_delegate.h"
#include "services/ui/ws/server_window.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

class EventSink;
class PlatformWindow;

namespace ws {

class ThreadedImageCursors;

// PlatformDisplay implementation that connects to a PlatformWindow and
// FrameGenerator for Chrome OS.
class PlatformDisplayDefault : public PlatformDisplay,
                               public ui::PlatformWindowDelegate {
 public:
  // |image_cursors| may be null, for example on Android or in tests.
  PlatformDisplayDefault(ServerWindow* root_window,
                         const display::ViewportMetrics& metrics,
                         std::unique_ptr<ThreadedImageCursors> image_cursors);
  ~PlatformDisplayDefault() override;

  // EventSource::
  EventSink* GetEventSink() override;

  // PlatformDisplay:
  void Init(PlatformDisplayDelegate* delegate) override;
  void SetViewportSize(const gfx::Size& size) override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void SetCursor(const ui::CursorData& cursor) override;
  void MoveCursorTo(const gfx::Point& window_pixel_location) override;
  void SetCursorSize(const ui::CursorSize& cursor_size) override;
  void ConfineCursorToBounds(const gfx::Rect& pixel_bounds) override;
  void UpdateTextInputState(const ui::TextInputState& state) override;
  void SetImeVisibility(bool visible) override;
  void UpdateViewportMetrics(const display::ViewportMetrics& metrics) override;
  const display::ViewportMetrics& GetViewportMetrics() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  FrameGenerator* GetFrameGenerator() override;
  void SetCursorConfig(display::Display::Rotation rotation,
                       float scale) override;

 private:
  // ui::PlatformWindowDelegate:
  void OnBoundsChanged(const gfx::Rect& new_bounds) override;
  void OnDamageRect(const gfx::Rect& damaged_region) override;
  void DispatchEvent(ui::Event* event) override;
  void OnCloseRequest() override;
  void OnClosed() override;
  void OnWindowStateChanged(ui::PlatformWindowState new_state) override;
  void OnLostCapture() override;
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget,
                                    float device_scale_factor) override;
  void OnAcceleratedWidgetDestroying() override;
  void OnAcceleratedWidgetDestroyed() override;
  void OnActivationChanged(bool active) override;

  ServerWindow* root_window_;

  std::unique_ptr<ThreadedImageCursors> image_cursors_;

  PlatformDisplayDelegate* delegate_ = nullptr;
  std::unique_ptr<FrameGenerator> frame_generator_;

  display::ViewportMetrics metrics_;
  std::unique_ptr<ui::PlatformWindow> platform_window_;
  gfx::AcceleratedWidget widget_;

  DISALLOW_COPY_AND_ASSIGN(PlatformDisplayDefault);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_PLATFORM_DISPLAY_DEFAULT_H_
