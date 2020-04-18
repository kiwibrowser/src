// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/direct_layer_tree_frame_sink.h"

#include <memory>

#include "base/test/simple_test_tick_clock.h"
#include "cc/test/fake_layer_tree_frame_sink_client.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/frame_sinks/delay_based_time_source.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/display/display_scheduler.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support_manager.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/test/begin_frame_args_test.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/fake_output_surface.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "components/viz/test/test_shared_bitmap_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace {

static constexpr FrameSinkId kArbitraryFrameSinkId(1, 1);

class TestDirectLayerTreeFrameSink : public DirectLayerTreeFrameSink {
 public:
  using DirectLayerTreeFrameSink::DirectLayerTreeFrameSink;
};

class TestCompositorFrameSinkSupportManager
    : public CompositorFrameSinkSupportManager {
 public:
  explicit TestCompositorFrameSinkSupportManager(
      FrameSinkManagerImpl* frame_sink_manager)
      : frame_sink_manager_(frame_sink_manager) {}
  ~TestCompositorFrameSinkSupportManager() override = default;

  std::unique_ptr<CompositorFrameSinkSupport> CreateCompositorFrameSinkSupport(
      mojom::CompositorFrameSinkClient* client,
      const FrameSinkId& frame_sink_id,
      bool is_root,
      bool needs_sync_points) override {
    return std::make_unique<CompositorFrameSinkSupport>(
        client, frame_sink_manager_, frame_sink_id, is_root, needs_sync_points);
  }

 private:
  FrameSinkManagerImpl* const frame_sink_manager_;

  DISALLOW_COPY_AND_ASSIGN(TestCompositorFrameSinkSupportManager);
};

class DirectLayerTreeFrameSinkTest : public testing::Test {
 public:
  DirectLayerTreeFrameSinkTest()
      : now_src_(new base::SimpleTestTickClock()),
        task_runner_(new cc::OrderedSimpleTaskRunner(now_src_.get(), true)),
        display_size_(1920, 1080),
        display_rect_(display_size_),
        support_manager_(&frame_sink_manager_),
        context_provider_(TestContextProvider::Create()) {
    auto display_output_surface = FakeOutputSurface::Create3d();
    display_output_surface_ = display_output_surface.get();

    begin_frame_source_ = std::make_unique<BackToBackBeginFrameSource>(
        std::make_unique<DelayBasedTimeSource>(task_runner_.get()));

    int max_frames_pending = 2;
    auto scheduler = std::make_unique<DisplayScheduler>(
        begin_frame_source_.get(), task_runner_.get(), max_frames_pending);

    display_ = std::make_unique<Display>(
        &bitmap_manager_, RendererSettings(), kArbitraryFrameSinkId,
        std::move(display_output_surface), std::move(scheduler), task_runner_);
    layer_tree_frame_sink_ = std::make_unique<TestDirectLayerTreeFrameSink>(
        kArbitraryFrameSinkId, &support_manager_, &frame_sink_manager_,
        display_.get(), nullptr /* display_client */, context_provider_,
        nullptr, task_runner_, &gpu_memory_buffer_manager_,
        false /* use_viz_hit_test */);
    layer_tree_frame_sink_->BindToClient(&layer_tree_frame_sink_client_);
    display_->Resize(display_size_);
    display_->SetVisible(true);

    EXPECT_FALSE(
        layer_tree_frame_sink_client_.did_lose_layer_tree_frame_sink_called());
  }

  ~DirectLayerTreeFrameSinkTest() override {
    layer_tree_frame_sink_->DetachFromClient();
  }

  void SwapBuffersWithDamage(const gfx::Rect& damage_rect) {
    auto frame = CompositorFrameBuilder()
                     .AddRenderPass(display_rect_, damage_rect)
                     .Build();
    layer_tree_frame_sink_->SubmitCompositorFrame(std::move(frame));
  }

  void SendRenderPassList(RenderPassList* pass_list) {
    auto frame = CompositorFrameBuilder()
                     .SetRenderPassList(std::move(*pass_list))
                     .Build();
    pass_list->clear();
    layer_tree_frame_sink_->SubmitCompositorFrame(std::move(frame));
  }

  void SetUp() override {
    // Draw the first frame to start in an "unlocked" state.
    SwapBuffersWithDamage(display_rect_);

    EXPECT_EQ(0u, display_output_surface_->num_sent_frames());
    task_runner_->RunUntilIdle();
    EXPECT_EQ(1u, display_output_surface_->num_sent_frames());
  }

