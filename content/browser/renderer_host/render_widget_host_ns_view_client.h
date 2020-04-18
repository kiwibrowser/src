// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_CLIENT_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_CLIENT_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/common/edit_command.h"
#include "content/common/mac/attributed_string_coder.h"
#include "ui/base/ime/ime_text_span.h"
#include "ui/base/ime/text_input_type.h"

namespace blink {
class WebGestureEvent;
class WebMouseEvent;
class WebMouseWheelEvent;
}  // namespace blink

namespace display {
class Display;
}  // namespace display

namespace gfx {
class PointF;
class Range;
class Rect;
}  // namespace gfx

namespace ui {
class LatencyInfo;
}  // namespace ui

namespace content {

class RenderWidgetHostViewMac;

// The interface through which the NSView for a RenderWidgetHostViewMac is to
// communicate with the RenderWidgetHostViewMac (potentially in another
// process).
class RenderWidgetHostNSViewClient {
 public:
  RenderWidgetHostNSViewClient() {}
  virtual ~RenderWidgetHostNSViewClient() {}

  // Return the RenderWidget's BrowserAccessibilityManager.
  // TODO(ccameron): This returns nullptr for non-local NSViews. A scheme for
  // non-local accessibility needs to be developed.
  virtual BrowserAccessibilityManager* GetRootBrowserAccessibilityManager() = 0;

  // Synchronously query if there exists a RenderViewHost for the corresponding
  // RenderWidgetHostView's RenderWidgetHost, and store the result in
  // |*is_render_view|.
  virtual void OnNSViewSyncIsRenderViewHost(bool* is_render_view) = 0;

  // Indicates that the RenderWidgetHost is to shut down.
  virtual void OnNSViewRequestShutdown() = 0;

  // Indicates whether or not the NSView is its NSWindow's first responder.
  virtual void OnNSViewIsFirstResponderChanged(bool is_first_responder) = 0;

  // Indicates whether or not the NSView's NSWindow is key.
  virtual void OnNSViewWindowIsKeyChanged(bool is_key) = 0;

  // Indicates the NSView's bounds in its NSWindow's DIP coordinate system (with
  // the origin at the upper-left corner), and indicate if the the NSView is
  // attached to an NSWindow (if it is not, then |view_bounds_in_window_dip|'s
  // origin is meaningless, but its size is still relevant).
  virtual void OnNSViewBoundsInWindowChanged(
      const gfx::Rect& view_bounds_in_window_dip,
      bool attached_to_window) = 0;

  // Indicates the NSView's NSWindow's frame in the global display::Screen
  // DIP coordinate system (where the origin the upper-left corner of
  // Screen::GetPrimaryDisplay).
  virtual void OnNSViewWindowFrameInScreenChanged(
      const gfx::Rect& window_frame_in_screen_dip) = 0;

  // Indicate the NSView's NSScreen's properties.
  virtual void OnNSViewDisplayChanged(const display::Display& display) = 0;

  // Indicate the begin and end block of a keyboard event. The beginning of this
  // block will record the active RenderWidgetHost, and will forward all
  // remaining keyboard and Ime messages to that RenderWidgetHost.
  virtual void OnNSViewBeginKeyboardEvent() = 0;
  virtual void OnNSViewEndKeyboardEvent() = 0;

  // Forward a keyboard event to the RenderWidgetHost that is currently handling
  // the key-down event.
  virtual void OnNSViewForwardKeyboardEvent(
      const NativeWebKeyboardEvent& key_event,
      const ui::LatencyInfo& latency_info) = 0;
  virtual void OnNSViewForwardKeyboardEventWithCommands(
      const NativeWebKeyboardEvent& key_event,
      const ui::LatencyInfo& latency_info,
      const std::vector<EditCommand>& commands) = 0;

