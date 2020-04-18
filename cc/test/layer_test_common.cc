// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/layer_test_common.h"

#include <stddef.h>

#include "cc/animation/animation.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_id_provider.h"
#include "cc/base/math_util.h"
#include "cc/base/region.h"
#include "cc/layers/append_quads_data.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/test/mock_occlusion_tracker.h"
#include "cc/trees/layer_tree_host_common.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/quads/render_pass.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace cc {

// Align with expected and actual output.
const char* LayerTestCommon::quad_string = "    Quad: ";

RenderSurfaceImpl* GetRenderSurface(LayerImpl* layer_impl) {
  EffectTree& effect_tree =
      layer_impl->layer_tree_impl()->property_trees()->effect_tree;

  if (RenderSurfaceImpl* surface =
          effect_tree.GetRenderSurface(layer_impl->effect_tree_index()))
    return surface;

  return effect_tree.GetRenderSurface(
      effect_tree.Node(layer_impl->effect_tree_index())->target_id);
}

static bool CanRectFBeSafelyRoundedToRect(const gfx::RectF& r) {
  // Ensure that range of float values is not beyond integer range.
  if (!r.IsExpressibleAsRect())
    return false;

  // Ensure that the values are actually integers.
  gfx::RectF floored_rect(std::floor(r.x()), std::floor(r.y()),
                          std::floor(r.width()), std::floor(r.height()));
  return floored_rect == r;
}

void LayerTestCommon::VerifyQuadsExactlyCoverRect(const viz::QuadList& quads,
                                                  const gfx::Rect& rect) {
  Region remaining = rect;

  for (auto iter = quads.cbegin(); iter != quads.cend(); ++iter) {
    EXPECT_TRUE(iter->rect.Contains(iter->visible_rect));

    gfx::RectF quad_rectf = MathUtil::MapClippedRect(
        iter->shared_quad_state->quad_to_target_transform,
        gfx::RectF(iter->visible_rect));

    // Before testing for exact coverage in the integer world, assert that
    // rounding will not round the rect incorrectly.
    ASSERT_TRUE(CanRectFBeSafelyRoundedToRect(quad_rectf));

    gfx::Rect quad_rect = gfx::ToEnclosingRect(quad_rectf);

    EXPECT_TRUE(rect.Contains(quad_rect)) << quad_string << iter.index()
                                          << " rect: " << rect.ToString()
                                          << " quad: " << quad_rect.ToString();
    EXPECT_TRUE(remaining.Contains(quad_rect))
        << quad_string << iter.index() << " remaining: " << remaining.ToString()
        << " quad: " << quad_rect.ToString();
    remaining.Subtract(quad_rect);
  }

  EXPECT_TRUE(remaining.IsEmpty());
}

// static
void LayerTestCommon::VerifyQuadsAreOccluded(const viz::QuadList& quads,
                                             const gfx::Rect& occluded,
                                             size_t* partially_occluded_count) {
  // No quad should exist if it's fully occluded.
  for (auto* quad : quads) {
    gfx::Rect target_visible_rect = MathUtil::MapEnclosingClippedRect(
        quad->shared_quad_state->quad_to_target_transform, quad->visible_rect);
    EXPECT_FALSE(occluded.Contains(target_visible_rect));
  }

  // Quads that are fully occluded on one axis only should be shrunken.
  for (auto* quad : quads) {
    gfx::Rect target_rect = MathUtil::MapEnclosingClippedRect(
        quad->shared_quad_state->quad_to_target_transform, quad->rect);
    if (!quad->shared_quad_state->quad_to_target_transform
             .IsIdentityOrIntegerTranslation()) {
      DCHECK(quad->shared_quad_state->quad_to_target_transform
                 .IsPositiveScaleOrTranslation())
          << quad->shared_quad_state->quad_to_target_transform.ToString();
      gfx::RectF target_rectf = MathUtil::MapClippedRect(
          quad->shared_quad_state->quad_to_target_transform,
          gfx::RectF(quad->rect));
      // Scale transforms allowed, as long as the final transformed rect
      // ends up on integer boundaries for ease of testing.
      ASSERT_EQ(target_rectf, gfx::RectF(target_rect));
    }

    bool fully_occluded_horizontal = target_rect.x() >= occluded.x() &&
                                     target_rect.right() <= occluded.right();
    bool fully_occluded_vertical = target_rect.y() >= occluded.y() &&
                                   target_rect.bottom() <= occluded.bottom();
    bool should_be_occluded =
        target_rect.Intersects(occluded) &&
        (fully_occluded_vertical || fully_occluded_horizontal);
    if (!should_be_occluded) {
      EXPECT_EQ(quad->rect.ToString(), quad->visible_rect.ToString());
    } else {
      EXPECT_NE(quad->rect.ToString(), quad->visible_rect.ToString());
      EXPECT_TRUE(quad->rect.Contains(quad->visible_rect));
      ++(*partially_occluded_count);
    }
  }
}

