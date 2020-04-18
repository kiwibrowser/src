// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/ozone/platform/wayland/fake_server.h"
#include "ui/ozone/platform/wayland/wayland_surface_factory.h"
#include "ui/ozone/platform/wayland/wayland_test.h"
#include "ui/ozone/platform/wayland/wayland_window.h"
#include "ui/ozone/public/surface_ozone_canvas.h"
#include "ui/ozone/test/mock_platform_window_delegate.h"

using ::testing::Expectation;
using ::testing::SaveArg;
using ::testing::_;

namespace ui {

class WaylandSurfaceFactoryTest : public WaylandTest {
 public:
  WaylandSurfaceFactoryTest() : surface_factory(connection_.get()) {}

  ~WaylandSurfaceFactoryTest() override {}

  void SetUp() override {
    WaylandTest::SetUp();
    canvas = surface_factory.CreateCanvasForWidget(widget_);
    ASSERT_TRUE(canvas);
  }

 protected:
  WaylandSurfaceFactory surface_factory;
  std::unique_ptr<SurfaceOzoneCanvas> canvas;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaylandSurfaceFactoryTest);
};

TEST_P(WaylandSurfaceFactoryTest, Canvas) {
  canvas->GetSurface();
  canvas->PresentCanvas(gfx::Rect(5, 10, 20, 15));

  Expectation damage = EXPECT_CALL(*surface_, Damage(5, 10, 20, 15));
  wl_resource* buffer_resource = nullptr;
  Expectation attach = EXPECT_CALL(*surface_, Attach(_, 0, 0))
                           .WillOnce(SaveArg<0>(&buffer_resource));
  EXPECT_CALL(*surface_, Commit()).After(damage, attach);

  Sync();

  ASSERT_TRUE(buffer_resource);
  wl_shm_buffer* buffer = wl_shm_buffer_get(buffer_resource);
  ASSERT_TRUE(buffer);
  EXPECT_EQ(wl_shm_buffer_get_width(buffer), 800);
  EXPECT_EQ(wl_shm_buffer_get_height(buffer), 600);

  // TODO(forney): We could check that the contents match something drawn to the
  // SkSurface above.
}

TEST_P(WaylandSurfaceFactoryTest, CanvasResize) {
  canvas->GetSurface();
  canvas->ResizeCanvas(gfx::Size(100, 50));
  canvas->GetSurface();
  canvas->PresentCanvas(gfx::Rect(0, 0, 100, 50));

  Expectation damage = EXPECT_CALL(*surface_, Damage(0, 0, 100, 50));
  wl_resource* buffer_resource = nullptr;
  Expectation attach = EXPECT_CALL(*surface_, Attach(_, 0, 0))
                           .WillOnce(SaveArg<0>(&buffer_resource));
  EXPECT_CALL(*surface_, Commit()).After(damage, attach);

  Sync();

  ASSERT_TRUE(buffer_resource);
  wl_shm_buffer* buffer = wl_shm_buffer_get(buffer_resource);
  ASSERT_TRUE(buffer);
  EXPECT_EQ(wl_shm_buffer_get_width(buffer), 100);
  EXPECT_EQ(wl_shm_buffer_get_height(buffer), 50);
}

INSTANTIATE_TEST_CASE_P(XdgVersionV5Test,
                        WaylandSurfaceFactoryTest,
                        ::testing::Values(kXdgShellV5));
INSTANTIATE_TEST_CASE_P(XdgVersionV6Test,
                        WaylandSurfaceFactoryTest,
                        ::testing::Values(kXdgShellV6));

}  // namespace ui
