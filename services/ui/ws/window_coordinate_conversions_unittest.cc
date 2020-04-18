// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_coordinate_conversions.h"

#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace ws {

class WindowCoordinateConversions : public testing::Test {
 public:
  WindowCoordinateConversions() {}
  ~WindowCoordinateConversions() override {}

  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }

 private:
  test::WindowServerTestHelper ws_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(WindowCoordinateConversions);
};

TEST_F(WindowCoordinateConversions, Transform) {
  TestServerWindowDelegate window_delegate(viz_host_proxy());
  ServerWindow root(&window_delegate, viz::FrameSinkId(1, 2));
  root.set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  root.SetVisible(true);
  root.SetBounds(gfx::Rect(0, 0, 100, 100), base::nullopt);
  ServerWindow child(&window_delegate, viz::FrameSinkId(1, 3));
  root.Add(&child);
  child.SetVisible(true);
  child.SetBounds(gfx::Rect(0, 0, 20, 20), base::nullopt);
  // Make the root |child|, and set a transform on |child|, which mirrors
  // how WindowManagerState and EventDispatcher work together.
  window_delegate.set_root_window(&child);
  gfx::Transform transform;
  transform.Scale(SkIntToMScalar(2), SkIntToMScalar(2));
  child.SetTransform(transform);

  ServerWindow child_child(&window_delegate, viz::FrameSinkId(1, 4));
  child.Add(&child_child);
  child_child.SetVisible(true);
  child_child.SetBounds(gfx::Rect(4, 6, 12, 24), base::nullopt);

  const gfx::Point converted_point = ConvertPointFromRootForEventDispatch(
      &child, &child_child, gfx::Point(14, 20));
  EXPECT_EQ(gfx::Point(3, 4), converted_point);
}

}  // namespace ws
}  // namespace ui
