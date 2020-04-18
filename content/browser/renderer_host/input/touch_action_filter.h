// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "cc/input/touch_action.h"
#include "content/common/content_export.h"

namespace blink {
class WebGestureEvent;
}

namespace content {

class MockRenderWidgetHost;

enum class FilterGestureEventResult {
  kFilterGestureEventAllowed,
  kFilterGestureEventFiltered
};

// The TouchActionFilter is responsible for filtering scroll and pinch gesture
// events according to the CSS touch-action values the renderer has sent for
// each touch point.
// For details see the touch-action design doc at http://goo.gl/KcKbxQ.
class CONTENT_EXPORT TouchActionFilter {
 public:
  TouchActionFilter();

  // Returns kFilterGestureEventFiltered if the supplied gesture event should be
  // dropped based on the current touch-action state. Otherwise returns
  // kFilterGestureEventAllowed, and possibly modifies the event's directional
  // parameters to make the event compatible with the effective touch-action.
  FilterGestureEventResult FilterGestureEvent(
      blink::WebGestureEvent* gesture_event);

  // Called when a set-touch-action message is received from the renderer
  // for a touch start event that is currently in flight.
  void OnSetTouchAction(cc::TouchAction touch_action);

  // Called at the end of a touch action sequence in order to log when a
  // whitelisted touch action is or is not equivalent to the allowed touch
  // action.
  void ReportAndResetTouchAction();

  // Must be called at least once between when the last gesture events for the
  // previous touch sequence have passed through the touch action filter and the
  // time the touch start for the next touch sequence has reached the
  // renderer. It may be called multiple times during this interval.
  void ResetTouchAction();

  // Called when a set-white-listed-touch-action message is received from the
  // renderer for a touch start event that is currently in flight.
  void OnSetWhiteListedTouchAction(cc::TouchAction white_listed_touch_action);

  cc::TouchAction allowed_touch_action() const { return allowed_touch_action_; }

  void SetForceEnableZoom(bool enabled) { force_enable_zoom_ = enabled; }

 private:
  friend class MockRenderWidgetHost;

  bool ShouldSuppressManipulation(const blink::WebGestureEvent&);
  bool FilterManipulationEventAndResetState();

  // Whether scroll and pinch gestures should be discarded due to touch-action.
  bool suppress_manipulation_events_;

  // Whether a tap ending event in this sequence should be discarded because a
  // previous GestureTapUnconfirmed event was turned into a GestureTap.
  bool drop_current_tap_ending_event_;

  // True iff the touch action of the last TapUnconfirmed or Tap event was
  // TOUCH_ACTION_AUTO. The double tap event depends on the touch action of the
  // previous tap or tap unconfirmed. Only valid between a TapUnconfirmed or Tap
  // and the next DoubleTap.
  bool allow_current_double_tap_event_;

  // Force enable zoom for Accessibility.
  bool force_enable_zoom_;

  // What touch actions are currently permitted.
  cc::TouchAction allowed_touch_action_;

  // Whitelisted touch action received from the compositor.
  base::Optional<cc::TouchAction> white_listed_touch_action_;

  DISALLOW_COPY_AND_ASSIGN(TouchActionFilter);
};

}
#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
