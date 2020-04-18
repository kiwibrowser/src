/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_WHEEL_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_WHEEL_EVENT_H_

#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/wheel_event_init.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"

namespace blink {

class CORE_EXPORT WheelEvent final : public MouseEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum { kTickMultiplier = 120 };

  enum DeltaMode { kDomDeltaPixel = 0, kDomDeltaLine, kDomDeltaPage };

  static WheelEvent* Create() { return new WheelEvent; }

  static WheelEvent* Create(const WebMouseWheelEvent& native_event,
                            AbstractView*);

  static WheelEvent* Create(const AtomicString& type,
                            const WheelEventInit& initializer) {
    return new WheelEvent(type, initializer);
  }

  double deltaX() const { return delta_x_; }  // Positive when scrolling right.
  double deltaY() const { return delta_y_; }  // Positive when scrolling down.
  double deltaZ() const { return delta_z_; }
  int wheelDelta() const {
    return wheelDeltaY() ? wheelDeltaY() : wheelDeltaX();
  }  // Deprecated.
  int wheelDeltaX() const {
    return wheel_delta_.X();
  }  // Deprecated, negative when scrolling right.
  int wheelDeltaY() const {
    return wheel_delta_.Y();
  }  // Deprecated, negative when scrolling down.
  unsigned deltaMode() const { return delta_mode_; }

  const AtomicString& InterfaceName() const override;
  bool IsMouseEvent() const override;
  bool IsWheelEvent() const override;

  const WebMouseWheelEvent& NativeEvent() const { return native_event_; }

  // WheelEvent doesn't modify the event path, but its parent MouseEvent does.
  // So we need to override its parent's DispatchEvent.
  DispatchEventResult DispatchEvent(EventDispatcher&) override;

  void Trace(blink::Visitor*) override;

 private:
  WheelEvent();
  WheelEvent(const AtomicString&, const WheelEventInit&);
  WheelEvent(const WebMouseWheelEvent&, AbstractView*);

  IntPoint wheel_delta_;
  double delta_x_;
  double delta_y_;
  double delta_z_;
  unsigned delta_mode_;
  WebMouseWheelEvent native_event_;
};

DEFINE_EVENT_TYPE_CASTS(WheelEvent);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_WHEEL_EVENT_H_
