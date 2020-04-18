// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/render_surface_impl.h"

#include <stddef.h>

#include "cc/layers/append_quads_data.h"
#include "cc/test/fake_mask_layer_impl.h"
#include "cc/test/fake_raster_source.h"
#include "cc/test/layer_test_common.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(RenderSurfaceLayerImplTest, Occlusion) {
  gfx::Size layer_size(1000, 1000);
  gfx::Size viewport_size(1000, 1000);

  LayerTestCommon::LayerImplTest impl;

  LayerImpl* owning_layer_impl = impl.AddChildToRoot<LayerImpl>();
  owning_layer_impl->SetBounds(layer_size);
  owning_layer_impl->SetDrawsContent(true);
  owning_layer_impl->test_properties()->force_render_surface = true;

  impl.CalcDrawProps(viewport_size);

  RenderSurfaceImpl* render_surface_impl = GetRenderSurface(owning_layer_impl);
  ASSERT_TRUE(render_surface_impl);

  {
    SCOPED_TRACE("No occlusion");
    gfx::Rect occluded;
    impl.AppendSurfaceQuadsWithOcclusion(render_surface_impl, occluded);

    LayerTestCommon::VerifyQuadsExactlyCoverRect(impl.quad_list(),
                                                 gfx::Rect(layer_size));
    EXPECT_EQ(1u, impl.quad_list().size());
  }

  {
    SCOPED_TRACE("Full occlusion");
    gfx::Rect occluded(owning_layer_impl->visible_layer_rect());
    impl.AppendSurfaceQuadsWithOcclusion(render_surface_impl, occluded);

    LayerTestCommon::VerifyQuadsExactlyCoverRect(impl.quad_list(), gfx::Rect());
    EXPECT_EQ(impl.quad_list().size(), 0u);
  }

  {
    SCOPED_TRACE("Partial occlusion");
    gfx::Rect occluded(200, 0, 800, 1000);
    impl.AppendSurfaceQuadsWithOcclusion(render_surface_impl, occluded);

    size_t partially_occluded_count = 0;
    LayerTestCommon::VerifyQuadsAreOccluded(
        impl.quad_list(), occluded, &partially_occluded_count);
    // The layer outputs one quad, which is partially occluded.
    EXPECT_EQ(1u, impl.quad_list().size());
    EXPECT_EQ(1u, partially_occluded_count);
  }
}

static std::unique_ptr<viz::RenderPass> DoAppendQuadsWithScaledMask(
    DrawMode draw_mode,
    float device_scale_factor,
    Layer::LayerMaskType mask_type) {
  gfx::Size layer_size(1000, 1000);
  gfx::Size viewport_size(1000, 1000);
  float scale_factor = 2;
  scoped_refptr<FakeRasterSource> raster_source =
      FakeRasterSource::CreateFilledSolidColor(layer_size);

  LayerTreeSettings settings;
  settings.layer_transforms_should_scale_layer_contents = true;
  LayerTestCommon::LayerImplTest impl(settings);
  std::unique_ptr<LayerImpl> root =
      LayerImpl::Create(impl.host_impl()->active_tree(), 2);
  std::unique_ptr<LayerImpl> surface =
      LayerImpl::Create(impl.host_impl()->active_tree(), 3);
  surface->SetBounds(layer_size);
  surface->test_properties()->force_render_surface = true;

  gfx::Transform scale;
  scale.Scale(scale_factor, scale_factor);
  surface->test_properties()->transform = scale;

  std::unique_ptr<FakeMaskLayerImpl> mask_layer = FakeMaskLayerImpl::Create(
      impl.host_impl()->active_tree(), 4, raster_source, mask_type);
  mask_layer->set_resource_size(
      gfx::ScaleToCeiledSize(layer_size, scale_factor));
  mask_layer->SetDrawsContent(true);
  mask_layer->SetBounds(layer_size);
  surface->test_properties()->SetMaskLayer(std::move(mask_layer));

  std::unique_ptr<LayerImpl> child =
      LayerImpl::Create(impl.host_impl()->active_tree(), 5);
  child->SetDrawsContent(true);
  child->SetBounds(layer_size);

  surface->test_properties()->AddChild(std::move(child));
  root->test_properties()->AddChild(std::move(surface));
  impl.host_impl()->active_tree()->SetRootLayerForTesting(std::move(root));

  impl.host_impl()->active_tree()->SetDeviceScaleFactor(device_scale_factor);
  impl.host_impl()->SetViewportSize(viewport_size);
  impl.host_impl()->active_tree()->BuildLayerListAndPropertyTreesForTesting();
  impl.host_impl()->active_tree()->UpdateDrawProperties();

  LayerImpl* surface_raw = impl.host_impl()
                               ->active_tree()
                               ->root_layer_for_testing()
                               ->test_properties()
                               ->children[0];
  RenderSurfaceImpl* render_surface_impl = GetRenderSurface(surface_raw);
  std::unique_ptr<viz::RenderPass> render_pass = viz::RenderPass::Create();
  AppendQuadsData append_quads_data;
  render_surface_impl->AppendQuads(draw_mode, render_pass.get(),
                                   &append_quads_data);
  return render_pass;
}

TEST(RenderSurfaceLayerImplTest, AppendQuadsWithScaledMask) {
  std::unique_ptr<viz::RenderPass> render_pass = DoAppendQuadsWithScaledMask(
      DRAW_MODE_HARDWARE, 1.f, Layer::LayerMaskType::SINGLE_TEXTURE_MASK);
  DCHECK(render_pass->quad_list.front());
  const viz::RenderPassDrawQuad* quad =
      viz::RenderPassDrawQuad::MaterialCast(render_pass->quad_list.front());
  EXPECT_EQ(gfx::RectF(0, 0, 1, 1), quad->mask_uv_rect);
  EXPECT_EQ(gfx::Vector2dF(2.f, 2.f), quad->filters_scale);
}

TEST(RenderSurfaceLayerImplTest, ResourcelessAppendQuadsSkipMask) {
  std::unique_ptr<viz::RenderPass> render_pass =
      DoAppendQuadsWithScaledMask(DRAW_MODE_RESOURCELESS_SOFTWARE, 1.f,
                                  Layer::LayerMaskType::SINGLE_TEXTURE_MASK);
  DCHECK(render_pass->quad_list.front());
  const viz::RenderPassDrawQuad* quad =
      viz::RenderPassDrawQuad::MaterialCast(render_pass->quad_list.front());
  EXPECT_EQ(0u, quad->mask_resource_id());
}

TEST(RenderSurfaceLayerImplTest,
     AppendQuadsWithSolidColorMaskAndDeviceScaleFactor) {
  std::unique_ptr<viz::RenderPass> render_pass = DoAppendQuadsWithScaledMask(
      DRAW_MODE_HARDWARE, 2.f, Layer::LayerMaskType::MULTI_TEXTURE_MASK);
  DCHECK(render_pass->quad_list.front());
  const viz::RenderPassDrawQuad* quad =
      viz::RenderPassDrawQuad::MaterialCast(render_pass->quad_list.front());
  EXPECT_EQ(gfx::Transform(),
            quad->shared_quad_state->quad_to_target_transform);
  // With tiled mask layer, we only generate mask quads for visible rect. In
  // this case |quad_layer_rect| is not fully covered, but
  // |visible_quad_layer_rect| is fully covered.
  LayerTestCommon::VerifyQuadsExactlyCoverRect(
      render_pass->quad_list, quad->shared_quad_state->visible_quad_layer_rect);
}

}  // namespace
}  // namespace cc
