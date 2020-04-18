/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_WIDGET_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_WIDGET_H_

#include "base/callback.h"
#include "base/time/time.h"
#include "third_party/blink/public/platform/web_browser_controls_state.h"
#include "third_party/blink/public/platform/web_canvas.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_float_size.h"
#include "third_party/blink/public/platform/web_input_event_result.h"
#include "third_party/blink/public/platform/web_menu_source_type.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_text_input_info.h"
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "third_party/blink/public/web/web_ime_text_span.h"
#include "third_party/blink/public/web/web_range.h"
#include "third_party/blink/public/web/web_text_direction.h"

class SkBitmap;

namespace blink {

class WebCoalescedInputEvent;
class WebLayerTreeView;
class WebPagePopup;
struct WebPoint;

class WebWidget {
 public:
  // This method closes and deletes the WebWidget.
  virtual void Close() {}

  // Returns the current size of the WebWidget.
  virtual WebSize Size() { return WebSize(); }

  // Called to resize the WebWidget.
  virtual void Resize(const WebSize&) {}

  // Resizes the unscaled visual viewport. Normally the unscaled visual
  // viewport is the same size as the main frame. The passed size becomes the
  // size of the viewport when unscaled (i.e. scale = 1). This is used to
  // shrink the visible viewport to allow things like the ChromeOS virtual
  // keyboard to overlay over content but allow scrolling it into view.
  virtual void ResizeVisualViewport(const WebSize&) {}

  // Called to notify the WebWidget of entering/exiting fullscreen mode.
  virtual void DidEnterFullscreen() {}
  virtual void DidExitFullscreen() {}

  // TODO(crbug.com/704763): Remove the need for this.
  virtual void SetSuppressFrameRequestsWorkaroundFor704763Only(bool) {}

  // Called to update imperative animation state. This should be called before
  // paint, although the client can rate-limit these calls.
  // |lastFrameTimeMonotonic| is in seconds.
  virtual void BeginFrame(base::TimeTicks last_frame_time) {}

  // Called to run through the entire set of document lifecycle phases needed
  // to render a frame of the web widget. This MUST be called before Paint,
  // and it may result in calls to WebWidgetClient::didInvalidateRect.
  virtual void UpdateAllLifecyclePhases() { UpdateLifecycle(); }

  // Selectively runs all lifecycle phases or all phases excluding paint. The
  // latter can be used to trigger side effects of updating layout and
  // animations if painting is not required.
  enum class LifecycleUpdate { kPrePaint, kAll };
  virtual void UpdateLifecycle(
      LifecycleUpdate requested_update = LifecycleUpdate::kAll) {}

  // Performs the complete set of document lifecycle phases, including updates
  // to the compositor state except rasterization.
  virtual void UpdateAllLifecyclePhasesAndCompositeForTesting() {}

  // Synchronously rasterizes and composites a frame.
  virtual void CompositeWithRasterForTesting() {}

  // Called to paint the rectangular region within the WebWidget
  // onto the specified canvas at (viewPort.x,viewPort.y).
  //
  // Before calling Paint(), you must call
  // UpdateLifecycle(LifecycleUpdate::All): this method assumes the lifecycle is
  // clean. It is okay to call paint multiple times once the lifecycle is
  // updated, assuming no other changes are made to the WebWidget (e.g., once
  // events are processed, it should be assumed that another call to
  // UpdateLifecycle is warranted before painting again).
  virtual void Paint(WebCanvas*, const WebRect& view_port) {}

  // Similar to paint() but ignores compositing decisions, squashing all
  // contents of the WebWidget into the output given to the WebCanvas.
  //
  // Before calling PaintIgnoringCompositing(), you must call
  // UpdateLifecycle(LifecycleUpdate::All): this method assumes the lifecycle is
  // clean.
  virtual void PaintIgnoringCompositing(WebCanvas*, const WebRect&) {}

  // Run layout and paint of all pending document changes asynchronously.
  virtual void LayoutAndPaintAsync(base::OnceClosure callback) {}

  // This should only be called when isAcceleratedCompositingActive() is true.
  virtual void CompositeAndReadbackAsync(
      base::OnceCallback<void(const SkBitmap&)> callback) {}

