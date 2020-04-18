// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/dc_layer_overlay.h"

#include "base/metrics/histogram_macros.h"
#include "cc/base/math_util.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/yuv_video_draw_quad.h"
#include "components/viz/service/display/display_resource_provider.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gl/gl_switches.h"

namespace viz {

namespace {

DCLayerOverlayProcessor::DCLayerResult FromYUVQuad(
    DisplayResourceProvider* resource_provider,
    const YUVVideoDrawQuad* quad,
    DCLayerOverlay* dc_layer_overlay) {
  for (const auto& resource : quad->resources) {
    if (!resource_provider->IsOverlayCandidate(resource))
      return DCLayerOverlayProcessor::DC_LAYER_FAILED_TEXTURE_NOT_CANDIDATE;
  }
  dc_layer_overlay->resources = quad->resources;
  dc_layer_overlay->contents_rect = quad->ya_tex_coord_rect;
  dc_layer_overlay->filter = GL_LINEAR;
  dc_layer_overlay->color_space = quad->video_color_space;
  dc_layer_overlay->require_overlay = quad->require_overlay;
  dc_layer_overlay->is_protected_video = quad->is_protected_video;
  return DCLayerOverlayProcessor::DC_LAYER_SUCCESS;
}

// This returns the smallest rectangle in target space that contains the quad.
gfx::RectF ClippedQuadRectangle(const DrawQuad* quad) {
  gfx::RectF quad_rect = cc::MathUtil::MapClippedRect(
      quad->shared_quad_state->quad_to_target_transform,
      gfx::RectF(quad->rect));
  if (quad->shared_quad_state->is_clipped)
    quad_rect.Intersect(gfx::RectF(quad->shared_quad_state->clip_rect));
  return quad_rect;
}

// Find a rectangle containing all the quads in a list that occlude the area
// in target_quad.
gfx::RectF GetOcclusionBounds(const gfx::RectF& target_quad,
                              QuadList::ConstIterator quad_list_begin,
                              QuadList::ConstIterator quad_list_end) {
  gfx::RectF occlusion_bounding_box;
  for (auto overlap_iter = quad_list_begin; overlap_iter != quad_list_end;
       ++overlap_iter) {
    float opacity = overlap_iter->shared_quad_state->opacity;
    if (opacity < std::numeric_limits<float>::epsilon())
      continue;
    const DrawQuad* quad = *overlap_iter;
    gfx::RectF overlap_rect = ClippedQuadRectangle(quad);
    if (quad->material == DrawQuad::SOLID_COLOR) {
      SkColor color = SolidColorDrawQuad::MaterialCast(quad)->color;
      float alpha = (SkColorGetA(color) * (1.0f / 255.0f)) * opacity;
      if (quad->ShouldDrawWithBlending() &&
          alpha < std::numeric_limits<float>::epsilon())
        continue;
    }
    overlap_rect.Intersect(target_quad);
    if (!overlap_rect.IsEmpty()) {
      occlusion_bounding_box.Union(overlap_rect);
    }
  }
  return occlusion_bounding_box;
}

void RecordDCLayerResult(DCLayerOverlayProcessor::DCLayerResult result) {
  UMA_HISTOGRAM_ENUMERATION("GPU.DirectComposition.DCLayerResult", result,
                            DCLayerOverlayProcessor::DC_LAYER_FAILED_MAX);
}

}  // namespace

DCLayerOverlay::DCLayerOverlay() : filter(GL_LINEAR) {}

DCLayerOverlay::DCLayerOverlay(const DCLayerOverlay& other) = default;

DCLayerOverlay::~DCLayerOverlay() {}

DCLayerOverlayProcessor::DCLayerOverlayProcessor() = default;

DCLayerOverlayProcessor::~DCLayerOverlayProcessor() = default;

DCLayerOverlayProcessor::DCLayerResult DCLayerOverlayProcessor::FromDrawQuad(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    QuadList::ConstIterator quad_list_begin,
    QuadList::ConstIterator quad,
    DCLayerOverlay* dc_layer_overlay) {
  if (quad->shared_quad_state->blend_mode != SkBlendMode::kSrcOver)
    return DC_LAYER_FAILED_QUAD_BLEND_MODE;

  DCLayerResult result;
  switch (quad->material) {
    case DrawQuad::YUV_VIDEO_CONTENT:
      result =
          FromYUVQuad(resource_provider, YUVVideoDrawQuad::MaterialCast(*quad),
                      dc_layer_overlay);
      break;
    default:
      return DC_LAYER_FAILED_UNSUPPORTED_QUAD;
  }
  if (result != DC_LAYER_SUCCESS)
    return result;

  scoped_refptr<DCLayerOverlaySharedState> overlay_shared_state(
      new DCLayerOverlaySharedState);
  overlay_shared_state->z_order = 1;

  overlay_shared_state->is_clipped = quad->shared_quad_state->is_clipped;
  overlay_shared_state->clip_rect =
      gfx::RectF(quad->shared_quad_state->clip_rect);

  overlay_shared_state->opacity = quad->shared_quad_state->opacity;
  overlay_shared_state->transform =
      quad->shared_quad_state->quad_to_target_transform.matrix();

  dc_layer_overlay->shared_state = overlay_shared_state;
  dc_layer_overlay->bounds_rect = gfx::RectF(quad->rect);

  return result;
}

void DCLayerOverlayProcessor::Process(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    RenderPassList* render_passes,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect,
    DCLayerOverlayList* dc_layer_overlays) {
  DCHECK(pass_info_.empty());
  processed_overlay_in_frame_ = false;
  if (base::FeatureList::IsEnabled(
          features::kDirectCompositionNonrootOverlays)) {
    for (auto& pass : *render_passes) {
      bool is_root = (pass == render_passes->back());
      ProcessRenderPass(resource_provider, display_rect, pass.get(), is_root,
                        overlay_damage_rect,
                        is_root ? damage_rect : &pass->damage_rect,
                        dc_layer_overlays);
    }
  } else {
    ProcessRenderPass(resource_provider, display_rect,
                      render_passes->back().get(), true, overlay_damage_rect,
                      damage_rect, dc_layer_overlays);
  }
  pass_info_.clear();
}

QuadList::Iterator DCLayerOverlayProcessor::ProcessRenderPassDrawQuad(
    RenderPass* render_pass,
    gfx::Rect* damage_rect,
    QuadList::Iterator it) {
  DCHECK_EQ(DrawQuad::RENDER_PASS, it->material);
  const RenderPassDrawQuad* rpdq = RenderPassDrawQuad::MaterialCast(*it);

  ++it;
  // Check if this quad is broken to avoid corrupting pass_info.
  if (rpdq->render_pass_id == render_pass->id)
    return it;
  if (!pass_info_.count(rpdq->render_pass_id))
    return it;
  pass_info_[render_pass->id] = std::vector<PunchThroughRect>();
  auto& pass_info = pass_info_[rpdq->render_pass_id];

  const SharedQuadState* original_shared_quad_state = rpdq->shared_quad_state;

  // Punch holes through for all child video quads that will be displayed in
  // underlays. This doesn't work perfectly in all cases - it breaks with
  // complex overlap or filters - but it's needed to be able to display these
  // videos at all. The EME spec allows that some HTML rendering capabilities
  // may be unavailable for EME videos.
  //
  // The solid color quads are inserted after the RPDQ, so they'll be drawn
  // before it and will only cut out contents behind it. A kDstOut solid color
  // quad is used with an accumulated opacity to do the hole punching, because
  // with premultiplied alpha that reduces the opacity of the current content
  // by the opacity of the layer.
  it = render_pass->quad_list
           .InsertBeforeAndInvalidateAllPointers<SolidColorDrawQuad>(
               it, pass_info.size());
  rpdq = nullptr;
  for (size_t i = 0; i < pass_info.size(); i++, ++it) {
    auto& punch_through = pass_info[i];
    SharedQuadState* new_shared_quad_state =
        render_pass->shared_quad_state_list
            .AllocateAndConstruct<SharedQuadState>();
    gfx::Transform new_transform(
        original_shared_quad_state->quad_to_target_transform,
        punch_through.transform_to_target);
    float new_opacity =
        punch_through.opacity * original_shared_quad_state->opacity;
    new_shared_quad_state->SetAll(new_transform, punch_through.rect,
                                  punch_through.rect, punch_through.rect, false,
                                  true, new_opacity, SkBlendMode::kDstOut, 0);
    auto* solid_quad = static_cast<SolidColorDrawQuad*>(*it);
    solid_quad->SetAll(new_shared_quad_state, punch_through.rect,
                       punch_through.rect, false, 0xff000000, true);
    damage_rect->Union(gfx::ToEnclosingRect(ClippedQuadRectangle(solid_quad)));

    // Add transformed info to list in case this renderpass is included in
    // another pass.
    PunchThroughRect info;
    info.rect = punch_through.rect;
    info.transform_to_target = new_transform;
    info.opacity = new_opacity;
    pass_info_[render_pass->id].push_back(info);
  }
  return it;
}

void DCLayerOverlayProcessor::ProcessRenderPass(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    RenderPass* render_pass,
    bool is_root,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect,
    DCLayerOverlayList* dc_layer_overlays) {
  gfx::Rect this_frame_underlay_rect;
  QuadList* quad_list = &render_pass->quad_list;

  auto next_it = quad_list->begin();
  for (auto it = quad_list->begin(); it != quad_list->end(); it = next_it) {
    next_it = it;
    ++next_it;
    // next_it may be modified inside the loop if methods modify the quad list
    // and invalidate iterators to it.

    if (it->material == DrawQuad::RENDER_PASS) {
      next_it = ProcessRenderPassDrawQuad(render_pass, damage_rect, it);
      continue;
    }

    DCLayerOverlay dc_layer;
    DCLayerResult result = FromDrawQuad(resource_provider, display_rect,
                                        quad_list->begin(), it, &dc_layer);
    if (result != DC_LAYER_SUCCESS) {
      RecordDCLayerResult(result);
      continue;
    }

    if (!it->shared_quad_state->quad_to_target_transform
             .Preserves2dAxisAlignment() &&
        !dc_layer.require_overlay &&
        !base::FeatureList::IsEnabled(
            features::kDirectCompositionComplexOverlays)) {
      RecordDCLayerResult(DC_LAYER_FAILED_COMPLEX_TRANSFORM);
      continue;
    }

    dc_layer.shared_state->transform.postConcat(
        render_pass->transform_to_root_target.matrix());

    gfx::Rect quad_rectangle = gfx::ToEnclosingRect(ClippedQuadRectangle(*it));
    gfx::RectF occlusion_bounding_box =
        GetOcclusionBounds(gfx::RectF(quad_rectangle), quad_list->begin(), it);
    bool processed_overlay = false;

    // Underlays are less efficient, so attempt regular overlays first.
    if (is_root && !processed_overlay_in_frame_ &&
        ProcessForOverlay(display_rect, quad_list, quad_rectangle,
                          occlusion_bounding_box, &it, damage_rect)) {
      // ProcessForOverlay makes the iterator point to the next value on
      // success.
      next_it = it;
      processed_overlay = true;
    } else if (ProcessForUnderlay(display_rect, render_pass, quad_rectangle,
                                  occlusion_bounding_box, it, is_root,
                                  damage_rect, &this_frame_underlay_rect,
                                  &dc_layer)) {
      processed_overlay = true;
    }

    if (processed_overlay) {
      gfx::Rect rect_in_root = cc::MathUtil::MapEnclosingClippedRect(
          render_pass->transform_to_root_target, quad_rectangle);
      overlay_damage_rect->Union(rect_in_root);

      RecordDCLayerResult(DC_LAYER_SUCCESS);
      dc_layer_overlays->push_back(dc_layer);
      if (!base::FeatureList::IsEnabled(
              features::kDirectCompositionNonrootOverlays)) {
        // Only allow one overlay for now.
        break;
      }
      processed_overlay_in_frame_ = true;
    }
  }
  if (is_root) {
    damage_rect->Intersect(gfx::ToEnclosingRect(display_rect));
    previous_frame_underlay_rect_ = this_frame_underlay_rect;
    previous_display_rect_ = display_rect;
  }
}

bool DCLayerOverlayProcessor::ProcessForOverlay(
    const gfx::RectF& display_rect,
    QuadList* quad_list,
    const gfx::Rect& quad_rectangle,
    const gfx::RectF& occlusion_bounding_box,
    QuadList::Iterator* it,
    gfx::Rect* damage_rect) {
  bool display_rect_changed = (display_rect != previous_display_rect_);
  if (!occlusion_bounding_box.IsEmpty())
    return false;
  // The quad is on top, so promote it to an overlay and remove all damage
  // underneath it.
  if ((*it)
          ->shared_quad_state->quad_to_target_transform
          .Preserves2dAxisAlignment() &&
      !display_rect_changed && !(*it)->ShouldDrawWithBlending()) {
    damage_rect->Subtract(quad_rectangle);
  }
  *it = quad_list->EraseAndInvalidateAllPointers(*it);
  return true;
}

bool DCLayerOverlayProcessor::ProcessForUnderlay(
    const gfx::RectF& display_rect,
    RenderPass* render_pass,
    const gfx::Rect& quad_rectangle,
    const gfx::RectF& occlusion_bounding_box,
    const QuadList::Iterator& it,
    bool is_root,
    gfx::Rect* damage_rect,
    gfx::Rect* this_frame_underlay_rect,
    DCLayerOverlay* dc_layer) {
  if (!dc_layer->require_overlay) {
    if (!base::FeatureList::IsEnabled(features::kDirectCompositionUnderlays)) {
      RecordDCLayerResult(DC_LAYER_FAILED_OCCLUDED);
      return false;
    }
    if (!is_root) {
      RecordDCLayerResult(DC_LAYER_FAILED_NON_ROOT);
      return false;
    }
    if (processed_overlay_in_frame_) {
      RecordDCLayerResult(DC_LAYER_FAILED_TOO_MANY_OVERLAYS);
      return false;
    }
    if ((it->shared_quad_state->opacity < 1.0)) {
      RecordDCLayerResult(DC_LAYER_FAILED_TRANSPARENT);
      return false;
    }
  }
  bool display_rect_changed = (display_rect != previous_display_rect_);
  // The quad is occluded, so replace it with a black solid color quad and
  // place the overlay itself under the quad.
  if (is_root && it->shared_quad_state->quad_to_target_transform
                     .IsIdentityOrIntegerTranslation()) {
    *this_frame_underlay_rect = quad_rectangle;
  }
  dc_layer->shared_state->z_order = -1;
  const SharedQuadState* shared_quad_state = it->shared_quad_state;
  gfx::Rect rect = it->visible_rect;

  if (shared_quad_state->opacity < 1.0) {
    SharedQuadState* new_shared_quad_state =
        render_pass->shared_quad_state_list.AllocateAndCopyFrom(
            shared_quad_state);
    new_shared_quad_state->blend_mode = SkBlendMode::kDstOut;
    auto* replacement =
        render_pass->quad_list.ReplaceExistingElement<SolidColorDrawQuad>(it);
    replacement->SetAll(shared_quad_state, rect, rect, false, 0xff000000, true);
  } else {
    // When the opacity == 1.0, drawing with transparent will be done without
    // blending and will have the proper effect of completely clearing the
    // layer.
    render_pass->quad_list.ReplaceExistingQuadWithOpaqueTransparentSolidColor(
        it);
  }

  if (*this_frame_underlay_rect == previous_frame_underlay_rect_ && is_root &&
      !processed_overlay_in_frame_) {
    // If this underlay rect is the same as for last frame, subtract its
    // area from the damage of the main surface, as the cleared area was
    // already cleared last frame. Add back the damage from the occluded
    // area for this and last frame, as that may have changed.
    if (it->shared_quad_state->quad_to_target_transform
            .Preserves2dAxisAlignment() &&
        !display_rect_changed) {
      gfx::Rect occluding_damage_rect = *damage_rect;
      occluding_damage_rect.Intersect(quad_rectangle);
      damage_rect->Subtract(quad_rectangle);
      gfx::Rect new_occlusion_bounding_box =
          gfx::ToEnclosingRect(occlusion_bounding_box);
      new_occlusion_bounding_box.Union(previous_occlusion_bounding_box_);
      occluding_damage_rect.Intersect(new_occlusion_bounding_box);

      damage_rect->Union(occluding_damage_rect);
    }
  } else {
    // Entire replacement quad must be redrawn.
    damage_rect->Union(quad_rectangle);
  }
  PunchThroughRect info;
  info.rect = gfx::ToEnclosingRect(dc_layer->bounds_rect);
  info.transform_to_target = shared_quad_state->quad_to_target_transform;
  info.opacity = shared_quad_state->opacity;
  pass_info_[render_pass->id].push_back(info);

  previous_occlusion_bounding_box_ =
      gfx::ToEnclosingRect(occlusion_bounding_box);
  return true;
}

}  // namespace viz
