// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/surface.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "components/exo/buffer.h"
#include "components/exo/shell_surface.h"
#include "components/exo/sub_surface.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/test/begin_frame_args_test.h"
#include "components/viz/test/fake_external_begin_frame_source.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/aura/env.h"
#include "ui/compositor/layer_tree_owner.h"
#include "ui/display/display.h"
#include "ui/display/display_switches.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/wm/core/window_util.h"

namespace exo {
namespace {

class SurfaceTest : public test::ExoTestBase,
                    public ::testing::WithParamInterface<float> {
 public:
  SurfaceTest() = default;
  ~SurfaceTest() override = default;
  void SetUp() override {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    // Set the device scale factor.
    command_line->AppendSwitchASCII(
        switches::kForceDeviceScaleFactor,
        base::StringPrintf("%f", device_scale_factor()));
    test::ExoTestBase::SetUp();
  }

  void TearDown() override {
    test::ExoTestBase::TearDown();
    display::Display::ResetForceDeviceScaleFactorForTesting();
  }

  float device_scale_factor() const { return GetParam(); }

  gfx::Rect ToPixel(const gfx::Rect rect) {
    return gfx::ConvertRectToPixel(device_scale_factor(), rect);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceTest);
};

void ReleaseBuffer(int* release_buffer_call_count) {
  (*release_buffer_call_count)++;
}

// Instantiate the Boolean which is used to toggle mouse and touch events in
// the parameterized tests.
INSTANTIATE_TEST_CASE_P(, SurfaceTest, testing::Values(1.0f, 1.25f, 2.0f));

TEST_P(SurfaceTest, Attach) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));

  // Set the release callback that will be run when buffer is no longer in use.
  int release_buffer_call_count = 0;
  buffer->set_release_callback(
      base::Bind(&ReleaseBuffer, base::Unretained(&release_buffer_call_count)));

  std::unique_ptr<Surface> surface(new Surface);

  // Attach the buffer to surface1.
  surface->Attach(buffer.get());
  surface->Commit();

  // Commit without calling Attach() should have no effect.
  surface->Commit();
  EXPECT_EQ(0, release_buffer_call_count);

  // Attach a null buffer to surface, this should release the previously
  // attached buffer.
  surface->Attach(nullptr);
  surface->Commit();
  // LayerTreeFrameSinkHolder::ReclaimResources() gets called via
  // CompositorFrameSinkClient interface. We need to wait here for the mojo
  // call to finish so that the release callback finishes running before
  // the assertion below.
  RunAllPendingInMessageLoop();
  ASSERT_EQ(1, release_buffer_call_count);
}

const viz::CompositorFrame& GetFrameFromSurface(ShellSurface* shell_surface) {
  viz::SurfaceId surface_id = shell_surface->host_window()->GetSurfaceId();
  viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                             ->context_factory_private()
                                             ->GetFrameSinkManager()
                                             ->surface_manager();
  const viz::CompositorFrame& frame =
      surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
  return frame;
}

TEST_P(SurfaceTest, Damage) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  // Attach the buffer to the surface. This will update the pending bounds of
  // the surface to the buffer size.
  surface->Attach(buffer.get());

  // Mark areas inside the bounds of the surface as damaged. This should result
  // in pending damage.
  surface->Damage(gfx::Rect(0, 0, 10, 10));
  surface->Damage(gfx::Rect(10, 10, 10, 10));
  EXPECT_TRUE(surface->HasPendingDamageForTesting(gfx::Rect(0, 0, 10, 10)));
  EXPECT_TRUE(surface->HasPendingDamageForTesting(gfx::Rect(10, 10, 10, 10)));
  EXPECT_FALSE(surface->HasPendingDamageForTesting(gfx::Rect(5, 5, 10, 10)));

  // Check that damage larger than contents is handled correctly at commit.
  surface->Damage(gfx::Rect(gfx::ScaleToCeiledSize(buffer_size, 2.0f)));
  surface->Commit();
  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    EXPECT_EQ(ToPixel(gfx::Rect(buffer_size)),
              frame.render_pass_list.back()->damage_rect);
  }

  gfx::RectF buffer_damage(32, 64, 16, 32);
  gfx::Rect surface_damage = gfx::ToNearestRect(buffer_damage);

  // Check that damage is correct for a non-square rectangle not at the origin.
  surface->Damage(surface_damage);
  surface->Commit();
  RunAllPendingInMessageLoop();

  // Adjust damage for DSF filtering and verify it below.
  if (device_scale_factor() > 1.f)
    buffer_damage.Inset(-1.f, -1.f);

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    EXPECT_TRUE(
        gfx::RectF(frame.render_pass_list.back()->damage_rect)
            .Contains(gfx::ScaleRect(buffer_damage, device_scale_factor())));
  }
}

