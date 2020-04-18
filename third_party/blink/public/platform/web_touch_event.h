// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_TOUCH_EVENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_TOUCH_EVENT_H_

#include "third_party/blink/public/platform/web_input_event.h"

namespace blink {

// See WebInputEvent.h for details why this pack is here.
#pragma pack(push, 4)

// WebTouchEvent --------------------------------------------------------------

// TODO(e_hakkinen): Replace with WebPointerEvent. crbug.com/508283
class WebTouchEvent : public WebInputEvent {
 public:
  // Maximum number of simultaneous touches supported on
  // Ash/Aura.
  enum { kTouchesLengthCap = 16 };

  unsigned touches_length;
  // List of all touches, regardless of state.
  WebTouchPoint touches[kTouchesLengthCap];

  // Whether the event is blocking, non-blocking, all event
  // listeners were passive or was forced to be non-blocking.
  DispatchType dispatch_type;

  // For a single touch, this is true after the touch-point has moved beyond
  // the platform slop region. For a multitouch, this is true after any
  // touch-point has moved (by whatever amount).
  bool moved_beyond_slop_region;

  // True for events from devices like some pens that support hovering
  // over digitizer and the events are sent while the device was hovering.
  bool hovering;

  // Whether this touch event is a touchstart or a first touchmove event per
  // scroll.
  bool touch_start_or_first_touch_move;

  // A unique identifier for the touch event. Valid ids start at one and
  // increase monotonically. Zero means an unknown id.
  uint32_t unique_touch_event_id;

  WebTouchEvent()
      : WebInputEvent(sizeof(WebTouchEvent)), dispatch_type(kBlocking) {}

  WebTouchEvent(Type type, int modifiers, base::TimeTicks time_stamp)
      : WebInputEvent(sizeof(WebTouchEvent), type, modifiers, time_stamp),
        dispatch_type(kBlocking) {}

#if INSIDE_BLINK

  // Sets any scaled values to be their computed values and sets |frame_scale_|
  // back to 1 and |frame_translate_| X and Y coordinates back to 0.
  BLINK_PLATFORM_EXPORT WebTouchEvent FlattenTransform() const;

  // Return a scaled WebTouchPoint in root frame coordinates.
  BLINK_PLATFORM_EXPORT WebTouchPoint
  TouchPointInRootFrame(unsigned touch_point) const;

  bool IsCancelable() const { return dispatch_type == kBlocking; }
#endif
};

#pragma pack(pop)

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_TOUCH_EVENT_H_
