// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVTOOLS_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_DELEGATE_H_
#define CONTENT_RENDERER_DEVTOOLS_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_DELEGATE_H_

#include "content/common/content_export.h"

namespace blink {
struct WebDeviceEmulationParams;
}

namespace content {

struct VisualProperties;

// Consumers of RenderWidgetScreenMetricsEmulatorDelegate implement this
// delegate in order to transport emulation information across processes.
class CONTENT_EXPORT RenderWidgetScreenMetricsEmulatorDelegate {
 public:
  // Requests a full redraw of the contents of the renderer.
  virtual void Redraw() = 0;

  // Synchronize visual properties with the widget.
  virtual void SynchronizeVisualProperties(
      const VisualProperties& visual_properties) = 0;

  // Passes device emulation parameters to the delegate.
  virtual void SetScreenMetricsEmulationParameters(
      bool enabled,
      const blink::WebDeviceEmulationParams& params) = 0;

  // Passes new view bounds and window bounds in screen coordinates to the
  // delegate.
  virtual void SetScreenRects(const gfx::Rect& view_screen_rect,
                              const gfx::Rect& window_screen_rect) = 0;

 protected:
  virtual ~RenderWidgetScreenMetricsEmulatorDelegate() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_DEVTOOLS_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_DELEGATE_H_