void SetFrameTime(base::TimeTicks* result, base::TimeTicks frame_time) {
  *result = frame_time;
}

TEST_P(SurfaceTest, RequestFrameCallback) {
  std::unique_ptr<Surface> surface(new Surface);

  base::TimeTicks frame_time;
  surface->RequestFrameCallback(
      base::Bind(&SetFrameTime, base::Unretained(&frame_time)));
  surface->Commit();

  // Callback should not run synchronously.
  EXPECT_TRUE(frame_time.is_null());
}

TEST_P(SurfaceTest, SetOpaqueRegion) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  // Attaching a buffer with alpha channel.
  surface->Attach(buffer.get());

  // Setting an opaque region that contains the buffer size doesn't require
  // draw with blending.
  surface->SetOpaqueRegion(gfx::Rect(256, 256));
  surface->Commit();
  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
    EXPECT_FALSE(frame.render_pass_list.back()
                     ->quad_list.back()
                     ->ShouldDrawWithBlending());
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 1, 1)),
              frame.render_pass_list.back()->damage_rect);
  }

  // Setting an empty opaque region requires draw with blending.
  surface->SetOpaqueRegion(gfx::Rect());
  surface->Commit();
  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
    EXPECT_TRUE(frame.render_pass_list.back()
                    ->quad_list.back()
                    ->ShouldDrawWithBlending());
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 1, 1)),
              frame.render_pass_list.back()->damage_rect);
  }

  std::unique_ptr<Buffer> buffer_without_alpha(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(
          buffer_size, gfx::BufferFormat::RGBX_8888)));

  // Attaching a buffer without an alpha channel doesn't require draw with
  // blending.
  surface->Attach(buffer_without_alpha.get());
  surface->Commit();
  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
    EXPECT_FALSE(frame.render_pass_list.back()
                     ->quad_list.back()
                     ->ShouldDrawWithBlending());
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 0, 0)),
              frame.render_pass_list.back()->damage_rect);
  }
}

