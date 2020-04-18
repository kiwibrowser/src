// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_QUADS_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_QUADS_STRUCT_TRAITS_H_

#include "base/containers/span.h"
#include "base/logging.h"
#include "components/viz/common/quads/debug_border_draw_quad.h"
#include "components/viz/common/quads/picture_draw_quad.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/stream_video_draw_quad.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/common/quads/tile_draw_quad.h"
#include "components/viz/common/quads/yuv_video_draw_quad.h"
#include "services/viz/public/cpp/compositing/filter_operation_struct_traits.h"
#include "services/viz/public/cpp/compositing/filter_operations_struct_traits.h"
#include "services/viz/public/cpp/compositing/shared_quad_state_struct_traits.h"
#include "services/viz/public/cpp/compositing/surface_id_struct_traits.h"
#include "services/viz/public/interfaces/compositing/quads.mojom-shared.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"

namespace mojo {

viz::DrawQuad* AllocateAndConstruct(
    viz::mojom::DrawQuadStateDataView::Tag material,
    viz::QuadList* list);

template <>
struct UnionTraits<viz::mojom::DrawQuadStateDataView, viz::DrawQuad> {
  static viz::mojom::DrawQuadStateDataView::Tag GetTag(
      const viz::DrawQuad& quad) {
    switch (quad.material) {
      case viz::DrawQuad::INVALID:
        break;
      case viz::DrawQuad::DEBUG_BORDER:
        return viz::mojom::DrawQuadStateDataView::Tag::DEBUG_BORDER_QUAD_STATE;
      case viz::DrawQuad::PICTURE_CONTENT:
        break;
      case viz::DrawQuad::RENDER_PASS:
        return viz::mojom::DrawQuadStateDataView::Tag::RENDER_PASS_QUAD_STATE;
      case viz::DrawQuad::SOLID_COLOR:
        return viz::mojom::DrawQuadStateDataView::Tag::SOLID_COLOR_QUAD_STATE;
      case viz::DrawQuad::STREAM_VIDEO_CONTENT:
        return viz::mojom::DrawQuadStateDataView::Tag::STREAM_VIDEO_QUAD_STATE;
      case viz::DrawQuad::SURFACE_CONTENT:
        return viz::mojom::DrawQuadStateDataView::Tag::SURFACE_QUAD_STATE;
      case viz::DrawQuad::TEXTURE_CONTENT:
        return viz::mojom::DrawQuadStateDataView::Tag::TEXTURE_QUAD_STATE;
      case viz::DrawQuad::TILED_CONTENT:
        return viz::mojom::DrawQuadStateDataView::Tag::TILE_QUAD_STATE;
      case viz::DrawQuad::YUV_VIDEO_CONTENT:
        return viz::mojom::DrawQuadStateDataView::Tag::YUV_VIDEO_QUAD_STATE;
    }
    NOTREACHED();
    return viz::mojom::DrawQuadStateDataView::Tag::DEBUG_BORDER_QUAD_STATE;
  }

