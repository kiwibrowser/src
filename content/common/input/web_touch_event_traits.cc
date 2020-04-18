// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/web_touch_event_traits.h"

#include <stddef.h>

#include "base/logging.h"
#include "third_party/blink/public/platform/web_touch_event.h"

using blink::WebInputEvent;
using blink::WebTouchEvent;
using blink::WebTouchPoint;

namespace content {

bool WebTouchEventTraits::AllTouchPointsHaveState(
    const WebTouchEvent& event,
    blink::WebTouchPoint::State state) {
  if (!event.touches_length)
    return false;
  for (size_t i = 0; i < event.touches_length; ++i) {
    if (event.touches[i].state != state)
      return false;
  }
  return true;
}

bool WebTouchEventTraits::IsTouchSequenceStart(const WebTouchEvent& event) {
  DCHECK(event.touches_length ||
         event.GetType() == WebInputEvent::kTouchScrollStarted);
  if (event.GetType() != WebInputEvent::kTouchStart)
    return false;
  return AllTouchPointsHaveState(event, blink::WebTouchPoint::kStatePressed);
}

bool WebTouchEventTraits::IsTouchSequenceEnd(const WebTouchEvent& event) {
  if (event.GetType() != WebInputEvent::kTouchEnd &&
      event.GetType() != WebInputEvent::kTouchCancel)
    return false;
  if (!event.touches_length)
    return true;
  for (size_t i = 0; i < event.touches_length; ++i) {
    if (event.touches[i].state != blink::WebTouchPoint::kStateReleased &&
        event.touches[i].state != blink::WebTouchPoint::kStateCancelled)
      return false;
  }
  return true;
}

void WebTouchEventTraits::ResetType(WebInputEvent::Type type,
                                    base::TimeTicks timestamp,
                                    WebTouchEvent* event) {
  DCHECK(WebInputEvent::IsTouchEventType(type));
  DCHECK(type != WebInputEvent::kTouchScrollStarted);

  event->SetType(type);
  event->dispatch_type = type == WebInputEvent::kTouchCancel
                             ? WebInputEvent::kEventNonBlocking
                             : WebInputEvent::kBlocking;
  event->SetTimeStamp(timestamp);
}

void WebTouchEventTraits::ResetTypeAndTouchStates(WebInputEvent::Type type,
                                                  base::TimeTicks timestamp,
                                                  WebTouchEvent* event) {
  ResetType(type, timestamp, event);

  WebTouchPoint::State newState = WebTouchPoint::kStateUndefined;
  switch (event->GetType()) {
    case WebInputEvent::kTouchStart:
      newState = WebTouchPoint::kStatePressed;
      break;
    case WebInputEvent::kTouchMove:
      newState = WebTouchPoint::kStateMoved;
      break;
    case WebInputEvent::kTouchEnd:
      newState = WebTouchPoint::kStateReleased;
      break;
    case WebInputEvent::kTouchCancel:
      newState = WebTouchPoint::kStateCancelled;
      break;
    default:
      NOTREACHED();
      break;
  }
  for (size_t i = 0; i < event->touches_length; ++i)
    event->touches[i].state = newState;
}

}  // namespace content
