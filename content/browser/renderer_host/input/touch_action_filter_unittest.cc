// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touch_action_filter.h"
#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "content/public/common/input_event_ack_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_input_event.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;

namespace content {
namespace {

const blink::WebGestureDevice kSourceDevice =
    blink::kWebGestureDeviceTouchscreen;

}  // namespace

static void PanTest(cc::TouchAction action,
                    float scroll_x,
                    float scroll_y,
                    float dx,
                    float dy,
                    float fling_x,
                    float fling_y,
                    float expected_dx,
                    float expected_dy,
                    float expected_fling_x,
                    float expected_fling_y) {
  TouchActionFilter filter;
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  {
    // Scrolls with no direction hint are permitted in the |action| direction.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);

    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(0, 0, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(expected_dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(expected_dy, scroll_update.data.scroll_update.delta_y);

    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
  }

  {
    // Scrolls biased towards the touch-action axis are permitted.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_x, scroll_y,
                                                          kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(expected_dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(expected_dy, scroll_update.data.scroll_update.delta_y);

    // Ensure that scrolls in the opposite direction are not filtered once
    // scrolling has started. (Once scrolling is started, the direction may
    // be reversed by the user even if scrolls that start in the reversed
    // direction are disallowed.
    WebGestureEvent scroll_update2 =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(-dx, -dy, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update2),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(-expected_dx, scroll_update2.data.scroll_update.delta_x);
    EXPECT_EQ(-expected_dy, scroll_update2.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        fling_x, fling_y, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(expected_fling_x, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(expected_fling_y, fling_start.data.fling_start.velocity_y);
  }

  {
    // Scrolls biased towards the perpendicular of the touch-action axis are
    // suppressed entirely.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_y, scroll_x,
                                                          kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(dy, scroll_update.data.scroll_update.delta_y);

    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
  }
}

static void PanTestForUnidirectionalTouchAction(cc::TouchAction action,
                                                float scroll_x,
                                                float scroll_y) {
  TouchActionFilter filter;
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  {
    // Scrolls towards the touch-action direction are permitted.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_x, scroll_y,
                                                          kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(scroll_x, scroll_y,
                                                           0, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
  }

  {
    // Scrolls towards the exact opposite of the touch-action direction are
    // suppressed entirely.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-scroll_x, -scroll_y,
                                                          kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(-scroll_x, -scroll_y,
                                                           0, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
  }

  {
    // Scrolls towards the diagonal opposite of the touch-action direction are
    // suppressed entirely.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(
            -scroll_x - scroll_y, -scroll_x - scroll_y, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(
            -scroll_x - scroll_y, -scroll_x - scroll_y, 0, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
  }
}

TEST(TouchActionFilterTest, SimpleFilter) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 10;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // No events filtered by default.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // cc::kTouchActionAuto doesn't cause any filtering.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // cc::kTouchActionNone filters out all scroll events, but no other events.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);

  // When a new touch sequence begins, the state is reset.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // Setting touch action doesn't impact any in-progress gestures.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // And the state is still cleared for the next gesture.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // Changing the touch action during a gesture has no effect.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);
}

TEST(TouchActionFilterTest, Fling) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(5, 10, 0,
                                                         kSourceDevice);
  const float kFlingX = 7;
  const float kFlingY = -4;
  WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
      kFlingX, kFlingY, kSourceDevice);
  WebGestureEvent pad_fling = SyntheticWebGestureEventBuilder::BuildFling(
      kFlingX, kFlingY, blink::kWebGestureDeviceTouchpad);

  // cc::kTouchActionNone filters out fling events.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
  EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);

  // touchpad flings aren't filtered.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&pad_fling),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
            FilterGestureEventResult::kFilterGestureEventFiltered);
}

TEST(TouchActionFilterTest, PanLeft) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanLeft, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, kDX, 0, kFlingX, 0);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanLeft, kScrollX, 0);
}

TEST(TouchActionFilterTest, PanRight) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = -7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanRight, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, kDX, 0, kFlingX, 0);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanRight, kScrollX, 0);
}

TEST(TouchActionFilterTest, PanX) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanX, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          kDX, 0, kFlingX, 0);
}

TEST(TouchActionFilterTest, PanUp) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = 7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanUp, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          0, kDY, 0, kFlingY);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanUp, 0, kScrollY);
}