  // Called to inform the WebWidget of a change in theme.
  // Implementors that cache rendered copies of widgets need to re-render
  // on receiving this message
  virtual void ThemeChanged() {}

  // Do a hit test at given point and return the WebHitTestResult.
  virtual WebHitTestResult HitTestResultAt(const WebPoint&) {
    return WebHitTestResult();
  }

  // Called to inform the WebWidget of an input event.
  virtual WebInputEventResult HandleInputEvent(const WebCoalescedInputEvent&) {
    return WebInputEventResult::kNotHandled;
  }

  // Send any outstanding touch events. Touch events need to be grouped together
  // and any changes since the last time a touch event is going to be sent in
  // the new touch event.
  virtual WebInputEventResult DispatchBufferedTouchEvents() {
    return WebInputEventResult::kNotHandled;
  }

  // Called to inform the WebWidget of the mouse cursor's visibility.
  virtual void SetCursorVisibilityState(bool is_visible) {}

  // Applies viewport related properties during a commit from the compositor
  // thread.
  virtual void ApplyViewportDeltas(const WebFloatSize& visual_viewport_delta,
                                   const WebFloatSize& layout_viewport_delta,
                                   const WebFloatSize& elastic_overscroll_delta,
                                   float scale_factor,
                                   float browser_controls_shown_ratio_delta) {}

  virtual void RecordWheelAndTouchScrollingCount(bool has_scrolled_by_wheel,
                                                 bool has_scrolled_by_touch) {}

  // Called to inform the WebWidget that mouse capture was lost.
  virtual void MouseCaptureLost() {}

  // Called to inform the WebWidget that it has gained or lost keyboard focus.
  virtual void SetFocus(bool) {}

  // Returns the anchor and focus bounds of the current selection.
  // If the selection range is empty, it returns the caret bounds.
  virtual bool SelectionBounds(WebRect& anchor, WebRect& focus) const {
    return false;
  }

  // Returns true if the WebWidget is currently animating a GestureFling.
  virtual bool IsFlinging() const { return false; }

  // Returns true if the WebWidget uses GPU accelerated compositing
  // to render its contents.
  virtual bool IsAcceleratedCompositingActive() const { return false; }

  // Returns true if the WebWidget created is of type WebView.
  virtual bool IsWebView() const { return false; }

  // Returns true if the WebWidget created is of type PepperWidget.
  virtual bool IsPepperWidget() const { return false; }

  // Returns true if the WebWidget created is of type WebFrameWidget.
  virtual bool IsWebFrameWidget() const { return false; }

  // Returns true if the WebWidget created is of type WebPagePopup.
  virtual bool IsPagePopup() const { return false; }

  // The WebLayerTreeView initialized on this WebWidgetClient will be going away
  // and is no longer safe to access.
  virtual void WillCloseLayerTreeView() {}

  // Calling WebWidgetClient::requestPointerLock() will result in one
  // return call to didAcquirePointerLock() or didNotAcquirePointerLock().
  virtual void DidAcquirePointerLock() {}
  virtual void DidNotAcquirePointerLock() {}

  // Pointer lock was held, but has been lost. This may be due to a
  // request via WebWidgetClient::requestPointerUnlock(), or for other
  // reasons such as the user exiting lock, window focus changing, etc.
  virtual void DidLosePointerLock() {}

  // The page background color. Can be used for filling in areas without
  // content.
  virtual SkColor BackgroundColor() const {
    return 0xFFFFFFFF; /* SK_ColorWHITE */
  }

  // The currently open page popup, which are calendar and datalist pickers
  // but not the select popup.
  virtual WebPagePopup* GetPagePopup() const { return 0; }

  // Updates browser controls constraints and current state. Allows embedder to
  // control what are valid states for browser controls and if it should
  // animate.
  virtual void UpdateBrowserControlsState(WebBrowserControlsState constraints,
                                          WebBrowserControlsState current,
                                          bool animate) {}

  // Called by client to request showing the context menu.
  virtual void ShowContextMenu(WebMenuSourceType) {}

 protected:
  ~WebWidget() = default;
};

}  // namespace blink

#endif
