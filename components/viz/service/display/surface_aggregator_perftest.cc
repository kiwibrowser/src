// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/lap_timer.h"
#include "cc/test/fake_output_surface_client.h"
#include "cc/test/fake_resource_provider.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/service/display/display_resource_provider.h"
#include "components/viz/service/display/surface_aggregator.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface_manager.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_shared_bitmap_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace viz {
namespace {

constexpr bool kIsRoot = true;
constexpr bool kIsChildRoot = false;
constexpr bool kNeedsSyncPoints = true;

const base::UnguessableToken kArbitraryToken = base::UnguessableToken::Create();

class SurfaceAggregatorPerfTest : public testing::Test {
 public:
  SurfaceAggregatorPerfTest() {
    context_provider_ = TestContextProvider::Create();
    context_provider_->BindToCurrentThread();
    shared_bitmap_manager_ = std::make_unique<TestSharedBitmapManager>();

    resource_provider_ =
        cc::FakeResourceProvider::CreateDisplayResourceProvider(
            context_provider_.get(), shared_bitmap_manager_.get());
  }

  void RunTest(int num_surfaces,
               int num_textures,
               float opacity,
               bool optimize_damage,
               bool full_damage,
               const std::string& name) {
    std::vector<std::unique_ptr<CompositorFrameSinkSupport>> child_supports(
        num_surfaces);
    for (int i = 0; i < num_surfaces; i++) {
      child_supports[i] = std::make_unique<CompositorFrameSinkSupport>(
          nullptr, &manager_, FrameSinkId(1, i + 1), kIsChildRoot,
          kNeedsSyncPoints);
    }
    aggregator_ = std::make_unique<SurfaceAggregator>(
        manager_.surface_manager(), resource_provider_.get(), optimize_damage);
    for (int i = 0; i < num_surfaces; i++) {
      LocalSurfaceId local_surface_id(i + 1, kArbitraryToken);

      auto pass = RenderPass::Create();
      pass->output_rect = gfx::Rect(0, 0, 1, 2);

      CompositorFrameBuilder frame_builder;

      auto* sqs = pass->CreateAndAppendSharedQuadState();
      for (int j = 0; j < num_textures; j++) {
        TransferableResource resource;
        resource.id = j;
        resource.is_software = true;
        frame_builder.AddTransferableResource(resource);

        auto* quad = pass->CreateAndAppendDrawQuad<TextureDrawQuad>();
        const gfx::Rect rect(0, 0, 1, 2);
        // Half of rects should be visible with partial damage.
        gfx::Rect visible_rect =
            j % 2 == 0 ? gfx::Rect(0, 0, 1, 2) : gfx::Rect(0, 1, 1, 1);
        bool needs_blending = false;
        bool premultiplied_alpha = false;
        const gfx::PointF uv_top_left;
        const gfx::PointF uv_bottom_right;
        SkColor background_color = SK_ColorGREEN;
        const float vertex_opacity[4] = {0.f, 0.f, 1.f, 1.f};
        bool flipped = false;
        bool nearest_neighbor = false;
        quad->SetAll(sqs, rect, visible_rect, needs_blending, j, gfx::Size(),
                     premultiplied_alpha, uv_top_left, uv_bottom_right,
                     background_color, vertex_opacity, flipped,
                     nearest_neighbor, false);
      }
      sqs = pass->CreateAndAppendSharedQuadState();
      sqs->opacity = opacity;
      if (i >= 1) {
        auto* surface_quad = pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
        surface_quad->SetNew(
            sqs, gfx::Rect(0, 0, 1, 1), gfx::Rect(0, 0, 1, 1),
            SurfaceId(FrameSinkId(1, i), LocalSurfaceId(i, kArbitraryToken)),
            base::nullopt, SK_ColorWHITE, false);
      }

      frame_builder.AddRenderPass(std::move(pass));
      child_supports[i]->SubmitCompositorFrame(local_surface_id,
                                               frame_builder.Build());
    }

    auto root_support = std::make_unique<CompositorFrameSinkSupport>(
        nullptr, &manager_, FrameSinkId(1, num_surfaces + 1), kIsRoot,
        kNeedsSyncPoints);
    base::TimeTicks next_fake_display_time =
        base::TimeTicks() + base::TimeDelta::FromSeconds(1);
    timer_.Reset();
    do {
      auto pass = RenderPass::Create();

      auto* sqs = pass->CreateAndAppendSharedQuadState();
      auto* surface_quad = pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
      surface_quad->SetNew(
          sqs, gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 100, 100),
          SurfaceId(FrameSinkId(1, num_surfaces),
                    LocalSurfaceId(num_surfaces, kArbitraryToken)),
          base::nullopt, SK_ColorWHITE, false);

      pass->output_rect = gfx::Rect(0, 0, 100, 100);

      if (full_damage)
        pass->damage_rect = gfx::Rect(0, 0, 100, 100);
      else
        pass->damage_rect = gfx::Rect(0, 0, 1, 1);

      CompositorFrame frame =
          CompositorFrameBuilder().AddRenderPass(std::move(pass)).Build();

      root_support->SubmitCompositorFrame(
          LocalSurfaceId(num_surfaces + 1, kArbitraryToken), std::move(frame));

      CompositorFrame aggregated = aggregator_->Aggregate(
          SurfaceId(FrameSinkId(1, num_surfaces + 1),
                    LocalSurfaceId(num_surfaces + 1, kArbitraryToken)),
          next_fake_display_time);
      next_fake_display_time += BeginFrameArgs::DefaultInterval();
      timer_.NextLap();
    } while (!timer_.HasTimeLimitExpired());

    perf_test::PrintResult("aggregator_speed", "", name, timer_.LapsPerSecond(),
                           "runs/s", true);
  }

 protected:
  FrameSinkManagerImpl manager_;
  scoped_refptr<TestContextProvider> context_provider_;
  std::unique_ptr<SharedBitmapManager> shared_bitmap_manager_;
  std::unique_ptr<DisplayResourceProvider> resource_provider_;
  std::unique_ptr<SurfaceAggregator> aggregator_;
  cc::LapTimer timer_;
};

TEST_F(SurfaceAggregatorPerfTest, ManySurfacesOpaque) {
  RunTest(20, 100, 1.f, false, true, "many_surfaces_opaque");
}

TEST_F(SurfaceAggregatorPerfTest, ManySurfacesTransparent) {
  RunTest(20, 100, .5f, false, true, "many_surfaces_transparent");
}

TEST_F(SurfaceAggregatorPerfTest, FewSurfaces) {
  RunTest(3, 1000, 1.f, false, true, "few_surfaces");
}

TEST_F(SurfaceAggregatorPerfTest, ManySurfacesOpaqueDamageCalc) {
  RunTest(20, 100, 1.f, true, true, "many_surfaces_opaque_damage_calc");
}

TEST_F(SurfaceAggregatorPerfTest, ManySurfacesTransparentDamageCalc) {
  RunTest(20, 100, .5f, true, true, "many_surfaces_transparent_damage_calc");
}

TEST_F(SurfaceAggregatorPerfTest, FewSurfacesDamageCalc) {
  RunTest(3, 1000, 1.f, true, true, "few_surfaces_damage_calc");
}

TEST_F(SurfaceAggregatorPerfTest, FewSurfacesAggregateDamaged) {
  RunTest(3, 1000, 1.f, true, false, "few_surfaces_aggregate_damaged");
}

}  // namespace
}  // namespace viz