TEST(TouchActionFilterTest, PanDown) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = -7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanDown, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, 0, kDY, 0, kFlingY);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanDown, 0, kScrollY);
}

TEST(TouchActionFilterTest, PanY) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = 7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanY, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          0, kDY, 0, kFlingY);
}

TEST(TouchActionFilterTest, PanXY) {
  TouchActionFilter filter;
  const float kDX = 5;
  const float kDY = 10;
  const float kFlingX = 7;
  const float kFlingY = -4;

  {
    // Scrolls hinted in the X axis are permitted and unmodified.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-7, 6, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(kDX, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(kDY, scroll_update.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);
  }

  {
    // Scrolls hinted in the Y axis are permitted and unmodified.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(kDX, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(kDY, scroll_update.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);
  }

  {
    // A two-finger gesture is not allowed.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice,
                                                          2);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventFiltered);
  }
}

TEST(TouchActionFilterTest, BitMath) {
  // Verify that the simple flag mixing properties we depend on are now
  // trivially true.
  EXPECT_EQ(cc::kTouchActionNone, cc::kTouchActionNone & cc::kTouchActionAuto);
  EXPECT_EQ(cc::kTouchActionNone, cc::kTouchActionPanY & cc::kTouchActionPanX);
  EXPECT_EQ(cc::kTouchActionPan, cc::kTouchActionAuto & cc::kTouchActionPan);
  EXPECT_EQ(cc::kTouchActionManipulation,
            cc::kTouchActionAuto & ~cc::kTouchActionDoubleTapZoom);
  EXPECT_EQ(cc::kTouchActionPanX,
            cc::kTouchActionPanLeft | cc::kTouchActionPanRight);
  EXPECT_EQ(cc::kTouchActionAuto,
            cc::kTouchActionManipulation | cc::kTouchActionDoubleTapZoom);
}

TEST(TouchActionFilterTest, MultiTouch) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 10;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  // For multiple points, the intersection is what matters.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);

  // Intersection of PAN_X and PAN_Y is NONE.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionPanX);
  filter.OnSetTouchAction(cc::kTouchActionPanY);
  filter.OnSetTouchAction(cc::kTouchActionPan);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);
}

class TouchActionFilterPinchTest : public testing::Test {
 public:
  void RunTest(bool force_enable_zoom) {
    TouchActionFilter filter;
    filter.SetForceEnableZoom(force_enable_zoom);

    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice,
                                                          2);
    WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGesturePinchBegin, kSourceDevice);
    WebGestureEvent pinch_update =
        SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                          kSourceDevice);
    WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGesturePinchEnd, kSourceDevice);
    WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGestureScrollEnd, kSourceDevice);

    // Pinch is allowed with touch-action: auto.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // Pinch is not allowed with touch-action: none.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);

    // Pinch is not allowed with touch-action: pan-x pan-y except for force
    // enable zoom.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    EXPECT_NE(filter.FilterGestureEvent(&scroll_begin),
              force_enable_zoom
                  ? FilterGestureEventResult::kFilterGestureEventFiltered
                  : FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_begin),
              force_enable_zoom
                  ? FilterGestureEventResult::kFilterGestureEventFiltered
                  : FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_update),
              force_enable_zoom
                  ? FilterGestureEventResult::kFilterGestureEventFiltered
                  : FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_end),
              force_enable_zoom
                  ? FilterGestureEventResult::kFilterGestureEventFiltered
                  : FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_NE(filter.FilterGestureEvent(&scroll_end),
              force_enable_zoom
                  ? FilterGestureEventResult::kFilterGestureEventFiltered
                  : FilterGestureEventResult::kFilterGestureEventAllowed);

    // Pinch is allowed with touch-action: manipulation.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionManipulation);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // Pinch state is automatically reset at the end of a scroll.
    filter.ResetTouchAction();
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // Pinching is only computed at GestureScrollBegin time.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // Once a pinch has started, any change in state won't affect the pinch
    // gestures since it is computed in GestureScrollBegin.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // Scrolling is allowed when two fingers are down.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPinchZoom);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    // A pinch event sequence with only one pointer is equivalent to a scroll
    // gesture, so disallowed as a pinch gesture.
    scroll_begin.data.scroll_begin.pointer_count = 1;
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPinchZoom);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
              FilterGestureEventResult::kFilterGestureEventFiltered);
  }
};

