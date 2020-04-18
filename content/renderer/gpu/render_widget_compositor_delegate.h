// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_DELEGATE_H_
#define CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/time/time.h"
#include "cc/trees/layer_tree_host_client.h"
#include "content/common/content_export.h"

namespace cc {
class LayerTreeFrameSink;
class SwapPromise;
}

namespace gfx {
class Vector2dF;
}

namespace viz {
class CopyOutputRequest;
}

namespace content {

using LayerTreeFrameSinkCallback =
    base::Callback<void(std::unique_ptr<cc::LayerTreeFrameSink>)>;

// Consumers of RenderWidgetCompositor implement this delegate in order to
// transport compositing information across processes.
class CONTENT_EXPORT RenderWidgetCompositorDelegate {
 public:
  // Report viewport related properties during a commit from the compositor
  // thread.
  virtual void ApplyViewportDeltas(
      const gfx::Vector2dF& inner_delta,
      const gfx::Vector2dF& outer_delta,
      const gfx::Vector2dF& elastic_overscroll_delta,
      float page_scale,
      float top_controls_delta) = 0;

  // Record use count of wheel/touch sources for scrolling on the compositor
  // thread.
  virtual void RecordWheelAndTouchScrollingCount(
      bool has_scrolled_by_wheel,
      bool has_scrolled_by_touch) = 0;

  // Notifies that the compositor has issed a BeginMainFrame.
  virtual void BeginMainFrame(base::TimeTicks frame_time) = 0;

  // Requests a LayerTreeFrameSink to submit CompositorFrames to.
  virtual void RequestNewLayerTreeFrameSink(
      const LayerTreeFrameSinkCallback& callback) = 0;

  // Notifies that the draw commands for a committed frame have been issued.
  virtual void DidCommitAndDrawCompositorFrame() = 0;

  // Notifies about a compositor frame commit operation having finished.
  virtual void DidCommitCompositorFrame() = 0;

  // Called by the compositor when page scale animation completed.
  virtual void DidCompletePageScaleAnimation() = 0;

  // Notifies that the last submitted CompositorFrame has been processed and
  // will be displayed.
  virtual void DidReceiveCompositorFrameAck() = 0;

  // Indicates whether the RenderWidgetCompositor is about to close.
  virtual bool IsClosing() const = 0;

  // Requests that the client schedule a composite now, and calculate
  // appropriate delay for potential future frame.
  virtual void RequestScheduleAnimation() = 0;

  // Requests a visual frame-based update to the state of the delegate if there
  // an update available.
  using VisualStateUpdate = cc::LayerTreeHostClient::VisualStateUpdate;
  virtual void UpdateVisualState(VisualStateUpdate requested_update) = 0;

  // Indicates that the compositor is about to begin a frame. This is primarily
  // to signal to flow control mechanisms that a frame is beginning, not to
  // perform actual painting work.
  virtual void WillBeginCompositorFrame() = 0;

  // For use in layout test mode only, attempts to copy the full content of the
  // compositor.
  virtual std::unique_ptr<cc::SwapPromise> RequestCopyOfOutputForLayoutTest(
      std::unique_ptr<viz::CopyOutputRequest> request) = 0;

 protected:
  virtual ~RenderWidgetCompositorDelegate() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_DELEGATE_H_
