// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_SURFACE_DRAW_QUAD_H_
#define COMPONENTS_VIZ_COMMON_QUADS_SURFACE_DRAW_QUAD_H_

#include <memory>

#include "base/optional.h"
#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/skia/include/core/SkColor.h"

namespace viz {

class VIZ_COMMON_EXPORT SurfaceDrawQuad : public DrawQuad {
 public:
  SurfaceDrawQuad();
  SurfaceDrawQuad(const SurfaceDrawQuad& other);
  ~SurfaceDrawQuad() override;

  SurfaceDrawQuad& operator=(const SurfaceDrawQuad& other);

  void SetNew(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              const SurfaceId& primary_surface_id,
              const base::Optional<SurfaceId>& fallback_surface_id,
              SkColor default_background_color,
              bool stretch_content_to_fill_bounds);

  void SetAll(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              bool needs_blending,
              const SurfaceId& primary_surface_id,
              const base::Optional<SurfaceId>& fallback_surface_id,
              SkColor default_background_color,
              bool stretch_content_to_fill_bounds);

  SurfaceId primary_surface_id;
  base::Optional<SurfaceId> fallback_surface_id;
  SkColor default_background_color = SK_ColorWHITE;
  bool stretch_content_to_fill_bounds = false;

  static const SurfaceDrawQuad* MaterialCast(const DrawQuad* quad);

 private:
  void ExtendValue(base::trace_event::TracedValue* value) const override;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_SURFACE_DRAW_QUAD_H_
