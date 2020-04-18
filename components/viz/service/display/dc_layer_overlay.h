// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_DC_LAYER_OVERLAY_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_DC_LAYER_OVERLAY_H_

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/service/viz_service_export.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkMatrix44.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gl/dc_renderer_layer_params.h"

namespace viz {
class DisplayResourceProvider;

class VIZ_SERVICE_EXPORT DCLayerOverlaySharedState
    : public base::RefCounted<DCLayerOverlaySharedState> {
 public:
  DCLayerOverlaySharedState() {}
  int z_order = 0;
  // If |is_clipped| is true, then clip to |clip_rect| in the target space.
  bool is_clipped = false;
  gfx::RectF clip_rect;
  // The opacity property for the CAayer.
  float opacity = 1;
  // The transform to apply to the DCLayer.
  SkMatrix44 transform = SkMatrix44(SkMatrix44::kIdentity_Constructor);

 private:
  friend class base::RefCounted<DCLayerOverlaySharedState>;
  ~DCLayerOverlaySharedState() {}
};

// Holds all information necessary to construct a DCLayer from a DrawQuad.
class VIZ_SERVICE_EXPORT DCLayerOverlay {
 public:
  DCLayerOverlay();
  DCLayerOverlay(const DCLayerOverlay& other);
  ~DCLayerOverlay();

  // State that is frequently shared between consecutive DCLayerOverlays.
  scoped_refptr<DCLayerOverlaySharedState> shared_state;

  // Resource ids that correspond to the DXGI textures to set as the contents
  // of the DCLayer.
  DrawQuad::Resources resources;
  // The contents rect property for the DCLayer.
  gfx::RectF contents_rect;
  // The bounds for the DCLayer in pixels.
  gfx::RectF bounds_rect;
  // The background color property for the DCLayer.
  SkColor background_color = SK_ColorTRANSPARENT;
  // The edge anti-aliasing mask property for the DCLayer.
  unsigned edge_aa_mask = 0;
  // The minification and magnification filters for the DCLayer.
  unsigned filter;
  // If |rpdq| is present, then the renderer must draw the filter effects and
  // copy the result into an IOSurface.
  const RenderPassDrawQuad* rpdq = nullptr;

  // This is the color-space the texture should be displayed as. If invalid,
  // then the default for the texture should be used. For YUV textures, that's
  // normally BT.709.
  gfx::ColorSpace color_space;

  bool require_overlay = false;
  bool is_protected_video = false;
};

typedef std::vector<DCLayerOverlay> DCLayerOverlayList;

class DCLayerOverlayProcessor {
 public:
  // This is used for a histogram to determine why overlays are or aren't
  // used, so don't remove entries and make sure to update enums.xml if
  // it changes.
  enum DCLayerResult {
    DC_LAYER_SUCCESS,
    DC_LAYER_FAILED_UNSUPPORTED_QUAD,
    DC_LAYER_FAILED_QUAD_BLEND_MODE,
    DC_LAYER_FAILED_TEXTURE_NOT_CANDIDATE,
    DC_LAYER_FAILED_OCCLUDED,
    DC_LAYER_FAILED_COMPLEX_TRANSFORM,
    DC_LAYER_FAILED_TRANSPARENT,
    DC_LAYER_FAILED_NON_ROOT,
    DC_LAYER_FAILED_TOO_MANY_OVERLAYS,
    DC_LAYER_FAILED_MAX,
  };

  DCLayerOverlayProcessor();
  ~DCLayerOverlayProcessor();

  void Process(DisplayResourceProvider* resource_provider,
               const gfx::RectF& display_rect,
               RenderPassList* render_passes,
               gfx::Rect* overlay_damage_rect,
               gfx::Rect* damage_rect,
               DCLayerOverlayList* ca_layer_overlays);
  void ClearOverlayState() {
    previous_frame_underlay_rect_ = gfx::Rect();
    previous_occlusion_bounding_box_ = gfx::Rect();
  }

 private:
  DCLayerResult FromDrawQuad(DisplayResourceProvider* resource_provider,
                             const gfx::RectF& display_rect,
                             QuadList::ConstIterator quad_list_begin,
                             QuadList::ConstIterator quad,
                             DCLayerOverlay* ca_layer_overlay);
  // Returns an iterator to the element after |it|.
  QuadList::Iterator ProcessRenderPassDrawQuad(RenderPass* render_pass,
                                               gfx::Rect* damage_rect,
                                               QuadList::Iterator it);
  void ProcessRenderPass(DisplayResourceProvider* resource_provider,
                         const gfx::RectF& display_rect,
                         RenderPass* render_pass,
                         bool is_root,
                         gfx::Rect* overlay_damage_rect,
                         gfx::Rect* damage_rect,
                         DCLayerOverlayList* ca_layer_overlays);
  bool ProcessForOverlay(const gfx::RectF& display_rect,
                         QuadList* quad_list,
                         const gfx::Rect& quad_rectangle,
                         const gfx::RectF& occlusion_bounding_box,
                         QuadList::Iterator* it,
                         gfx::Rect* damage_rect);
  bool ProcessForUnderlay(const gfx::RectF& display_rect,
                          RenderPass* render_pass,
                          const gfx::Rect& quad_rectangle,
                          const gfx::RectF& occlusion_bounding_box,
                          const QuadList::Iterator& it,
                          bool is_root,
                          gfx::Rect* damage_rect,
                          gfx::Rect* this_frame_underlay_rect,
                          DCLayerOverlay* dc_layer);

  gfx::Rect previous_frame_underlay_rect_;
  gfx::Rect previous_occlusion_bounding_box_;
  gfx::RectF previous_display_rect_;
  bool processed_overlay_in_frame_ = false;

  // Store information about punch-through rectangles for non-root
  // RenderPasses. These rectangles are used to clear the corresponding areas
  // in parent renderpasses.
  struct PunchThroughRect {
    gfx::Rect rect;
    gfx::Transform transform_to_target;
    float opacity;
  };

  base::flat_map<RenderPassId, std::vector<PunchThroughRect>> pass_info_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_DC_LAYER_OVERLAY_H_
