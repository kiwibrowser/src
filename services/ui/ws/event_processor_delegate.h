// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_PROCESSOR_DELEGATE_H_
#define SERVICES_UI_WS_EVENT_PROCESSOR_DELEGATE_H_

#include <stdint.h>

#include "services/ui/common/types.h"

namespace gfx {
class PointF;
}

namespace viz {
class FrameSinkId;
class HitTestQuery;
}

namespace ui {

class Event;

namespace ws {

class ServerWindow;

// Notified of various state changes to EventProcessor. This is also used to
// obtain ServerWindows matching certain criteria (such as the ServerWindow for
// a display).
class EventProcessorDelegate {
 public:
  virtual void SetFocusedWindowFromEventProcessor(ServerWindow* window) = 0;
  virtual ServerWindow* GetFocusedWindowForEventProcessor(
      int64_t display_id) = 0;

  // Called when capture should be set on the native display. |window| is the
  // window capture is being set on.
  virtual void SetNativeCapture(ServerWindow* window) = 0;

  // Called when the native display is having capture released. There is no
  // longer a ServerWindow holding capture.
  virtual void ReleaseNativeCapture() = 0;

  // Called when EventProcessor has a new value for the cursor and our
  // delegate should perform the native updates.
  virtual void UpdateNativeCursorFromEventProcessor() = 0;

  // Called when |window| has lost capture. The native display may still be
  // holding capture. The delegate should not change native display capture.
  // ReleaseNativeCapture() is invoked if appropriate.
  virtual void OnCaptureChanged(ServerWindow* new_capture,
                                ServerWindow* old_capture) = 0;

  virtual void OnMouseCursorLocationChanged(const gfx::PointF& point,
                                            int64_t display_id) = 0;

  virtual void OnEventChangesCursorVisibility(const ui::Event& event,
                                              bool visible) = 0;

  virtual void OnEventChangesCursorTouchVisibility(const ui::Event& event,
                                                   bool visible) = 0;

  // Returns the id of the client to send events to. |in_nonclient_area| is
  // true if the event occurred in the non-client area of the window.
  virtual ClientSpecificId GetEventTargetClientId(const ServerWindow* window,
                                                  bool in_nonclient_area) = 0;

  // Returns the window to start searching from at the specified location, or
  // null if there is a no window containing |location_in_display|.
  // |location_in_display| is in display coordinates and in pixels.
  // |location_in_display| and |display_id| are updated if the window we
  // found is on a different display than the originated display.
  // TODO(riajiang): No need to update |location_in_display| and |display_id|
  // after ozone drm can tell us the right display the cursor is on for
  // drag-n-drop events. crbug.com/726470
  virtual ServerWindow* GetRootWindowForDisplay(int64_t display_id) = 0;

  // Returns the root of |window| that is used for event dispatch. The returned
  // value is used for coordinate conversion. Returns null if |window| is not
  // in a hierarchy that events should be dispatched to (typically not
  // associated with a display).
  virtual ServerWindow* GetRootWindowForEventDispatch(ServerWindow* window) = 0;

  // Called when event dispatch could not find a target. OnAccelerator may still
  // be called.
  virtual void OnEventTargetNotFound(const ui::Event& event,
                                     int64_t display_id) = 0;

  // If an event is blocked by a modal window this function is used to determine
  // which window the event should be dispatched to. Return null to indicate no
  // window. |window| is the window the event would be targetted at if there was
  // no modal window open.
  virtual ServerWindow* GetFallbackTargetForEventBlockedByModal(
      ServerWindow* window) = 0;

  // Called when an event occurs that targets a window that should be blocked
  // by a modal window. |modal_window| is the modal window that blocked the
  // event.
  virtual void OnEventOccurredOutsideOfModalWindow(
      ServerWindow* modal_window) = 0;

  virtual viz::HitTestQuery* GetHitTestQueryForDisplay(int64_t display_id) = 0;

  virtual ServerWindow* GetWindowFromFrameSinkId(
      const viz::FrameSinkId& frame_sink_id) = 0;

 protected:
  virtual ~EventProcessorDelegate() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_PROCESSOR_DELEGATE_H_
