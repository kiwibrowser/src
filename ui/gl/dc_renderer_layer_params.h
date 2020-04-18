// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_DC_RENDERER_LAYER_PARAMS_H_
#define UI_GL_DC_RENDERER_LAYER_PARAMS_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/transform.h"
#include "ui/gl/gl_export.h"

namespace gl {
class GLImage;
}

namespace ui {

struct GL_EXPORT DCRendererLayerParams {
  DCRendererLayerParams(bool is_clipped,
                        const gfx::Rect clip_rect,
                        int z_order,
                        const gfx::Transform& transform,
                        std::vector<scoped_refptr<gl::GLImage>> image,
                        const gfx::RectF& contents_rect,
                        const gfx::Rect& rect,
                        unsigned background_color,
                        unsigned edge_aa_mask,
                        float opacity,
                        unsigned filter,
                        bool is_protected_video);
  DCRendererLayerParams(const DCRendererLayerParams& other);
  ~DCRendererLayerParams();

  bool is_clipped;
  const gfx::Rect clip_rect;
  int z_order;
  const gfx::Transform transform;
  std::vector<scoped_refptr<gl::GLImage>> image;
  const gfx::RectF contents_rect;
  const gfx::Rect rect;
  unsigned background_color;
  unsigned edge_aa_mask;
  float opacity;
  unsigned filter;
  bool is_protected_video;

  // This is a subset of cc::FilterOperation::FilterType.
  enum class FilterEffectType : uint32_t {
    GRAYSDCLE,
    SEPIA,
    SATURATE,
    HUE_ROTATE,
    INVERT,
    BRIGHTNESS,
    CONTRAST,
    OPACITY,
    BLUR,
    DROP_SHADOW,
  };
  struct GL_EXPORT FilterEffect {
    FilterEffectType type = FilterEffectType::GRAYSDCLE;

    // For every filter other than DROP_SHADOW, only |amount| is populated.
    float amount = 0;
    gfx::Point drop_shadow_offset;
    SkColor drop_shadow_color = 0;
  };
  using FilterEffects = std::vector<FilterEffect>;

  FilterEffects filter_effects;
};

}  // namespace ui

#endif  // UI_GL_DC_RENDERER_LAYER_PARAMS_H_
