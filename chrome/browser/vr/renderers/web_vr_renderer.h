// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_RENDERERS_WEB_VR_RENDERER_H_
#define CHROME_BROWSER_VR_RENDERERS_WEB_VR_RENDERER_H_

#include "base/macros.h"
#include "chrome/browser/vr/renderers/base_quad_renderer.h"

namespace vr {

// Renders a page-generated stereo VR view.
class WebVrRenderer : public BaseQuadRenderer {
 public:
  WebVrRenderer();
  ~WebVrRenderer() override;

  void Draw(int texture_handle,
            const float (&uv_transform)[16],
            float xborder,
            float yborder);

 private:
  GLuint texture_handle_;
  GLuint uv_transform_;
  GLuint x_border_handle_;
  GLuint y_border_handle_;

  DISALLOW_COPY_AND_ASSIGN(WebVrRenderer);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_RENDERERS_WEB_VR_RENDERER_H_