TEST_F(TouchActionFilterPinchTest, Pinch) {
  RunTest(false);
}

// Enables force enable zoom will override touch-action except for
// touch-action: none.
TEST_F(TouchActionFilterPinchTest, ForceEnableZoom) {
  RunTest(true);
}

TEST(TouchActionFilterTest, DoubleTapWithTouchActionAuto) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap_cancel = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapCancel, kSourceDevice);
  WebGestureEvent double_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureDoubleTap, kSourceDevice);

  // Double tap is allowed with touch action auto.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&unconfirmed_tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(unconfirmed_tap.GetType(), WebInputEvent::kGestureTapUnconfirmed);
  // The tap cancel will come as part of the next touch sequence.
  filter.ResetTouchAction();
  // Changing the touch action for the second tap doesn't effect the behaviour
  // of the event.
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_cancel),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&double_tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
}

TEST(TouchActionFilterTest, DoubleTap) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap_cancel = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapCancel, kSourceDevice);
  WebGestureEvent double_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureDoubleTap, kSourceDevice);

  // Double tap is disabled with any touch action other than auto.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionManipulation);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&unconfirmed_tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(WebInputEvent::kGestureTap, unconfirmed_tap.GetType());
  // Changing the touch action for the second tap doesn't effect the behaviour
  // of the event. The tap cancel will come as part of the next touch sequence.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_cancel),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&double_tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(WebInputEvent::kGestureTap, double_tap.GetType());
}

TEST(TouchActionFilterTest, SingleTapWithTouchActionAuto) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap1 = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // Single tap is allowed with touch action auto.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&unconfirmed_tap1),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(WebInputEvent::kGestureTapUnconfirmed, unconfirmed_tap1.GetType());
  EXPECT_EQ(filter.FilterGestureEvent(&tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);
}

TEST(TouchActionFilterTest, SingleTap) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap1 = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // With touch action other than auto, tap unconfirmed is turned into tap.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&tap_down),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&unconfirmed_tap1),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(WebInputEvent::kGestureTap, unconfirmed_tap1.GetType());
  EXPECT_EQ(filter.FilterGestureEvent(&tap),
            FilterGestureEventResult::kFilterGestureEventFiltered);
}

TEST(TouchActionFilterTest, TouchActionResetsOnResetTouchAction) {
  TouchActionFilter filter;

  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);
  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);

  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&tap),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
}

TEST(TouchActionFilterTest, TouchActionResetMidSequence) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                        kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);

  // Even though the allowed action is auto after the reset, the remaining
  // scroll and pinch events should be suppressed.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventFiltered);

  // A new scroll and pinch sequence should be allowed.
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);

  // Resetting from auto to auto mid-stream should have no effect.
  filter.ResetTouchAction();
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_update),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&pinch_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_end),
            FilterGestureEventResult::kFilterGestureEventAllowed);
}

TEST(TouchActionFilterTest, ZeroVelocityFlingsConvertedToScrollEnd) {
  TouchActionFilter filter;
  const float kFlingX = 7;
  const float kFlingY = -4;

  {
    // Scrolls hinted mostly in the Y axis will suppress flings with a
    // component solely on the X axis, converting them to a GestureScrollEnd.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPanY);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, 0, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(WebInputEvent::kGestureScrollEnd, fling_start.GetType());
  }

  filter.ResetTouchAction();

  {
    // Scrolls hinted mostly in the X axis will suppress flings with a
    // component solely on the Y axis, converting them to a GestureScrollEnd.
    filter.ResetTouchAction();
    filter.OnSetTouchAction(cc::kTouchActionPanX);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-7, 6, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
              FilterGestureEventResult::kFilterGestureEventAllowed);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        0, kFlingY, kSourceDevice);
    EXPECT_EQ(filter.FilterGestureEvent(&fling_start),
              FilterGestureEventResult::kFilterGestureEventAllowed);
    EXPECT_EQ(WebInputEvent::kGestureScrollEnd, fling_start.GetType());
  }
}

TEST(TouchActionFilterTest, TouchpadScroll) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(
          2, 3, blink::kWebGestureDeviceTouchpad);

  // cc::kTouchActionNone filters out only touchscreen scroll events.
  filter.ResetTouchAction();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_EQ(filter.FilterGestureEvent(&scroll_begin),
            FilterGestureEventResult::kFilterGestureEventAllowed);
}

}  // namespace content
