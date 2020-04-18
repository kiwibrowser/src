// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/util.h"

#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/macros.h"

namespace gestures {

void CombineButtonsGestures(Gesture* gesture, const Gesture* addend);

// Returns a priority for a given gesture type. Lower number is more important.
int CombineGesturePriority(const Gesture* gesture) {
  int priority = 0;

  // Use a switch with no default so that if a new gesture type is added,
  // we get a compiler warning here.
  // Least important at top
  switch (gesture->type) {
    case kGestureTypeNull: priority++;  // fallthrough
    case kGestureTypeContactInitiated: priority++;  // fallthrough

    // Midrange, equal priority
    case kGestureTypeFourFingerSwipe:  // fallthrough
    case kGestureTypePinch:  // fallthrough
    case kGestureTypeSwipe:  // fallthrough
    case kGestureTypeMove:  // fallthrough
    case kGestureTypeScroll: priority++;  // fallthrough

    case kGestureTypeFourFingerSwipeLift: priority++;  // fallthrough
    case kGestureTypeFling: priority++;  // fallthrough
    case kGestureTypeSwipeLift: priority++;  // fallthrough
    case kGestureTypeButtonsChange: priority++;  // fallthrough
    case kGestureTypeMetrics: priority++;  // fallthrough
  }
  // Most important at bottom

  return priority;
}

void CombineGestures(Gesture* gesture, const Gesture* addend) {
  if (!gesture) {
    Err("gesture must be non-NULL.");
    return;
  }
  if (!addend)
    return;
  if (gesture->type == kGestureTypeNull) {
    *gesture = *addend;
    return;
  }
  if (gesture->type == addend->type) {
    // Same type; merge them
    switch (gesture->type) {
      case kGestureTypeNull:  // fallthrough
      case kGestureTypeContactInitiated:  // fallthrough
      case kGestureTypeFling:
      case kGestureTypeSwipeLift:
      case kGestureTypeFourFingerSwipeLift:
      case kGestureTypeMetrics:
        break;
      case kGestureTypeButtonsChange:
        CombineButtonsGestures(gesture, addend);
        break;
        // These we actually combine:
      case kGestureTypeMove:
        gesture->details.move.dx += addend->details.move.dx;
        gesture->details.move.dy += addend->details.move.dy;
        break;
      case kGestureTypeScroll:
        gesture->details.scroll.dx += addend->details.scroll.dx;
        gesture->details.scroll.dy += addend->details.scroll.dy;
        break;
      case kGestureTypeSwipe:
        gesture->details.swipe.dx += addend->details.swipe.dx;
        gesture->details.swipe.dy += addend->details.swipe.dy;
        break;
      case kGestureTypeFourFingerSwipe:
        gesture->details.four_finger_swipe.dx +=
            addend->details.four_finger_swipe.dx;
        gesture->details.four_finger_swipe.dy +=
            addend->details.four_finger_swipe.dy;
        break;
      case kGestureTypePinch:
        gesture->details.pinch.dz *= addend->details.pinch.dz;
        gesture->details.pinch.zoom_state = addend->details.pinch.zoom_state;
        break;
    }
    gesture->start_time = std::min(gesture->start_time, addend->start_time);
    gesture->end_time = std::max(gesture->end_time, addend->end_time);
    return;
  }
  if (CombineGesturePriority(gesture) < CombineGesturePriority(addend)) {
    Log("Losing gesture");  // losing 'addend'
    return;
  }
  Log("Losing gesture");  // losing 'gesture'
  *gesture = *addend;
}

void CombineButtonsGestures(Gesture* gesture, const Gesture* addend) {
  // We have 2 button events. merge them
  unsigned buttons[] = { GESTURES_BUTTON_LEFT,
                         GESTURES_BUTTON_MIDDLE,
                         GESTURES_BUTTON_RIGHT };
  for (size_t i = 0; i < arraysize(buttons); ++i) {
    unsigned button = buttons[i];
    unsigned g_down = gesture->details.buttons.down & button;
    unsigned g_up = gesture->details.buttons.up & button;
    unsigned a_down = addend->details.buttons.down & button;
    unsigned a_up = addend->details.buttons.up & button;
    // How we merge buttons: Remember that a button gesture event can send
    // some button down events, then button up events. Ideally we can combine
    // them simply: e.g. if |gesture| has button down and |addend| has button
    // up, we can put those both into |gesture|. If there is a conflict (e.g.
    // button up followed by button down/up), there is no proper way to
    // represent that in a single gesture. We work around that case by removing
    // pairs of down/up, so in the example just given, the result would be just
    // button up. There is one exception to these two rules: if |gesture| is
    // button up, and |addend| is button down, combing them into one gesture
    // would mean a click, because when executing the gestures, the down
    // actions happen before the up. So for that case, we just remove all
    // button action.
    if (!g_down && g_up && a_down && !a_up) {
      // special case
      g_down = 0;
      g_up = 0;
    } else if ((g_down & a_down) | (g_up & a_up)) {
      // If we have a conflict, this logic seems to remove the full click.
      g_down = (~(g_down ^ a_down)) & button;
      g_up = (~(g_up ^ a_up)) & button;
    } else {
      // Non-conflict case
      g_down |= a_down;
      g_up |= a_up;
    }
    gesture->details.buttons.down =
        (gesture->details.buttons.down & ~button) | g_down;
    gesture->details.buttons.up =
        (gesture->details.buttons.up & ~button) | g_up;
  }
  if (!gesture->details.buttons.down && !gesture->details.buttons.up)
    *gesture = Gesture();
}

}  // namespace gestures
