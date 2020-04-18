// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/static_geometry_binding.h"

#include <stddef.h>
#include <stdint.h>

#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/gfx/geometry/rect_f.h"

namespace viz {

StaticGeometryBinding::StaticGeometryBinding(gpu::gles2::GLES2Interface* gl,
                                             const gfx::RectF& quad_vertex_rect)
    : gl_(gl), quad_vertices_vbo_(0), quad_elements_vbo_(0) {
  GeometryBindingQuad quads[NUM_QUADS];
  GeometryBindingQuadIndex quad_indices[NUM_QUADS];

  static_assert(sizeof(GeometryBindingQuad) == 24 * sizeof(float),
                "struct Quad should be densely packed");
  static_assert(sizeof(GeometryBindingQuadIndex) == 6 * sizeof(uint16_t),
                "struct QuadIndex should be densely packed");

  for (size_t i = 0; i < NUM_QUADS; i++) {
    GeometryBindingVertex v0 = {
        {quad_vertex_rect.x(), quad_vertex_rect.bottom(), 0.0f},
        {0.0f, 1.0f},
        i * 4.0f + 0.0f};
    GeometryBindingVertex v1 = {
        {quad_vertex_rect.x(), quad_vertex_rect.y(), 0.0f},
        {0.0f, 0.0f},
        i * 4.0f + 1.0f};
    GeometryBindingVertex v2 = {
        {quad_vertex_rect.right(), quad_vertex_rect.y(), 0.0f},
        {1.0f, 0.0f},
        i * 4.0f + 2.0f};
    GeometryBindingVertex v3 = {
        {quad_vertex_rect.right(), quad_vertex_rect.bottom(), 0.0f},
        {1.0f, 1.0f},
        i * 4.0f + 3.0f};
    GeometryBindingQuad x(v0, v1, v2, v3);
    quads[i] = x;
    GeometryBindingQuadIndex y(
        static_cast<uint16_t>(0 + 4 * i), static_cast<uint16_t>(1 + 4 * i),
        static_cast<uint16_t>(2 + 4 * i), static_cast<uint16_t>(3 + 4 * i),
        static_cast<uint16_t>(0 + 4 * i), static_cast<uint16_t>(2 + 4 * i));
    quad_indices[i] = y;
  }

  gl_->GenBuffers(1, &quad_vertices_vbo_);
  gl_->GenBuffers(1, &quad_elements_vbo_);

  gl_->BindBuffer(GL_ARRAY_BUFFER, quad_vertices_vbo_);
  gl_->BufferData(GL_ARRAY_BUFFER, sizeof(GeometryBindingQuad) * NUM_QUADS,
                  quads, GL_STATIC_DRAW);

  gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_elements_vbo_);
  gl_->BufferData(GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GeometryBindingQuadIndex) * NUM_QUADS, &quad_indices,
                  GL_STATIC_DRAW);
}

StaticGeometryBinding::~StaticGeometryBinding() {
  gl_->DeleteBuffers(1, &quad_vertices_vbo_);
  gl_->DeleteBuffers(1, &quad_elements_vbo_);
}

void StaticGeometryBinding::PrepareForDraw() {
  SetupGLContext(gl_, quad_elements_vbo_, quad_vertices_vbo_);
}

}  // namespace viz
