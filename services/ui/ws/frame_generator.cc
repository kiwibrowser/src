// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/frame_generator.h"

#include <utility>
#include <vector>

#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "components/viz/common/quads/shared_quad_state.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "ui/gfx/geometry/size_f.h"

namespace ui {
namespace ws {

FrameGenerator::FrameGenerator() = default;

FrameGenerator::~FrameGenerator() = default;

void FrameGenerator::SetDeviceScaleFactor(float device_scale_factor) {
  if (device_scale_factor_ == device_scale_factor)
    return;
  device_scale_factor_ = device_scale_factor;
  SetNeedsBeginFrame(true);
}

void FrameGenerator::SetHighContrastMode(bool enabled) {
  if (high_contrast_mode_enabled_ == enabled)
    return;

  high_contrast_mode_enabled_ = enabled;
  SetNeedsBeginFrame(true);
}

void FrameGenerator::SetEmbeddedSurface(const viz::SurfaceInfo& surface_info) {
  DCHECK(surface_info.is_valid());

  // Only handle embedded surfaces changing here. The display root surface
  // changing is handled immediately after the CompositorFrame is submitted.
  if (surface_info != window_manager_surface_info_) {
    window_manager_surface_info_ = surface_info;
    SetNeedsBeginFrame(true);
  }
}

void FrameGenerator::SwapSurfaceWith(FrameGenerator* other) {
  viz::SurfaceInfo window_manager_surface_info = window_manager_surface_info_;
  window_manager_surface_info_ = other->window_manager_surface_info_;
  other->window_manager_surface_info_ = window_manager_surface_info;
  if (window_manager_surface_info_.is_valid())
    SetNeedsBeginFrame(true);
  if (other->window_manager_surface_info_.is_valid())
    other->SetNeedsBeginFrame(true);
}

void FrameGenerator::OnWindowDamaged() {
  SetNeedsBeginFrame(true);
}

void FrameGenerator::OnWindowSizeChanged(const gfx::Size& pixel_size) {
  if (pixel_size_ == pixel_size)
    return;

  pixel_size_ = pixel_size;
  SetNeedsBeginFrame(true);
}

void FrameGenerator::Bind(
    std::unique_ptr<viz::mojom::CompositorFrameSink> compositor_frame_sink) {
  DCHECK(!compositor_frame_sink_);
  compositor_frame_sink_ = std::move(compositor_frame_sink);
}

void FrameGenerator::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  // Nothing to do here because FrameGenerator CompositorFrames don't reference
  // any resources.
  DCHECK(resources.empty());
}

void FrameGenerator::DidReceiveCompositorFrameAck(
    const std::vector<viz::ReturnedResource>& resources) {}

void FrameGenerator::DidPresentCompositorFrame(uint32_t presentation_token,
                                               base::TimeTicks time,
                                               base::TimeDelta refresh,
                                               uint32_t flags) {
  NOTIMPLEMENTED();
}

void FrameGenerator::DidDiscardCompositorFrame(uint32_t presentation_token) {
  NOTIMPLEMENTED();
}

void FrameGenerator::OnBeginFrame(const viz::BeginFrameArgs& begin_frame_args) {
  DCHECK(compositor_frame_sink_);
  current_begin_frame_ack_ = viz::BeginFrameAck(
      begin_frame_args.source_id, begin_frame_args.sequence_number, false);
  if (begin_frame_args.type == viz::BeginFrameArgs::MISSED) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack_);
    return;
  }

  current_begin_frame_ack_.has_damage = true;
  last_begin_frame_args_ = begin_frame_args;

  // TODO(fsamuel): We should add a trace for generating a top level frame.
  viz::CompositorFrame frame(GenerateCompositorFrame());
  if (!id_allocator_.GetCurrentLocalSurfaceId().is_valid() ||
      frame.size_in_pixels() != last_submitted_frame_size_ ||
      frame.device_scale_factor() != last_device_scale_factor_) {
    last_device_scale_factor_ = frame.device_scale_factor();
    last_submitted_frame_size_ = frame.size_in_pixels();
    id_allocator_.GenerateId();
  }

  compositor_frame_sink_->SubmitCompositorFrame(
      id_allocator_.GetCurrentLocalSurfaceId(), std::move(frame),
      GenerateHitTestRegionList(), 0);

  SetNeedsBeginFrame(false);
}