TEST_P(SurfaceTest, SetInputRegion) {
  // Create a shell surface which size is 512x512.
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());
  gfx::Size buffer_size(512, 512);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  surface->Attach(buffer.get());
  surface->Commit();

  {
    // Default input region should match surface bounds.
    auto rects = surface->GetHitTestShapeRects();
    ASSERT_TRUE(rects);
    ASSERT_EQ(1u, rects->size());
    ASSERT_EQ(gfx::Rect(512, 512), (*rects)[0]);
  }

  {
    // Setting a non-empty input region should succeed.
    surface->SetInputRegion(gfx::Rect(256, 256));
    surface->Commit();

    auto rects = surface->GetHitTestShapeRects();
    ASSERT_TRUE(rects);
    ASSERT_EQ(1u, rects->size());
    ASSERT_EQ(gfx::Rect(256, 256), (*rects)[0]);
  }

  {
    // Setting an empty input region should succeed.
    surface->SetInputRegion(gfx::Rect());
    surface->Commit();

    EXPECT_FALSE(surface->GetHitTestShapeRects());
  }

  {
    cc::Region region = gfx::Rect(512, 512);
    region.Subtract(gfx::Rect(0, 64, 64, 64));
    region.Subtract(gfx::Rect(88, 88, 12, 55));
    region.Subtract(gfx::Rect(100, 0, 33, 66));

    // Setting a non-rectangle input region should succeed.
    surface->SetInputRegion(region);
    surface->Commit();

    auto rects = surface->GetHitTestShapeRects();
    ASSERT_TRUE(rects);
    ASSERT_EQ(10u, rects->size());
    cc::Region result;
    for (const auto& r : *rects)
      result.Union(r);
    ASSERT_EQ(result, region);
  }

  {
    // Input region should be clipped to surface bounds.
    surface->SetInputRegion(gfx::Rect(-50, -50, 1000, 100));
    surface->Commit();

    auto rects = surface->GetHitTestShapeRects();
    ASSERT_TRUE(rects);
    ASSERT_EQ(1u, rects->size());
    ASSERT_EQ(gfx::Rect(512, 50), (*rects)[0]);
  }

  {
    // Hit test region should accumulate input regions of sub-surfaces.
    gfx::Rect input_rect(50, 50, 100, 100);
    surface->SetInputRegion(input_rect);

    gfx::Rect child_input_rect(-50, -50, 1000, 100);
    auto child_buffer = std::make_unique<Buffer>(
        exo_test_helper()->CreateGpuMemoryBuffer(child_input_rect.size()));
    auto child_surface = std::make_unique<Surface>();
    auto sub_surface =
        std::make_unique<SubSurface>(child_surface.get(), surface.get());
    sub_surface->SetPosition(child_input_rect.origin());
    child_surface->Attach(child_buffer.get());
    child_surface->Commit();
    surface->Commit();

    auto rects = surface->GetHitTestShapeRects();
    ASSERT_TRUE(rects);
    ASSERT_EQ(2u, rects->size());
    cc::Region result = cc::UnionRegions((*rects)[0], (*rects)[1]);
    ASSERT_EQ(cc::UnionRegions(input_rect, child_input_rect), result);
  }
}

TEST_P(SurfaceTest, SetBufferScale) {
  gfx::Size buffer_size(512, 512);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  // This will update the bounds of the surface and take the buffer scale into
  // account.
  const float kBufferScale = 2.0f;
  surface->Attach(buffer.get());
  surface->SetBufferScale(kBufferScale);
  surface->Commit();
  EXPECT_EQ(
      gfx::ScaleToFlooredSize(buffer_size, 1.0f / kBufferScale).ToString(),
      surface->window()->bounds().size().ToString());
  EXPECT_EQ(
      gfx::ScaleToFlooredSize(buffer_size, 1.0f / kBufferScale).ToString(),
      surface->content_size().ToString());

  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  ASSERT_EQ(1u, frame.render_pass_list.size());
  EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 256, 256)),
            frame.render_pass_list.back()->damage_rect);
}

TEST_P(SurfaceTest, SetBufferTransform) {
  gfx::Size buffer_size(256, 512);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  // This will update the bounds of the surface and take the buffer transform
  // into account.
  surface->Attach(buffer.get());
  surface->SetBufferTransform(Transform::ROTATE_90);
  surface->Commit();
  EXPECT_EQ(gfx::Size(buffer_size.height(), buffer_size.width()),
            surface->window()->bounds().size());
  EXPECT_EQ(gfx::Size(buffer_size.height(), buffer_size.width()),
            surface->content_size());

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    EXPECT_EQ(
        ToPixel(gfx::Rect(0, 0, buffer_size.height(), buffer_size.width())),
        frame.render_pass_list.back()->damage_rect);
    const auto& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    EXPECT_EQ(
        ToPixel(gfx::Rect(0, 0, 512, 256)),
        cc::MathUtil::MapEnclosingClippedRect(
            quad_list.front()->shared_quad_state->quad_to_target_transform,
            quad_list.front()->rect));
  }

  gfx::Size child_buffer_size(64, 128);
  auto child_buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(child_buffer_size));
  auto child_surface = std::make_unique<Surface>();
  auto sub_surface =
      std::make_unique<SubSurface>(child_surface.get(), surface.get());

  // Set position to 20, 10.
  gfx::Point child_position(20, 10);
  sub_surface->SetPosition(child_position);

  child_surface->Attach(child_buffer.get());
  child_surface->SetBufferTransform(Transform::ROTATE_180);
  const int kChildBufferScale = 2;
  child_surface->SetBufferScale(kChildBufferScale);
  child_surface->Commit();
  surface->Commit();
  EXPECT_EQ(
      gfx::ScaleToRoundedSize(child_buffer_size, 1.0f / kChildBufferScale),
      child_surface->window()->bounds().size());
  EXPECT_EQ(
      gfx::ScaleToRoundedSize(child_buffer_size, 1.0f / kChildBufferScale),
      child_surface->content_size());

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const auto& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(2u, quad_list.size());
    EXPECT_EQ(
        ToPixel(gfx::Rect(child_position,
                          gfx::ScaleToRoundedSize(child_buffer_size,
                                                  1.0f / kChildBufferScale))),
        cc::MathUtil::MapEnclosingClippedRect(
            quad_list.front()->shared_quad_state->quad_to_target_transform,
            quad_list.front()->rect));
  }
}

