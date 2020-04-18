// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_POINTER_EVENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_POINTER_EVENT_H_

#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"
#include "third_party/blink/public/platform/web_pointer_properties.h"
#include "third_party/blink/public/platform/web_touch_event.h"

namespace blink {

// See WebInputEvent.h for details why this pack is here.
#pragma pack(push, 4)

// WebPointerEvent
// This is a WIP and currently used only in Blink and only for touch.
// TODO(nzolghadr): We should unify the fields in this class into
// WebPointerProperties and not have pointertype specific attributes here.
// --------------------------------------------------------------

class WebPointerEvent : public WebInputEvent, public WebPointerProperties {
 public:
  WebPointerEvent()
      : WebInputEvent(sizeof(WebPointerEvent)), WebPointerProperties(0) {}
  WebPointerEvent(WebInputEvent::Type type_param,
                  WebPointerProperties web_pointer_properties_param,
                  float width_param,
                  float height_param)
      : WebInputEvent(sizeof(WebPointerEvent)),
        WebPointerProperties(web_pointer_properties_param),
        width(width_param),
        height(height_param) {
    SetType(type_param);
  }
  BLINK_PLATFORM_EXPORT WebPointerEvent(const WebTouchEvent&,
                                        const WebTouchPoint&);
  BLINK_PLATFORM_EXPORT WebPointerEvent(WebInputEvent::Type,
                                        const WebMouseEvent&);

  BLINK_PLATFORM_EXPORT static WebPointerEvent CreatePointerCausesUaActionEvent(
      WebPointerProperties::PointerType,
      base::TimeTicks time_stamp);

  // ------------ Touch Point Specific ------------

  float rotation_angle;

  // ------------ Touch Event Specific ------------

  // A unique identifier for the touch event. Valid ids start at one and
  // increase monotonically. Zero means an unknown id.
  uint32_t unique_touch_event_id;

  // Whether the event is blocking, non-blocking, all event
  // listeners were passive or was forced to be non-blocking.
  DispatchType dispatch_type;

  // For a single touch, this is true after the touch-point has moved beyond
  // the platform slop region. For a multitouch, this is true after any
  // touch-point has moved (by whatever amount).
  bool moved_beyond_slop_region;

  // Whether this touch event is a touchstart or a first touchmove event per
  // scroll.
  bool touch_start_or_first_touch_move;

  // ------------ Common fields across pointer types ------------

  // True if this pointer was hovering and false otherwise. False value entails
  // the event was processed as part of gesture detection and it may cause
  // scrolling.
  bool hovering;

  // TODO(crbug.com/736014): We need a clarified definition of the scale and
  // the coordinate space on these attributes.
  float width;
  float height;

#if INSIDE_BLINK
  bool IsCancelable() const { return dispatch_type == kBlocking; }
  bool HasWidth() const { return !std::isnan(width); }
  bool HasHeight() const { return !std::isnan(height); }

  BLINK_PLATFORM_EXPORT WebPointerEvent WebPointerEventInRootFrame() const;

#endif
};

#pragma pack(pop)

}  // namespace blink

#endif  // WebMouseEvent_h
