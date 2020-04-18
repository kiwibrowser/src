// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/surface_hittest_test_helpers.h"

#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "components/viz/common/quads/shared_quad_state.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/test/compositor_frame_helpers.h"

namespace viz {
namespace test {

void CreateSharedQuadState(RenderPass* pass,
                           const gfx::Transform& transform,
                           const gfx::Rect& root_rect) {
  SharedQuadState* child_shared_state = pass->CreateAndAppendSharedQuadState();
  child_shared_state->SetAll(transform, root_rect, root_rect, root_rect, false,
                             false, 1.0f, SkBlendMode::kSrcOver, 0);
}

void CreateSolidColorDrawQuad(RenderPass* pass,
                              const gfx::Transform& transform,
                              const gfx::Rect& root_rect,
                              const gfx::Rect& quad_rect) {
  CreateSharedQuadState(pass, transform, root_rect);
  SolidColorDrawQuad* color_quad =
      pass->CreateAndAppendDrawQuad<SolidColorDrawQuad>();
  color_quad->SetNew(pass->shared_quad_state_list.back(), quad_rect, quad_rect,
                     SK_ColorYELLOW, false);
}

void CreateRenderPassDrawQuad(RenderPass* pass,
                              const gfx::Transform& transform,
                              const gfx::Rect& root_rect,
                              const gfx::Rect& quad_rect,
                              int render_pass_id) {
  CreateSharedQuadState(pass, transform, root_rect);
  RenderPassDrawQuad* render_pass_quad =
      pass->CreateAndAppendDrawQuad<RenderPassDrawQuad>();
  render_pass_quad->SetNew(pass->shared_quad_state_list.back(), quad_rect,
                           quad_rect, render_pass_id, ResourceId(),
                           gfx::RectF(), gfx::Size(), gfx::Vector2dF(),
                           gfx::PointF(), gfx::RectF(), false);
}

void CreateSurfaceDrawQuad(RenderPass* pass,
                           const gfx::Transform& transform,
                           const gfx::Rect& root_rect,
                           const gfx::Rect& quad_rect,
                           SurfaceId surface_id) {
  CreateSharedQuadState(pass, transform, root_rect);
  SurfaceDrawQuad* surface_quad =
      pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  surface_quad->SetNew(pass->shared_quad_state_list.back(), quad_rect,
                       quad_rect, surface_id, SurfaceId(), SK_ColorWHITE,
                       false);
}

void CreateRenderPass(int render_pass_id,
                      const gfx::Rect& rect,
                      const gfx::Transform& transform_to_root_target,
                      RenderPassList* render_pass_list) {
  std::unique_ptr<RenderPass> render_pass = RenderPass::Create();
  render_pass->SetNew(render_pass_id, rect, rect, transform_to_root_target);
  render_pass_list->push_back(std::move(render_pass));
}

CompositorFrame CreateCompositorFrame(const gfx::Rect& root_rect,
                                      RenderPass** render_pass) {
  CompositorFrame root_frame = MakeDefaultCompositorFrame();
  int root_id = 1;
  CreateRenderPass(root_id, root_rect, gfx::Transform(),
                   &root_frame.render_pass_list);
  *render_pass = root_frame.render_pass_list.back().get();
  return root_frame;
}

TestSurfaceHittestDelegate::TestSurfaceHittestDelegate()
    : reject_target_overrides_(0), accept_target_overrides_(0) {}

TestSurfaceHittestDelegate::~TestSurfaceHittestDelegate() {}

void TestSurfaceHittestDelegate::AddInsetsForRejectSurface(
    const SurfaceId& surface_id,
    const gfx::Insets& inset) {
  insets_for_reject_.insert(std::make_pair(surface_id, inset));
}

void TestSurfaceHittestDelegate::AddInsetsForAcceptSurface(
    const SurfaceId& surface_id,
    const gfx::Insets& inset) {
  insets_for_accept_.insert(std::make_pair(surface_id, inset));
}

bool TestSurfaceHittestDelegate::RejectHitTarget(
    const SurfaceDrawQuad* surface_quad,
    const gfx::Point& point_in_quad_space) {
  if (!insets_for_reject_.count(surface_quad->primary_surface_id))
    return false;
  gfx::Rect bounds(surface_quad->rect);
  bounds.Inset(insets_for_reject_[surface_quad->primary_surface_id]);
  // If the point provided falls outside the inset, then we skip this surface.
  if (!bounds.Contains(point_in_quad_space)) {
    if (surface_quad->rect.Contains(point_in_quad_space))
      ++reject_target_overrides_;
    return true;
  }
  return false;
}

bool TestSurfaceHittestDelegate::AcceptHitTarget(
    const SurfaceDrawQuad* surface_quad,
    const gfx::Point& point_in_quad_space) {
  if (!insets_for_accept_.count(surface_quad->primary_surface_id))
    return false;
  gfx::Rect bounds(surface_quad->rect);
  bounds.Inset(insets_for_accept_[surface_quad->primary_surface_id]);
  // If the point provided falls outside the inset, then we accept this surface.
  if (!bounds.Contains(point_in_quad_space)) {
    ++accept_target_overrides_;
    return true;
  }
  return false;
}

}  // namespace test
}  // namespace viz