TEST_P(SurfaceTest, MirrorLayers) {
  gfx::Size buffer_size(512, 512);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_EQ(buffer_size, surface->window()->bounds().size());
  EXPECT_EQ(buffer_size, surface->window()->layer()->bounds().size());
  std::unique_ptr<ui::LayerTreeOwner> old_layer_owner =
      ::wm::MirrorLayers(shell_surface->host_window(), false /* sync_bounds */);
  EXPECT_EQ(buffer_size, surface->window()->bounds().size());
  EXPECT_EQ(buffer_size, surface->window()->layer()->bounds().size());
  EXPECT_EQ(buffer_size, old_layer_owner->root()->bounds().size());
  EXPECT_TRUE(shell_surface->host_window()->layer()->has_external_content());
  EXPECT_TRUE(old_layer_owner->root()->has_external_content());
}

TEST_P(SurfaceTest, SetViewport) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  // This will update the bounds of the surface and take the viewport into
  // account.
  surface->Attach(buffer.get());
  gfx::Size viewport(256, 256);
  surface->SetViewport(viewport);
  surface->Commit();
  EXPECT_EQ(viewport.ToString(), surface->content_size().ToString());

  // This will update the bounds of the surface and take the viewport2 into
  // account.
  gfx::Size viewport2(512, 512);
  surface->SetViewport(viewport2);
  surface->Commit();
  EXPECT_EQ(viewport2.ToString(),
            surface->window()->bounds().size().ToString());
  EXPECT_EQ(viewport2.ToString(), surface->content_size().ToString());

  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  ASSERT_EQ(1u, frame.render_pass_list.size());
  EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 512, 512)),
            frame.render_pass_list.back()->damage_rect);
}

TEST_P(SurfaceTest, SetCrop) {
  gfx::Size buffer_size(16, 16);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  surface->Attach(buffer.get());
  gfx::Size crop_size(12, 12);
  surface->SetCrop(gfx::RectF(gfx::PointF(2.0, 2.0), gfx::SizeF(crop_size)));
  surface->Commit();
  EXPECT_EQ(crop_size.ToString(),
            surface->window()->bounds().size().ToString());
  EXPECT_EQ(crop_size.ToString(), surface->content_size().ToString());

  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  ASSERT_EQ(1u, frame.render_pass_list.size());
  EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 12, 12)),
            frame.render_pass_list.back()->damage_rect);
}