viz::CompositorFrame FrameGenerator::GenerateCompositorFrame() {
  const int render_pass_id = 1;
  const gfx::Rect bounds(pixel_size_);
  std::unique_ptr<viz::RenderPass> render_pass = viz::RenderPass::Create();
  render_pass->SetNew(render_pass_id, bounds, bounds, gfx::Transform());

  DrawWindow(render_pass.get());

  viz::CompositorFrame frame;
  frame.render_pass_list.push_back(std::move(render_pass));
  if (high_contrast_mode_enabled_) {
    std::unique_ptr<viz::RenderPass> invert_pass = viz::RenderPass::Create();
    invert_pass->SetNew(2, bounds, bounds, gfx::Transform());
    viz::SharedQuadState* shared_state =
        invert_pass->CreateAndAppendSharedQuadState();
    gfx::Size scaled_bounds = gfx::ScaleToCeiledSize(
        pixel_size_, window_manager_surface_info_.device_scale_factor(),
        window_manager_surface_info_.device_scale_factor());
    bool is_clipped = false;
    bool are_contents_opaque = false;
    shared_state->SetAll(gfx::Transform(), gfx::Rect(scaled_bounds), bounds,
                         bounds, is_clipped, are_contents_opaque, 1.f,
                         SkBlendMode::kSrcOver, 0);
    auto* quad =
        invert_pass->CreateAndAppendDrawQuad<viz::RenderPassDrawQuad>();
    frame.render_pass_list.back()->filters.Append(
        cc::FilterOperation::CreateInvertFilter(1.f));
    quad->SetNew(
        shared_state, bounds, bounds, render_pass_id, 0 /* mask_resource_id */,
        gfx::RectF() /* mask_uv_rect */, gfx::Size() /* mask_texture_size */,
        gfx::Vector2dF() /* filters_scale */,
        gfx::PointF() /* filters_origin */, gfx::RectF() /* tex_coord_rect */,
        false /* force_anti_aliasing_off */);
    frame.render_pass_list.push_back(std::move(invert_pass));
  }
  frame.metadata.device_scale_factor = device_scale_factor_;
  frame.metadata.begin_frame_ack = current_begin_frame_ack_;

  if (window_manager_surface_info_.is_valid()) {
    frame.metadata.referenced_surfaces.push_back(
        window_manager_surface_info_.id());
  }

  return frame;
}

viz::HitTestRegionList FrameGenerator::GenerateHitTestRegionList() const {
  viz::HitTestRegionList hit_test_region_list;
  hit_test_region_list.flags = viz::HitTestRegionFlags::kHitTestMine;
  hit_test_region_list.bounds.set_size(pixel_size_);

  viz::HitTestRegion hit_test_region;
  hit_test_region.frame_sink_id =
      window_manager_surface_info_.id().frame_sink_id();
  hit_test_region.flags = viz::HitTestRegionFlags::kHitTestChildSurface;
  hit_test_region.rect = gfx::Rect(pixel_size_);

  hit_test_region_list.regions.push_back(std::move(hit_test_region));

  return hit_test_region_list;
}

void FrameGenerator::DrawWindow(viz::RenderPass* pass) {
  DCHECK(window_manager_surface_info_.is_valid());

  const gfx::Rect bounds_at_origin(
      window_manager_surface_info_.size_in_pixels());

  gfx::Transform quad_to_target_transform;

  if (scale_and_center_) {
    // Determine the scaling to fit the source within the target.
    gfx::SizeF source(window_manager_surface_info_.size_in_pixels());
    const gfx::SizeF target(pixel_size_);
    const float scale = std::min(target.width() / source.width(),
                                 target.height() / source.height());

    // Apply the transform to center the source within the output.
    source.Scale(scale);
    DCHECK(source.width() <= target.width() ||
           source.height() <= target.height());
    if (source.width() < target.width()) {
      quad_to_target_transform.Translate(
          (target.width() - source.width()) / 2.0f, 0);
    } else if (source.height() < target.height()) {
      quad_to_target_transform.Translate(
          0, (target.height() - source.height()) / 2.0f);
    }

    // Apply the scaling after the transform.
    quad_to_target_transform.Scale(scale, scale);
  }

  viz::SharedQuadState* sqs = pass->CreateAndAppendSharedQuadState();

  gfx::Size scaled_bounds = gfx::ScaleToCeiledSize(
      bounds_at_origin.size(),
      window_manager_surface_info_.device_scale_factor(),
      window_manager_surface_info_.device_scale_factor());

  // TODO(fsamuel): These clipping and visible rects are incorrect. They need
  // to be populated from CompositorFrame structs.
  sqs->SetAll(quad_to_target_transform,
              gfx::Rect(scaled_bounds) /* layer_rect */,
              bounds_at_origin /* visible_layer_bounds */,
              bounds_at_origin /* clip_rect */, false /* is_clipped */,
              false /* are_contents_opaque */, 1.0f /* opacity */,
              SkBlendMode::kSrcOver, 0 /* sorting-context_id */);
  auto* quad = pass->CreateAndAppendDrawQuad<viz::SurfaceDrawQuad>();
  quad->SetAll(sqs, bounds_at_origin /* rect */,
               bounds_at_origin /* visible_rect */, true /* needs_blending*/,
               window_manager_surface_info_.id(),
               window_manager_surface_info_.id(),
               SK_ColorWHITE /* default_background_color */,
               false /* stretch_content_to_fill_bounds */);
}

void FrameGenerator::SetNeedsBeginFrame(bool needs_begin_frame) {
  needs_begin_frame &= window_manager_surface_info_.is_valid();
  compositor_frame_sink_->SetNeedsBeginFrame(needs_begin_frame);
}

}  // namespace ws
}  // namespace ui
