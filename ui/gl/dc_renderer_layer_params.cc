// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/dc_renderer_layer_params.h"

#include "ui/gl/gl_image.h"

namespace ui {

DCRendererLayerParams::DCRendererLayerParams(
    bool is_clipped,
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
    bool is_protected_video)
    : is_clipped(is_clipped),
      clip_rect(clip_rect),
      z_order(z_order),
      transform(transform),
      image(image),
      contents_rect(contents_rect),
      rect(rect),
      background_color(background_color),
      edge_aa_mask(edge_aa_mask),
      opacity(opacity),
      filter(filter),
      is_protected_video(is_protected_video) {}

DCRendererLayerParams::DCRendererLayerParams(
    const DCRendererLayerParams& other) = default;
DCRendererLayerParams::~DCRendererLayerParams() = default;

}  // namespace ui