 protected:
  std::unique_ptr<base::SimpleTestTickClock> now_src_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> task_runner_;

  const gfx::Size display_size_;
  const gfx::Rect display_rect_;
  FrameSinkManagerImpl frame_sink_manager_;
  TestCompositorFrameSinkSupportManager support_manager_;
  TestSharedBitmapManager bitmap_manager_;
  TestGpuMemoryBufferManager gpu_memory_buffer_manager_;

  scoped_refptr<TestContextProvider> context_provider_;
  FakeOutputSurface* display_output_surface_ = nullptr;
  std::unique_ptr<BackToBackBeginFrameSource> begin_frame_source_;
  std::unique_ptr<Display> display_;
  cc::FakeLayerTreeFrameSinkClient layer_tree_frame_sink_client_;
  std::unique_ptr<TestDirectLayerTreeFrameSink> layer_tree_frame_sink_;
};

TEST_F(DirectLayerTreeFrameSinkTest, DamageTriggersSwapBuffers) {
  SwapBuffersWithDamage(display_rect_);
  EXPECT_EQ(1u, display_output_surface_->num_sent_frames());
  task_runner_->RunUntilIdle();
  EXPECT_EQ(2u, display_output_surface_->num_sent_frames());
}

TEST_F(DirectLayerTreeFrameSinkTest, NoDamageDoesNotTriggerSwapBuffers) {
  SwapBuffersWithDamage(gfx::Rect());
  EXPECT_EQ(1u, display_output_surface_->num_sent_frames());
  task_runner_->RunUntilIdle();
  EXPECT_EQ(1u, display_output_surface_->num_sent_frames());
}