  static const viz::DrawQuad& debug_border_quad_state(
      const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& render_pass_quad_state(
      const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& solid_color_quad_state(
      const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& surface_quad_state(const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& texture_quad_state(const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& tile_quad_state(const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& stream_video_quad_state(
      const viz::DrawQuad& quad) {
    return quad;
  }

  static const viz::DrawQuad& yuv_video_quad_state(const viz::DrawQuad& quad) {
    return quad;
  }

  static bool Read(viz::mojom::DrawQuadStateDataView data, viz::DrawQuad* out) {
    switch (data.tag()) {
      case viz::mojom::DrawQuadStateDataView::Tag::DEBUG_BORDER_QUAD_STATE:
        return data.ReadDebugBorderQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::RENDER_PASS_QUAD_STATE:
        return data.ReadRenderPassQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::SOLID_COLOR_QUAD_STATE:
        return data.ReadSolidColorQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::SURFACE_QUAD_STATE:
        return data.ReadSurfaceQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::TEXTURE_QUAD_STATE:
        return data.ReadTextureQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::TILE_QUAD_STATE:
        return data.ReadTileQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::STREAM_VIDEO_QUAD_STATE:
        return data.ReadStreamVideoQuadState(out);
      case viz::mojom::DrawQuadStateDataView::Tag::YUV_VIDEO_QUAD_STATE:
        return data.ReadYuvVideoQuadState(out);
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<viz::mojom::DebugBorderQuadStateDataView, viz::DrawQuad> {
  static uint32_t color(const viz::DrawQuad& input) {
    const viz::DebugBorderDrawQuad* quad =
        viz::DebugBorderDrawQuad::MaterialCast(&input);
    return quad->color;
  }

  static int32_t width(const viz::DrawQuad& input) {
    const viz::DebugBorderDrawQuad* quad =
        viz::DebugBorderDrawQuad::MaterialCast(&input);
    return quad->width;
  }

  static bool Read(viz::mojom::DebugBorderQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::RenderPassQuadStateDataView, viz::DrawQuad> {
  static int32_t render_pass_id(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    DCHECK(quad->render_pass_id);
    return quad->render_pass_id;
  }

  static uint32_t mask_resource_id(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->mask_resource_id();
  }

  static const gfx::RectF& mask_uv_rect(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->mask_uv_rect;
  }

  static const gfx::Size& mask_texture_size(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->mask_texture_size;
  }

  static const gfx::Vector2dF& filters_scale(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->filters_scale;
  }

  static const gfx::PointF& filters_origin(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->filters_origin;
  }

  static const gfx::RectF& tex_coord_rect(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->tex_coord_rect;
  }

  static bool force_anti_aliasing_off(const viz::DrawQuad& input) {
    const viz::RenderPassDrawQuad* quad =
        viz::RenderPassDrawQuad::MaterialCast(&input);
    return quad->force_anti_aliasing_off;
  }

  static bool Read(viz::mojom::RenderPassQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::SolidColorQuadStateDataView, viz::DrawQuad> {
  static uint32_t color(const viz::DrawQuad& input) {
    const viz::SolidColorDrawQuad* quad =
        viz::SolidColorDrawQuad::MaterialCast(&input);
    return quad->color;
  }

  static bool force_anti_aliasing_off(const viz::DrawQuad& input) {
    const viz::SolidColorDrawQuad* quad =
        viz::SolidColorDrawQuad::MaterialCast(&input);
    return quad->force_anti_aliasing_off;
  }

  static bool Read(viz::mojom::SolidColorQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::StreamVideoQuadStateDataView, viz::DrawQuad> {
  static uint32_t resource_id(const viz::DrawQuad& input) {
    const viz::StreamVideoDrawQuad* quad =
        viz::StreamVideoDrawQuad::MaterialCast(&input);
    return quad->resources.ids[viz::StreamVideoDrawQuad::kResourceIdIndex];
  }

  static const gfx::Size& resource_size_in_pixels(const viz::DrawQuad& input) {
    const viz::StreamVideoDrawQuad* quad =
        viz::StreamVideoDrawQuad::MaterialCast(&input);
    return quad->overlay_resources
        .size_in_pixels[viz::StreamVideoDrawQuad::kResourceIdIndex];
  }

  static const gfx::Transform& matrix(const viz::DrawQuad& input) {
    const viz::StreamVideoDrawQuad* quad =
        viz::StreamVideoDrawQuad::MaterialCast(&input);
    return quad->matrix;
  }

  static bool Read(viz::mojom::StreamVideoQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::SurfaceQuadStateDataView, viz::DrawQuad> {
  static const viz::SurfaceId& primary_surface_id(const viz::DrawQuad& input) {
    const viz::SurfaceDrawQuad* quad =
        viz::SurfaceDrawQuad::MaterialCast(&input);
    return quad->primary_surface_id;
  }

  static const base::Optional<viz::SurfaceId>& fallback_surface_id(
      const viz::DrawQuad& input) {
    const viz::SurfaceDrawQuad* quad =
        viz::SurfaceDrawQuad::MaterialCast(&input);
    return quad->fallback_surface_id;
  }

  static uint32_t default_background_color(const viz::DrawQuad& input) {
    const viz::SurfaceDrawQuad* quad =
        viz::SurfaceDrawQuad::MaterialCast(&input);
    return quad->default_background_color;
  }

  static bool stretch_content_to_fill_bounds(const viz::DrawQuad& input) {
    const viz::SurfaceDrawQuad* quad =
        viz::SurfaceDrawQuad::MaterialCast(&input);
    return quad->stretch_content_to_fill_bounds;
  }

  static bool Read(viz::mojom::SurfaceQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::TextureQuadStateDataView, viz::DrawQuad> {
  static uint32_t resource_id(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->resource_id();
  }

  static const gfx::Size& resource_size_in_pixels(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->resource_size_in_pixels();
  }

  static bool premultiplied_alpha(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->premultiplied_alpha;
  }

  static const gfx::PointF& uv_top_left(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->uv_top_left;
  }

  static const gfx::PointF& uv_bottom_right(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->uv_bottom_right;
  }

  static uint32_t background_color(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->background_color;
  }

  static base::span<const float> vertex_opacity(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->vertex_opacity;
  }

  static bool y_flipped(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->y_flipped;
  }

  static bool nearest_neighbor(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->nearest_neighbor;
  }

  static bool secure_output_only(const viz::DrawQuad& input) {
    const viz::TextureDrawQuad* quad =
        viz::TextureDrawQuad::MaterialCast(&input);
    return quad->secure_output_only;
  }

  static bool Read(viz::mojom::TextureQuadStateDataView data,
                   viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::TileQuadStateDataView, viz::DrawQuad> {
  static const gfx::RectF& tex_coord_rect(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->tex_coord_rect;
  }

  static const gfx::Size& texture_size(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->texture_size;
  }

  static bool swizzle_contents(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->swizzle_contents;
  }

  static bool is_premultiplied(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->is_premultiplied;
  }

  static bool nearest_neighbor(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->nearest_neighbor;
  }

  static uint32_t resource_id(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->resource_id();
  }

  static bool force_anti_aliasing_off(const viz::DrawQuad& input) {
    const viz::TileDrawQuad* quad = viz::TileDrawQuad::MaterialCast(&input);
    return quad->force_anti_aliasing_off;
  }

  static bool Read(viz::mojom::TileQuadStateDataView data, viz::DrawQuad* out);
};

template <>
struct StructTraits<viz::mojom::YUVVideoQuadStateDataView, viz::DrawQuad> {
  static const gfx::RectF& ya_tex_coord_rect(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->ya_tex_coord_rect;
  }

  static const gfx::RectF& uv_tex_coord_rect(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->uv_tex_coord_rect;
  }

  static const gfx::Size& ya_tex_size(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->ya_tex_size;
  }

  static const gfx::Size& uv_tex_size(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->uv_tex_size;
  }

  static uint32_t y_plane_resource_id(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->y_plane_resource_id();
  }

  static uint32_t u_plane_resource_id(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->u_plane_resource_id();
  }

  static uint32_t v_plane_resource_id(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->v_plane_resource_id();
  }

  static uint32_t a_plane_resource_id(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->a_plane_resource_id();
  }

  static float resource_offset(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->resource_offset;
  }

  static float resource_multiplier(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->resource_multiplier;
  }

  static uint32_t bits_per_channel(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->bits_per_channel;
  }
  static gfx::ColorSpace video_color_space(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->video_color_space;
  }
  static bool require_overlay(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->require_overlay;
  }
  static bool is_protected_video(const viz::DrawQuad& input) {
    const viz::YUVVideoDrawQuad* quad =
        viz::YUVVideoDrawQuad::MaterialCast(&input);
    return quad->is_protected_video;
  }

  static bool Read(viz::mojom::YUVVideoQuadStateDataView data,
                   viz::DrawQuad* out);
};

struct DrawQuadWithSharedQuadState {
  const viz::DrawQuad* quad;
  const viz::SharedQuadState* shared_quad_state;
};

template <>
struct StructTraits<viz::mojom::DrawQuadDataView, DrawQuadWithSharedQuadState> {
  static const gfx::Rect& rect(const DrawQuadWithSharedQuadState& input) {
    return input.quad->rect;
  }

  static const gfx::Rect& visible_rect(
      const DrawQuadWithSharedQuadState& input) {
    return input.quad->visible_rect;
  }

  static bool needs_blending(const DrawQuadWithSharedQuadState& input) {
    return input.quad->needs_blending;
  }

  static OptSharedQuadState sqs(const DrawQuadWithSharedQuadState& input) {
    return {input.shared_quad_state};
  }

  static const viz::DrawQuad& draw_quad_state(
      const DrawQuadWithSharedQuadState& input) {
    return *input.quad;
  }
};

// This StructTraits is only used for deserialization within RenderPasses.
template <>
struct StructTraits<viz::mojom::DrawQuadDataView, viz::DrawQuad> {
  static bool Read(viz::mojom::DrawQuadDataView data, viz::DrawQuad* out);
};

template <>
struct ArrayTraits<viz::QuadList> {
  using Element = DrawQuadWithSharedQuadState;
  struct ConstIterator {
    explicit ConstIterator(const viz::QuadList::ConstIterator& it)
        : it(it), last_shared_quad_state(nullptr) {}

    viz::QuadList::ConstIterator it;
    const viz::SharedQuadState* last_shared_quad_state;
  };

  static ConstIterator GetBegin(const viz::QuadList& input) {
    return ConstIterator(input.begin());
  }

  static void AdvanceIterator(ConstIterator& iterator) {  // NOLINT
    iterator.last_shared_quad_state = (*iterator.it)->shared_quad_state;
    ++iterator.it;
  }

  static Element GetValue(ConstIterator& iterator) {  // NOLINT
    DrawQuadWithSharedQuadState dq = {*iterator.it, nullptr};
    // Only serialize the SharedQuadState if we haven't seen it before and
    // therefore have not already serialized it.
    const viz::SharedQuadState* current_sqs = (*iterator.it)->shared_quad_state;
    if (current_sqs != iterator.last_shared_quad_state)
      dq.shared_quad_state = current_sqs;
    return dq;
  }

  static size_t GetSize(const viz::QuadList& input) { return input.size(); }
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_QUADS_STRUCT_TRAITS_H_
