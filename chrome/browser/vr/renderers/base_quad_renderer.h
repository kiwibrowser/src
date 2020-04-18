// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_RENDERERS_BASE_QUAD_RENDERER_H_
#define CHROME_BROWSER_VR_RENDERERS_BASE_QUAD_RENDERER_H_

#include "base/macros.h"
#include "chrome/browser/vr/renderers/base_renderer.h"

namespace gfx {
class Transform;
}  // namespace gfx

namespace vr {

class BaseQuadRenderer : public BaseRenderer {
 public:
  BaseQuadRenderer(const char* vertex_src, const char* fragment_src);
  ~BaseQuadRenderer() override;

  static void CreateBuffers();
  static int NumQuadIndices();

 protected:
  void PrepareToDraw(GLuint view_proj_matrix_handle,
                     const gfx::Transform& view_proj_matrix);

  static GLuint vertex_buffer_;
  static GLuint index_buffer_;

  DISALLOW_COPY_AND_ASSIGN(BaseQuadRenderer);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_RENDERERS_BASE_QUAD_RENDERER_H_
