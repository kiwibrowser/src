// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_PICTURE_DRAW_QUAD_H_
#define COMPONENTS_VIZ_COMMON_QUADS_PICTURE_DRAW_QUAD_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "cc/paint/display_item_list.h"
#include "components/viz/common/quads/content_draw_quad_base.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/viz_common_export.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// Used for on-demand tile rasterization.
class VIZ_COMMON_EXPORT PictureDrawQuad : public ContentDrawQuadBase {
 public:
  PictureDrawQuad();
  PictureDrawQuad(const PictureDrawQuad& other);
  ~PictureDrawQuad() override;

  void SetNew(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              bool needs_blending,
              const gfx::RectF& tex_coord_rect,
              const gfx::Size& texture_size,
              bool nearest_neighbor,
              ResourceFormat texture_format,
              const gfx::Rect& content_rect,
              float contents_scale,
              scoped_refptr<cc::DisplayItemList> display_item_list);

  void SetAll(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              bool needs_blending,
              const gfx::RectF& tex_coord_rect,
              const gfx::Size& texture_size,
              bool nearest_neighbor,
              ResourceFormat texture_format,
              const gfx::Rect& content_rect,
              float contents_scale,
              scoped_refptr<cc::DisplayItemList> display_item_list);

  gfx::Rect content_rect;
  float contents_scale;
  scoped_refptr<cc::DisplayItemList> display_item_list;
  ResourceFormat texture_format;

  static const PictureDrawQuad* MaterialCast(const DrawQuad* quad);

 private:
  void ExtendValue(base::trace_event::TracedValue* value) const override;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_PICTURE_DRAW_QUAD_H_