// Test that hit_test_region_list are created correctly for the browser.
TEST_F(DirectLayerTreeFrameSinkTest, HitTestRegionList) {
  RenderPassList pass_list;

  // Add a DrawQuad that is not a SurfaceDrawQuad. |hit_test_region_list_|
  // shouldn't have any child regions.
  auto pass1 = RenderPass::Create();
  pass1->output_rect = display_rect_;
  pass1->id = 1;
  auto* shared_quad_state1 = pass1->CreateAndAppendSharedQuadState();
  gfx::Rect rect1(display_rect_);
  shared_quad_state1->SetAll(
      gfx::Transform(), rect1 /* quad_layer_rect */,
      rect1 /* visible_quad_layer_rect */, rect1 /*clip_rect */,
      false /* is_clipped */, false /* are_contents_opaque */,
      0.5f /* opacity */, SkBlendMode::kSrcOver, 0 /* sorting_context_id */);
  auto* quad1 = pass1->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  quad1->SetNew(shared_quad_state1, rect1 /* rect */, rect1 /* visible_rect */,
                SK_ColorBLACK, false /* force_anti_aliasing_off */);
  pass_list.push_back(std::move(pass1));
  SendRenderPassList(&pass_list);
  task_runner_->RunUntilIdle();

  const auto* hit_test_region_list =
      frame_sink_manager_.hit_test_manager()->GetActiveHitTestRegionList(
          display_.get(), display_->CurrentSurfaceId().frame_sink_id());
  EXPECT_TRUE(hit_test_region_list);
  EXPECT_EQ(display_rect_, hit_test_region_list->bounds);
  EXPECT_EQ(HitTestRegionFlags::kHitTestMouse |
                HitTestRegionFlags::kHitTestTouch |
                HitTestRegionFlags::kHitTestMine,
            hit_test_region_list->flags);
  EXPECT_FALSE(hit_test_region_list->regions.size());

  // Add a SurfaceDrawQuad to one render pass, and add a SolidColorDrawQuad to
  // another render pass, and add a SurfaceDrawQuad with a transform that's not
  // invertible. |hit_test_region_list_| should contain one child
  // region corresponding to that first SurfaceDrawQuad.
  const SurfaceId child_surface_id(
      FrameSinkId(1, 1), LocalSurfaceId(1, base::UnguessableToken::Create()));
  auto pass2 = RenderPass::Create();
  pass2->output_rect = display_rect_;
  pass2->id = 2;
  auto* shared_quad_state2 = pass2->CreateAndAppendSharedQuadState();
  gfx::Rect rect2 = gfx::Rect(20, 20);
  gfx::Transform transform2;
  transform2.Translate(-200, -100);
  shared_quad_state2->SetAll(
      transform2, rect2 /* quad_layer_rect */,
      rect2 /* visible_quad_layer_rect */, rect2 /*clip_rect */,
      false /* is_clipped */, false /* are_contents_opaque */,
      0.5f /* opacity */, SkBlendMode::kSrcOver, 0 /* sorting_context_id */);
  auto* quad2 = pass2->quad_list.AllocateAndConstruct<SurfaceDrawQuad>();
  quad2->SetNew(shared_quad_state2, rect2 /* rect */, rect2 /* visible_rect */,
                child_surface_id /* primary_surface_id */,
                base::Optional<SurfaceId>() /* fallback_surface_id */,
                SK_ColorBLACK, false /* stretch_content_to_fill_bounds */);
  pass_list.push_back(std::move(pass2));

  auto pass3 = RenderPass::Create();
  pass3->output_rect = display_rect_;
  pass3->id = 3;
  auto* shared_quad_state3 = pass3->CreateAndAppendSharedQuadState();
  gfx::Rect rect3(display_rect_);
  shared_quad_state3->SetAll(
      gfx::Transform(), rect3 /* quad_layer_rect */,
      rect3 /* visible_quad_layer_rect */, rect3 /*clip_rect */,
      false /* is_clipped */, false /* are_contents_opaque */,
      0.5f /* opacity */, SkBlendMode::kSrcOver, 0 /* sorting_context_id */);
  auto* quad3 = pass3->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  quad3->SetNew(shared_quad_state3, rect3 /* rect */, rect3 /* visible_rect */,
                SK_ColorBLACK, false /* force_anti_aliasing_off */);
  pass_list.push_back(std::move(pass3));
  SendRenderPassList(&pass_list);
  task_runner_->RunUntilIdle();

  const SurfaceId child_surface_id4(
      FrameSinkId(4, 1), LocalSurfaceId(1, base::UnguessableToken::Create()));
  auto pass4 = RenderPass::Create();
  pass4->output_rect = display_rect_;
  pass4->id = 4;
  auto* shared_quad_state4 = pass4->CreateAndAppendSharedQuadState();
  gfx::Rect rect4(display_rect_);
  // A degenerate matrix of all zeros is not invertible.
  gfx::Transform transform4;
  transform4.matrix().set(0, 0, 0.f);
  transform4.matrix().set(1, 1, 0.f);
  transform4.matrix().set(2, 2, 0.f);
  transform4.matrix().set(3, 3, 0.f);
  shared_quad_state4->SetAll(
      transform4, rect4 /* quad_layer_rect */,
      rect4 /* visible_quad_layer_rect */, rect4 /*clip_rect */,
      false /* is_clipped */, false /* are_contents_opaque */,
      0.5f /* opacity */, SkBlendMode::kSrcOver, 0 /* sorting_context_id */);
  auto* quad4 = pass4->quad_list.AllocateAndConstruct<SurfaceDrawQuad>();
  quad4->SetNew(shared_quad_state4, rect4 /* rect */, rect4 /* visible_rect */,
                child_surface_id4 /* primary_surface_id */,
                base::Optional<SurfaceId>() /* fallback_surface_id */,
                SK_ColorBLACK, false /* stretch_content_to_fill_bounds */);
  pass_list.push_back(std::move(pass4));

  const auto* hit_test_region_list1 =
      frame_sink_manager_.hit_test_manager()->GetActiveHitTestRegionList(
          display_.get(), display_->CurrentSurfaceId().frame_sink_id());
  EXPECT_TRUE(hit_test_region_list1);
  EXPECT_EQ(display_rect_, hit_test_region_list1->bounds);
  EXPECT_EQ(HitTestRegionFlags::kHitTestMouse |
                HitTestRegionFlags::kHitTestTouch |
                HitTestRegionFlags::kHitTestMine,
            hit_test_region_list1->flags);
  EXPECT_EQ(1u, hit_test_region_list1->regions.size());
  EXPECT_EQ(child_surface_id.frame_sink_id(),
            hit_test_region_list1->regions[0].frame_sink_id);
  EXPECT_EQ(HitTestRegionFlags::kHitTestMouse |
                HitTestRegionFlags::kHitTestTouch |
                HitTestRegionFlags::kHitTestChildSurface,
            hit_test_region_list1->regions[0].flags);
  EXPECT_EQ(gfx::Rect(20, 20), hit_test_region_list1->regions[0].rect);
  gfx::Transform transform2_inverse;
  EXPECT_TRUE(transform2.GetInverse(&transform2_inverse));
  EXPECT_EQ(transform2_inverse, hit_test_region_list1->regions[0].transform);
}

}  // namespace
}  // namespace viz
