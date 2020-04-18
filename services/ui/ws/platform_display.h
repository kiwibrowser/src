// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_PLATFORM_DISPLAY_H_
#define SERVICES_UI_WS_PLATFORM_DISPLAY_H_

#include <stdint.h>

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "ui/display/display.h"
#include "ui/events/event_source.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

enum class CursorSize;
class PlatformWindow;
class PlatformWindowDelegate;
struct TextInputState;

namespace ws {

class FrameGenerator;
class PlatformDisplayDelegate;
class PlatformDisplayFactory;
class ServerWindow;
class ThreadedImageCursorsFactory;

// PlatformDisplay is used to connect the root ServerWindow to a display.
class PlatformDisplay : public ui::EventSource {
 public:
  ~PlatformDisplay() override {}

  static std::unique_ptr<PlatformDisplay> Create(
      ServerWindow* root_window,
      const display::ViewportMetrics& metrics,
      ThreadedImageCursorsFactory* threaded_image_cursors_factory);

  virtual void Init(PlatformDisplayDelegate* delegate) = 0;

  virtual void SetViewportSize(const gfx::Size& size) = 0;

  virtual void SetTitle(const base::string16& title) = 0;

  virtual void SetCapture() = 0;

  virtual void ReleaseCapture() = 0;

  virtual void SetCursor(const ui::CursorData& cursor) = 0;

  virtual void MoveCursorTo(const gfx::Point& window_pixel_location) = 0;

  virtual void SetCursorSize(const ui::CursorSize& cursor_size) = 0;

  virtual void ConfineCursorToBounds(const gfx::Rect& pixel_bounds) = 0;

  virtual void UpdateTextInputState(const ui::TextInputState& state) = 0;
  virtual void SetImeVisibility(bool visible) = 0;

  // Updates the viewport metrics for the display.
  virtual void UpdateViewportMetrics(
      const display::ViewportMetrics& metrics) = 0;

  virtual const display::ViewportMetrics& GetViewportMetrics() = 0;

  // Returns the AcceleratedWidget associated with the Display. It can return
  // kNullAcceleratedWidget if the accelerated widget is not available yet.
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() const = 0;

  virtual FrameGenerator* GetFrameGenerator() = 0;

  virtual void SetCursorConfig(display::Display::Rotation rotation,
                               float scale) = 0;

  // Overrides factory for testing. Default (NULL) value indicates regular
  // (non-test) environment.
  static void set_factory_for_testing(PlatformDisplayFactory* factory) {
    PlatformDisplay::factory_ = factory;
  }

 protected:
  // Create a platform window with the given delegate and bounds.
  static std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      PlatformWindowDelegate* delegate,
      const gfx::Rect& bounds);

 private:
  // Static factory instance (always NULL for non-test).
  static PlatformDisplayFactory* factory_;
};


}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_PLATFORM_DISPLAY_H_