TEST_P(SurfaceTest, SetCropAndBufferTransform) {
  gfx::Size buffer_size(128, 64);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  const gfx::RectF crop_0(
      gfx::SkRectToRectF(SkRect::MakeLTRB(0.03125f, 0.1875f, 0.4375f, 0.25f)));
  const gfx::RectF crop_90(
      gfx::SkRectToRectF(SkRect::MakeLTRB(0.875f, 0.0625f, 0.90625f, 0.875f)));
  const gfx::RectF crop_180(
      gfx::SkRectToRectF(SkRect::MakeLTRB(0.5625f, 0.75f, 0.96875f, 0.8125f)));
  const gfx::RectF crop_270(
      gfx::SkRectToRectF(SkRect::MakeLTRB(0.09375f, 0.125f, 0.125f, 0.9375f)));
  const gfx::Rect target_with_no_viewport(ToPixel(gfx::Rect(gfx::Size(52, 4))));
  const gfx::Rect target_with_viewport(ToPixel(gfx::Rect(gfx::Size(128, 64))));

  surface->Attach(buffer.get());
  gfx::Size crop_size(52, 4);
  surface->SetCrop(gfx::RectF(gfx::PointF(4, 12), gfx::SizeF(crop_size)));
  surface->SetBufferTransform(Transform::NORMAL);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_0.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_0.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        ToPixel(gfx::Rect(0, 0, 52, 4)),
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_90);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_90.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_90.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        ToPixel(gfx::Rect(0, 0, 52, 4)),
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_180);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_180.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_180.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        ToPixel(gfx::Rect(0, 0, 52, 4)),
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_270);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_270.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_270.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        target_with_no_viewport,
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetViewport(gfx::Size(128, 64));
  surface->SetBufferTransform(Transform::NORMAL);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_0.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_0.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        target_with_viewport,
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_90);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_90.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_90.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        target_with_viewport,
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_180);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_180.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_180.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        target_with_viewport,
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }

  surface->SetBufferTransform(Transform::ROTATE_270);
  surface->Commit();

  RunAllPendingInMessageLoop();

  {
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    const viz::QuadList& quad_list = frame.render_pass_list[0]->quad_list;
    ASSERT_EQ(1u, quad_list.size());
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(quad_list.front());
    EXPECT_EQ(crop_270.origin(), quad->uv_top_left);
    EXPECT_EQ(crop_270.bottom_right(), quad->uv_bottom_right);
    EXPECT_EQ(
        target_with_viewport,
        cc::MathUtil::MapEnclosingClippedRect(
            quad->shared_quad_state->quad_to_target_transform, quad->rect));
  }
}

TEST_P(SurfaceTest, SetBlendMode) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  surface->Attach(buffer.get());
  surface->SetBlendMode(SkBlendMode::kSrc);
  surface->Commit();
  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  ASSERT_EQ(1u, frame.render_pass_list.size());
  ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
  EXPECT_FALSE(frame.render_pass_list.back()
                   ->quad_list.back()
                   ->ShouldDrawWithBlending());
}

TEST_P(SurfaceTest, OverlayCandidate) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size), GL_TEXTURE_2D, 0,
      true, true);
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  surface->Attach(buffer.get());
  surface->Commit();
  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  ASSERT_EQ(1u, frame.render_pass_list.size());
  ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
  viz::DrawQuad* draw_quad = frame.render_pass_list.back()->quad_list.back();
  ASSERT_EQ(viz::DrawQuad::TEXTURE_CONTENT, draw_quad->material);

  const viz::TextureDrawQuad* texture_quad =
      viz::TextureDrawQuad::MaterialCast(draw_quad);
  EXPECT_FALSE(texture_quad->resource_size_in_pixels().IsEmpty());
}

TEST_P(SurfaceTest, SetAlpha) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size), GL_TEXTURE_2D, 0,
      true, true);
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  {
    surface->Attach(buffer.get());
    surface->SetAlpha(0.5f);
    surface->Commit();
    RunAllPendingInMessageLoop();

    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
    ASSERT_EQ(1u, frame.resource_list.size());
    ASSERT_EQ(1u, frame.resource_list.back().id);
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 1, 1)),
              frame.render_pass_list.back()->damage_rect);
  }

  {
    surface->SetAlpha(0.f);
    surface->Commit();
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    // No quad if alpha is 0.
    ASSERT_EQ(0u, frame.render_pass_list.back()->quad_list.size());
    ASSERT_EQ(0u, frame.resource_list.size());
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 1, 1)),
              frame.render_pass_list.back()->damage_rect);
  }

  {
    surface->SetAlpha(1.f);
    surface->Commit();
    const viz::CompositorFrame& frame =
        GetFrameFromSurface(shell_surface.get());
    ASSERT_EQ(1u, frame.render_pass_list.size());
    ASSERT_EQ(1u, frame.render_pass_list.back()->quad_list.size());
    ASSERT_EQ(1u, frame.resource_list.size());
    // The resource should be updated again, the id should be changed.
    ASSERT_EQ(2u, frame.resource_list.back().id);
    EXPECT_EQ(ToPixel(gfx::Rect(0, 0, 1, 1)),
              frame.render_pass_list.back()->damage_rect);
  }
}

