// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_STREAM_VIDEO_DRAW_QUAD_H_
#define COMPONENTS_VIZ_COMMON_QUADS_STREAM_VIDEO_DRAW_QUAD_H_

#include <stddef.h>

#include <memory>

#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/viz_common_export.h"
#include "ui/gfx/transform.h"

namespace viz {

class VIZ_COMMON_EXPORT StreamVideoDrawQuad : public DrawQuad {
 public:
  static const size_t kResourceIdIndex = 0;

  StreamVideoDrawQuad();

  void SetNew(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              bool needs_blending,
              unsigned resource_id,
              gfx::Size resource_size_in_pixels,
              const gfx::Transform& matrix);

  void SetAll(const SharedQuadState* shared_quad_state,
              const gfx::Rect& rect,
              const gfx::Rect& visible_rect,
              bool needs_blending,
              unsigned resource_id,
              gfx::Size resource_size_in_pixels,
              const gfx::Transform& matrix);

  gfx::Transform matrix;

  struct OverlayResources {
    OverlayResources();
    gfx::Size size_in_pixels[Resources::kMaxResourceIdCount];
  };
  OverlayResources overlay_resources;

  static const StreamVideoDrawQuad* MaterialCast(const DrawQuad*);

  ResourceId resource_id() const { return resources.ids[kResourceIdIndex]; }
  const gfx::Size& resource_size_in_pixels() const {
    return overlay_resources.size_in_pixels[kResourceIdIndex];
  }

 private:
  void ExtendValue(base::trace_event::TracedValue* value) const override;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_STREAM_VIDEO_DRAW_QUAD_H_
