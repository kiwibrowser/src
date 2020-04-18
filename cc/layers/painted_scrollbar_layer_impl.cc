// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/painted_scrollbar_layer_impl.h"

#include <algorithm>

#include "base/memory/ptr_util.h"
#include "cc/input/scrollbar_animation_controller.h"
#include "cc/layers/layer.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/layer_tree_settings.h"
#include "cc/trees/occlusion.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace cc {

std::unique_ptr<PaintedScrollbarLayerImpl> PaintedScrollbarLayerImpl::Create(
    LayerTreeImpl* tree_impl,
    int id,
    ScrollbarOrientation orientation,
    bool is_left_side_vertical_scrollbar,
    bool is_overlay) {
  return base::WrapUnique(new PaintedScrollbarLayerImpl(
      tree_impl, id, orientation, is_left_side_vertical_scrollbar, is_overlay));
}

PaintedScrollbarLayerImpl::PaintedScrollbarLayerImpl(
    LayerTreeImpl* tree_impl,
    int id,
    ScrollbarOrientation orientation,
    bool is_left_side_vertical_scrollbar,
    bool is_overlay)
    : ScrollbarLayerImplBase(tree_impl,
                             id,
                             orientation,
                             is_left_side_vertical_scrollbar,
                             is_overlay),
      track_ui_resource_id_(0),
      thumb_ui_resource_id_(0),
      thumb_opacity_(1.f),
      internal_contents_scale_(1.f),
      thumb_thickness_(0),
      thumb_length_(0),
      track_start_(0),
      track_length_(0) {}

PaintedScrollbarLayerImpl::~PaintedScrollbarLayerImpl() = default;

std::unique_ptr<LayerImpl> PaintedScrollbarLayerImpl::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return PaintedScrollbarLayerImpl::Create(tree_impl, id(), orientation(),
                                           is_left_side_vertical_scrollbar(),
                                           is_overlay_scrollbar());
}

void PaintedScrollbarLayerImpl::PushPropertiesTo(LayerImpl* layer) {
  ScrollbarLayerImplBase::PushPropertiesTo(layer);

  PaintedScrollbarLayerImpl* scrollbar_layer =
      static_cast<PaintedScrollbarLayerImpl*>(layer);

  scrollbar_layer->set_internal_contents_scale_and_bounds(
      internal_contents_scale_, internal_content_bounds_);

  scrollbar_layer->SetThumbThickness(thumb_thickness_);
  scrollbar_layer->SetThumbLength(thumb_length_);
  scrollbar_layer->SetTrackStart(track_start_);
  scrollbar_layer->SetTrackLength(track_length_);

  scrollbar_layer->set_track_ui_resource_id(track_ui_resource_id_);
  scrollbar_layer->set_thumb_ui_resource_id(thumb_ui_resource_id_);

  scrollbar_layer->set_thumb_opacity(thumb_opacity_);
}

bool PaintedScrollbarLayerImpl::WillDraw(
    DrawMode draw_mode,
    LayerTreeResourceProvider* resource_provider) {
  DCHECK(draw_mode != DRAW_MODE_RESOURCELESS_SOFTWARE);
  return LayerImpl::WillDraw(draw_mode, resource_provider);
}