TEST_P(SurfaceTest, Commit) {
  std::unique_ptr<Surface> surface(new Surface);

  // Calling commit without a buffer should succeed.
  surface->Commit();
}

TEST_P(SurfaceTest, SendsBeginFrameAcks) {
  viz::FakeExternalBeginFrameSource source(0.f, false);
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size), GL_TEXTURE_2D, 0,
      true, true);
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());
  shell_surface->SetBeginFrameSource(&source);
  surface->Attach(buffer.get());

  // Request a frame callback so that Surface now needs BeginFrames.
  base::TimeTicks frame_time;
  surface->RequestFrameCallback(
      base::Bind(&SetFrameTime, base::Unretained(&frame_time)));
  surface->Commit();  // Move callback from pending callbacks to current ones.
  RunAllPendingInMessageLoop();

  // Surface should add itself as observer during
  // DidReceiveCompositorFrameAck().
  shell_surface->DidReceiveCompositorFrameAck();
  EXPECT_EQ(1u, source.num_observers());

  viz::BeginFrameArgs args(source.CreateBeginFrameArgs(BEGINFRAME_FROM_HERE));
  args.frame_time = base::TimeTicks::FromInternalValue(100);
  source.TestOnBeginFrame(args);  // Runs the frame callback.
  EXPECT_EQ(args.frame_time, frame_time);

  surface->Commit();  // Acknowledges the BeginFrame via CompositorFrame.
  RunAllPendingInMessageLoop();

  const viz::CompositorFrame& frame = GetFrameFromSurface(shell_surface.get());
  viz::BeginFrameAck expected_ack(args.source_id, args.sequence_number, true);
  EXPECT_EQ(expected_ack, frame.metadata.begin_frame_ack);

  // TODO(eseckler): Add test for DidNotProduceFrame plumbing.
}

TEST_P(SurfaceTest, RemoveSubSurface) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());
  surface->Attach(buffer.get());

  // Create a subsurface:
  gfx::Size child_buffer_size(64, 128);
  auto child_buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(child_buffer_size));
  auto child_surface = std::make_unique<Surface>();
  auto sub_surface =
      std::make_unique<SubSurface>(child_surface.get(), surface.get());
  sub_surface->SetPosition(gfx::Point(20, 10));
  child_surface->Attach(child_buffer.get());
  child_surface->Commit();
  surface->Commit();
  RunAllPendingInMessageLoop();

  // Remove the subsurface by destroying it. This should not damage |surface|.
  // TODO(penghuang): Make the damage more precise for sub surface changes.
  // https://crbug.com/779704
  sub_surface.reset();
  EXPECT_FALSE(surface->HasPendingDamageForTesting(gfx::Rect(20, 10, 64, 128)));
}

TEST_P(SurfaceTest, DestroyAttachedBuffer) {
  gfx::Size buffer_size(1, 1);
  auto buffer = std::make_unique<Buffer>(
      exo_test_helper()->CreateGpuMemoryBuffer(buffer_size));
  auto surface = std::make_unique<Surface>();
  auto shell_surface = std::make_unique<ShellSurface>(surface.get());

  surface->Attach(buffer.get());
  surface->Commit();
  RunAllPendingInMessageLoop();

  // Make sure surface size is still valid after buffer is destroyed.
  buffer.reset();
  surface->Commit();
  EXPECT_FALSE(surface->content_size().IsEmpty());
}

}  // namespace
}  // namespace exo
