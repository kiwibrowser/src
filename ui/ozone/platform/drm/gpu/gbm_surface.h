// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_H_
#define UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image.h"
#include "ui/ozone/platform/drm/gpu/gbm_surfaceless.h"

namespace ui {

class DrmWindowProxy;
class GbmSurfaceFactory;

// A GLSurface for GBM Ozone platform provides surface-like semantics
// implemented through surfaceless. A framebuffer is bound automatically.
class GbmSurface : public GbmSurfaceless {
 public:
  GbmSurface(GbmSurfaceFactory* surface_factory,
             std::unique_ptr<DrmWindowProxy> window,
             gfx::AcceleratedWidget widget);

  // gl::GLSurface:
  unsigned int GetBackingFramebufferObject() override;
  bool OnMakeCurrent(gl::GLContext* context) override;
  bool Resize(const gfx::Size& size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;
  bool SupportsPostSubBuffer() override;
  void SwapBuffersAsync(
      const SwapCompletionCallback& completion_callback,
      const PresentationCallback& presentation_callback) override;
  void Destroy() override;
  bool IsSurfaceless() const override;

 private:
  ~GbmSurface() override;

  void BindFramebuffer();
  bool CreatePixmaps();

  scoped_refptr<gl::GLContext> context_;
  GLuint fbo_ = 0;
  GLuint textures_[2];
  scoped_refptr<gl::GLImage> images_[2];
  int current_surface_ = 0;

  DISALLOW_COPY_AND_ASSIGN(GbmSurface);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_H_