LayerTestCommon::LayerImplTest::LayerImplTest()
    : LayerImplTest(LayerTreeSettings()) {}

LayerTestCommon::LayerImplTest::LayerImplTest(
    std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink)
    : LayerImplTest(LayerTreeSettings(), std::move(layer_tree_frame_sink)) {}

LayerTestCommon::LayerImplTest::LayerImplTest(const LayerTreeSettings& settings)
    : LayerImplTest(settings, FakeLayerTreeFrameSink::Create3d()) {}

LayerTestCommon::LayerImplTest::LayerImplTest(
    const LayerTreeSettings& settings,
    std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink)
    : layer_tree_frame_sink_(std::move(layer_tree_frame_sink)),
      animation_host_(AnimationHost::CreateForTesting(ThreadInstance::MAIN)),
      host_(FakeLayerTreeHost::Create(&client_,
                                      &task_graph_runner_,
                                      animation_host_.get(),
                                      settings)),
      render_pass_(viz::RenderPass::Create()),
      layer_impl_id_(2) {
  std::unique_ptr<LayerImpl> root =
      LayerImpl::Create(host_->host_impl()->active_tree(), 1);
  host_->host_impl()->active_tree()->SetRootLayerForTesting(std::move(root));
  host_->host_impl()->SetVisible(true);
  EXPECT_TRUE(
      host_->host_impl()->InitializeRenderer(layer_tree_frame_sink_.get()));

  const int timeline_id = AnimationIdProvider::NextTimelineId();
  timeline_ = AnimationTimeline::Create(timeline_id);
  animation_host_->AddAnimationTimeline(timeline_);
  // Create impl-side instance.
  animation_host_->PushPropertiesTo(host_->host_impl()->animation_host());
  timeline_impl_ =
      host_->host_impl()->animation_host()->GetTimelineById(timeline_id);
}

LayerTestCommon::LayerImplTest::~LayerImplTest() {
  animation_host_->RemoveAnimationTimeline(timeline_);
  timeline_ = nullptr;
  host_->host_impl()->ReleaseLayerTreeFrameSink();
}

void LayerTestCommon::LayerImplTest::CalcDrawProps(
    const gfx::Size& viewport_size) {
  RenderSurfaceList render_surface_list;
  LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
      root_layer_for_testing(), viewport_size, &render_surface_list);
  LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);
}

void LayerTestCommon::LayerImplTest::AppendQuadsWithOcclusion(
    LayerImpl* layer_impl,
    const gfx::Rect& occluded) {
  AppendQuadsData data;

  render_pass_->quad_list.clear();
  render_pass_->shared_quad_state_list.clear();

  Occlusion occlusion(layer_impl->DrawTransform(),
                      SimpleEnclosedRegion(occluded), SimpleEnclosedRegion());
  layer_impl->draw_properties().occlusion_in_content_space = occlusion;

  layer_impl->WillDraw(DRAW_MODE_HARDWARE, resource_provider());
  layer_impl->AppendQuads(render_pass_.get(), &data);
  layer_impl->DidDraw(resource_provider());
}

void LayerTestCommon::LayerImplTest::AppendQuadsForPassWithOcclusion(
    LayerImpl* layer_impl,
    viz::RenderPass* given_render_pass,
    const gfx::Rect& occluded) {
  AppendQuadsData data;

  given_render_pass->quad_list.clear();
  given_render_pass->shared_quad_state_list.clear();

  Occlusion occlusion(layer_impl->DrawTransform(),
                      SimpleEnclosedRegion(occluded), SimpleEnclosedRegion());
  layer_impl->draw_properties().occlusion_in_content_space = occlusion;

  layer_impl->WillDraw(DRAW_MODE_HARDWARE, resource_provider());
  layer_impl->AppendQuads(given_render_pass, &data);
  layer_impl->DidDraw(resource_provider());
}

void LayerTestCommon::LayerImplTest::AppendSurfaceQuadsWithOcclusion(
    RenderSurfaceImpl* surface_impl,
    const gfx::Rect& occluded) {
  AppendQuadsData data;

  render_pass_->quad_list.clear();
  render_pass_->shared_quad_state_list.clear();

  surface_impl->set_occlusion_in_content_space(
      Occlusion(gfx::Transform(), SimpleEnclosedRegion(occluded),
                SimpleEnclosedRegion()));
  surface_impl->AppendQuads(resource_provider()->IsSoftware()
                                ? DRAW_MODE_SOFTWARE
                                : DRAW_MODE_HARDWARE,
                            render_pass_.get(), &data);
}

void LayerTestCommon::LayerImplTest::RequestCopyOfOutput() {
  root_layer_for_testing()->test_properties()->copy_requests.push_back(
      viz::CopyOutputRequest::CreateStubForTesting());
}

}  // namespace cc
