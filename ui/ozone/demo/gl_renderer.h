// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_DEMO_GL_RENDERER_H_
#define UI_OZONE_DEMO_GL_RENDERER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/swap_result.h"
#include "ui/ozone/demo/renderer_base.h"

namespace gfx {
struct PresentationFeedback;
}  // namespace gfx

namespace gl {
class GLContext;
class GLSurface;
}  // namespace gl

namespace ui {

class GlRenderer : public RendererBase {
 public:
  GlRenderer(gfx::AcceleratedWidget widget,
             const scoped_refptr<gl::GLSurface>& surface,
             const gfx::Size& size);
  ~GlRenderer() override;

  // Renderer:
  bool Initialize() override;

 private:
  void RenderFrame();
  void PostRenderFrameTask(gfx::SwapResult result);
  void OnPresentation(const gfx::PresentationFeedback& feedback);

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;

  base::WeakPtrFactory<GlRenderer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GlRenderer);
};

}  // namespace ui

#endif  // UI_OZONE_DEMO_GL_RENDERER_H_
