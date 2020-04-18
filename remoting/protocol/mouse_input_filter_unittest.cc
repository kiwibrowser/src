// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mouse_input_filter.h"

#include "base/macros.h"
#include "remoting/proto/event.pb.h"
#include "remoting/protocol/protocol_mock_objects.h"
#include "remoting/protocol/test_event_matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

using ::testing::_;
using ::testing::InSequence;

namespace remoting {
namespace protocol {

using test::EqualsMouseMoveEvent;

static MouseEvent MouseMoveEvent(int x, int y) {
  MouseEvent event;
  event.set_x(x);
  event.set_y(y);
  return event;
}

static void InjectTestSequence(InputStub* input_stub) {
  struct Point {
    int x;
    int y;
  };
  static const Point input_sequence[] = {
    {-5, 10}, {0, 10}, {-1, 10}, {15, 40}, {15, 45}, {15, 39}, {15, 25}
  };
  for (unsigned int i = 0; i < arraysize(input_sequence); ++i) {
    const Point& point = input_sequence[i];
    input_stub->InjectMouseEvent(MouseMoveEvent(point.x, point.y));
  }
  for (unsigned int i = 0; i < arraysize(input_sequence); ++i) {
    const Point& point = input_sequence[i];
    input_stub->InjectMouseEvent(MouseMoveEvent(point.y, point.x));
  }
}

// Verify that no events get through if we don't set either dimensions.
TEST(MouseInputFilterTest, BothDimensionsZero) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);

  EXPECT_CALL(mock_stub, InjectMouseEvent(_))
        .Times(0);

  InjectTestSequence(&mouse_filter);
}

// Verify that no events get through if there's no input size.
TEST(MouseInputFilterTest, InputDimensionsZero) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);
  mouse_filter.set_output_size(webrtc::DesktopSize(50, 50));

  EXPECT_CALL(mock_stub, InjectMouseEvent(_))
      .Times(0);

  InjectTestSequence(&mouse_filter);
}

// Verify that no events get through if there's no output size.
TEST(MouseInputFilterTest, OutputDimensionsZero) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);
  mouse_filter.set_input_size(webrtc::DesktopSize(50, 50));

  EXPECT_CALL(mock_stub, InjectMouseEvent(_))
      .Times(0);

  InjectTestSequence(&mouse_filter);
}

// Verify that all events get through, clamped to the output.
TEST(MouseInputFilterTest, NoScalingOrClipping) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);
  mouse_filter.set_output_size(webrtc::DesktopSize(40,40));
  mouse_filter.set_input_size(webrtc::DesktopSize(40,40));

  {
    InSequence s;

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(0, 10))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(15, 39))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(15, 25))).
        Times(1);

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(10, 0))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(39, 15))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(25, 15))).
        Times(1);
  }

  InjectTestSequence(&mouse_filter);
}

// Verify that we can up-scale with clamping.
TEST(MouseInputFilterTest, UpScalingAndClamping) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);
  mouse_filter.set_output_size(webrtc::DesktopSize(80, 80));
  mouse_filter.set_input_size(webrtc::DesktopSize(40, 40));

  {
    InSequence s;

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(0, 20))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(30, 79))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(30, 51))).
        Times(1);

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(20, 0))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(79, 30))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(51, 30))).
        Times(1);
  }

  InjectTestSequence(&mouse_filter);
}

// Verify that we can down-scale with clamping.
TEST(MouseInputFilterTest, DownScalingAndClamping) {
  MockInputStub mock_stub;
  MouseInputFilter mouse_filter(&mock_stub);
  mouse_filter.set_output_size(webrtc::DesktopSize(30, 30));
  mouse_filter.set_input_size(webrtc::DesktopSize(40, 40));

  {
    InSequence s;

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(0, 7))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(11, 29))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(11, 19))).
        Times(1);

    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(7, 0))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(29, 11))).
        Times(3);
    EXPECT_CALL(mock_stub, InjectMouseEvent(EqualsMouseMoveEvent(19, 11))).
        Times(1);

  }

  InjectTestSequence(&mouse_filter);
}

}  // namespace protocol
}  // namespace remoting