  // Forward events to the renderer or the input router, as appropriate.
  virtual void OnNSViewRouteOrProcessMouseEvent(
      const blink::WebMouseEvent& web_event) = 0;
  virtual void OnNSViewRouteOrProcessWheelEvent(
      const blink::WebMouseWheelEvent& web_event) = 0;

  // Special case forwarding of synthetic events to the renderer.
  virtual void OnNSViewForwardMouseEvent(
      const blink::WebMouseEvent& web_event) = 0;
  virtual void OnNSViewForwardWheelEvent(
      const blink::WebMouseWheelEvent& web_event) = 0;

  // Handling pinch gesture events.
  virtual void OnNSViewGestureBegin(blink::WebGestureEvent begin_event,
                                    bool is_synthetically_injected) = 0;
  virtual void OnNSViewGestureUpdate(blink::WebGestureEvent update_event) = 0;
  virtual void OnNSViewGestureEnd(blink::WebGestureEvent end_event) = 0;
  virtual void OnNSViewSmartMagnify(
      const blink::WebGestureEvent& smart_magnify_event) = 0;

  // Forward the corresponding Ime commands to the appropriate RenderWidgetHost.
  // Appropriate, has two meanings here. If this is during a key-down event,
  // then the target is the RWH that is handling that key-down event. Otherwise,
  // it is the result of GetActiveWidget.
  virtual void OnNSViewImeSetComposition(
      const base::string16& text,
      const std::vector<ui::ImeTextSpan>& ime_text_spans,
      const gfx::Range& replacement_range,
      int selection_start,
      int selection_end) = 0;
  virtual void OnNSViewImeCommitText(const base::string16& text,
                                     const gfx::Range& replacement_range) = 0;
  virtual void OnNSViewImeFinishComposingText() = 0;
  virtual void OnNSViewImeCancelComposition() = 0;

  // Request an overlay dictionary be displayed for the text at the specified
  // point.
  virtual void OnNSViewLookUpDictionaryOverlayAtPoint(
      const gfx::PointF& root_point) = 0;

  // Request an overlay dictionary be displayed for the text in the the
  // specified character range.
  virtual void OnNSViewLookUpDictionaryOverlayFromRange(
      const gfx::Range& range) = 0;

  // Synchronously query the character index for |root_point| and return it in
  // |*index|. Sets it to UINT32_MAX if the request fails or is not completed.
  virtual void OnNSViewSyncGetCharacterIndexAtPoint(
      const gfx::PointF& root_point,
      uint32_t* index) = 0;

  // Synchronously query the composition character boundary rectangle and return
  // it in |*rect|. Set |*actual_range| to the range actually used for the
  // returned rectangle. If there was no focused RenderWidgetHost to query,
  // then set |*success| to false.
  virtual void OnNSViewSyncGetFirstRectForRange(
      const gfx::Range& requested_range,
      gfx::Rect* rect,
      gfx::Range* actual_range,
      bool* success) = 0;

  // Forward the corresponding edit menu command to the RenderWidgetHost's
  // delegate.
  virtual void OnNSViewExecuteEditCommand(const std::string& command) = 0;
  virtual void OnNSViewUndo() = 0;
  virtual void OnNSViewRedo() = 0;
  virtual void OnNSViewCut() = 0;
  virtual void OnNSViewCopy() = 0;
  virtual void OnNSViewCopyToFindPboard() = 0;
  virtual void OnNSViewPaste() = 0;
  virtual void OnNSViewPasteAndMatchStyle() = 0;
  virtual void OnNSViewSelectAll() = 0;

  // Speak the selected text of the appropriate RenderWidgetHostView using
  // TextServicesContextMenu.
  virtual void OnNSViewSpeakSelection() = 0;

  // Stop speaking using TextServicesContextMenu.
  virtual void OnNSViewStopSpeaking() = 0;

  // Synchronously query if TextServicesContextMenu is currently speaking and
  // store the result in |*is_speaking|.
  virtual void OnNSViewSyncIsSpeaking(bool* is_speaking) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostNSViewClient);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_CLIENT_H_