void PaintedScrollbarLayerImpl::AppendQuads(
    viz::RenderPass* render_pass,
    AppendQuadsData* append_quads_data) {
  bool premultipled_alpha = true;
  bool flipped = false;
  bool nearest_neighbor = false;
  gfx::PointF uv_top_left(0.f, 0.f);
  gfx::PointF uv_bottom_right(1.f, 1.f);

  viz::SharedQuadState* shared_quad_state =
      render_pass->CreateAndAppendSharedQuadState();
  PopulateScaledSharedQuadState(shared_quad_state, internal_contents_scale_,
                                internal_contents_scale_, contents_opaque());

  AppendDebugBorderQuad(render_pass, gfx::Rect(internal_content_bounds_),
                        shared_quad_state, append_quads_data);

  gfx::Rect thumb_quad_rect = ComputeThumbQuadRect();
  gfx::Rect scaled_thumb_quad_rect =
      gfx::ScaleToEnclosingRect(thumb_quad_rect, internal_contents_scale_);
  gfx::Rect visible_thumb_quad_rect =
      draw_properties().occlusion_in_content_space.GetUnoccludedContentRect(
          thumb_quad_rect);
  gfx::Rect scaled_visible_thumb_quad_rect = gfx::ScaleToEnclosingRect(
      visible_thumb_quad_rect, internal_contents_scale_);

  viz::ResourceId thumb_resource_id =
      layer_tree_impl()->ResourceIdForUIResource(thumb_ui_resource_id_);
  viz::ResourceId track_resource_id =
      layer_tree_impl()->ResourceIdForUIResource(track_ui_resource_id_);

  if (thumb_resource_id && !visible_thumb_quad_rect.IsEmpty()) {
    bool needs_blending = true;
    const float opacity[] = {thumb_opacity_, thumb_opacity_, thumb_opacity_,
                             thumb_opacity_};
    auto* quad = render_pass->CreateAndAppendDrawQuad<viz::TextureDrawQuad>();
    quad->SetNew(shared_quad_state, scaled_thumb_quad_rect,
                 scaled_visible_thumb_quad_rect, needs_blending,
                 thumb_resource_id, premultipled_alpha, uv_top_left,
                 uv_bottom_right, SK_ColorTRANSPARENT, opacity, flipped,
                 nearest_neighbor, false);
    ValidateQuadResources(quad);
  }

  gfx::Rect track_quad_rect(bounds());
  gfx::Rect scaled_track_quad_rect(internal_content_bounds_);
  gfx::Rect visible_track_quad_rect =
      draw_properties().occlusion_in_content_space.GetUnoccludedContentRect(
          track_quad_rect);
  gfx::Rect scaled_visible_track_quad_rect = gfx::ScaleToEnclosingRect(
      visible_track_quad_rect, internal_contents_scale_);
  if (track_resource_id && !visible_track_quad_rect.IsEmpty()) {
    bool needs_blending = !contents_opaque();
    const float opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
    auto* quad = render_pass->CreateAndAppendDrawQuad<viz::TextureDrawQuad>();
    quad->SetNew(shared_quad_state, scaled_track_quad_rect,
                 scaled_visible_track_quad_rect, needs_blending,
                 track_resource_id, premultipled_alpha, uv_top_left,
                 uv_bottom_right, SK_ColorTRANSPARENT, opacity, flipped,
                 nearest_neighbor, false);
    ValidateQuadResources(quad);
  }
}

gfx::Rect PaintedScrollbarLayerImpl::GetEnclosingRectInTargetSpace() const {
  if (internal_content_bounds_.IsEmpty())
    return gfx::Rect();
  DCHECK_GT(internal_contents_scale_, 0.f);
  return GetScaledEnclosingRectInTargetSpace(internal_contents_scale_);
}

void PaintedScrollbarLayerImpl::SetThumbThickness(int thumb_thickness) {
  if (thumb_thickness_ == thumb_thickness)
    return;
  thumb_thickness_ = thumb_thickness;
  NoteLayerPropertyChanged();
}

int PaintedScrollbarLayerImpl::ThumbThickness() const {
  return thumb_thickness_;
}

void PaintedScrollbarLayerImpl::SetThumbLength(int thumb_length) {
  if (thumb_length_ == thumb_length)
    return;
  thumb_length_ = thumb_length;
  NoteLayerPropertyChanged();
}

int PaintedScrollbarLayerImpl::ThumbLength() const {
  return thumb_length_;
}

void PaintedScrollbarLayerImpl::SetTrackStart(int track_start) {
  if (track_start_ == track_start)
    return;
  track_start_ = track_start;
  NoteLayerPropertyChanged();
}

int PaintedScrollbarLayerImpl::TrackStart() const {
  return track_start_;
}

void PaintedScrollbarLayerImpl::SetTrackLength(int track_length) {
  if (track_length_ == track_length)
    return;
  track_length_ = track_length;
  NoteLayerPropertyChanged();
}

float PaintedScrollbarLayerImpl::TrackLength() const {
  return track_length_ + (orientation() == VERTICAL ? vertical_adjust() : 0);
}

bool PaintedScrollbarLayerImpl::IsThumbResizable() const {
  return false;
}

const char* PaintedScrollbarLayerImpl::LayerTypeAsString() const {
  return "cc::PaintedScrollbarLayerImpl";
}

LayerTreeSettings::ScrollbarAnimator
PaintedScrollbarLayerImpl::GetScrollbarAnimator() const {
  return LayerTreeSettings::NO_ANIMATOR;
}

}  // namespace cc
