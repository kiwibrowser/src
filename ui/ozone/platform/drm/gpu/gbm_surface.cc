// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/gbm_surface.h"

#include <utility>

#include "base/logging.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gl/gl_image_native_pixmap.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/platform/drm/gpu/drm_window_proxy.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"

namespace ui {

GbmSurface::GbmSurface(GbmSurfaceFactory* surface_factory,
                       std::unique_ptr<DrmWindowProxy> window,
                       gfx::AcceleratedWidget widget)
    : GbmSurfaceless(surface_factory, std::move(window), widget) {
  for (auto& texture : textures_)
    texture = 0;
}

unsigned int GbmSurface::GetBackingFramebufferObject() {
  return fbo_;
}

bool GbmSurface::OnMakeCurrent(gl::GLContext* context) {
  DCHECK(!context_ || context == context_);
  context_ = context;
  if (!fbo_) {
    glGenFramebuffersEXT(1, &fbo_);
    if (!fbo_)
      return false;
    glGenTextures(arraysize(textures_), textures_);
    if (!CreatePixmaps())
      return false;
  }
  BindFramebuffer();
  glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_);
  return SurfacelessEGL::OnMakeCurrent(context);
}

bool GbmSurface::Resize(const gfx::Size& size,
                        float scale_factor,
                        ColorSpace color_space,
                        bool has_alpha) {
  if (size == GetSize())
    return true;
  // Alpha value isn't actually used in allocating buffers yet, so always use
  // true instead.
  return GbmSurfaceless::Resize(size, scale_factor, color_space, true) &&
         CreatePixmaps();
}

bool GbmSurface::SupportsPostSubBuffer() {
  return false;
}

void GbmSurface::SwapBuffersAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  if (!images_[current_surface_]->ScheduleOverlayPlane(
          widget(), 0, gfx::OverlayTransform::OVERLAY_TRANSFORM_NONE,
          gfx::Rect(GetSize()), gfx::RectF(1, 1), /* enable_blend */ false,
          /* gpu_fence */ nullptr)) {
    completion_callback.Run(gfx::SwapResult::SWAP_FAILED);
    // Notify the caller, the buffer is never presented on a screen.
    presentation_callback.Run(gfx::PresentationFeedback());
    return;
  }
  GbmSurfaceless::SwapBuffersAsync(completion_callback, presentation_callback);
  current_surface_ ^= 1;
  BindFramebuffer();
}

void GbmSurface::Destroy() {
  if (!context_)
    return;
  scoped_refptr<gl::GLContext> previous_context = gl::GLContext::GetCurrent();
  scoped_refptr<GLSurface> previous_surface;

  bool was_current = previous_context && previous_context->IsCurrent(nullptr) &&
                     GLSurface::GetCurrent() == this;
  if (!was_current) {
    // Only take a reference to previous surface if it's not |this|
    // because otherwise we can take a self reference from our own dtor.
    previous_surface = GLSurface::GetCurrent();
    context_->MakeCurrent(this);
  }

  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
  if (fbo_) {
    glDeleteTextures(arraysize(textures_), textures_);
    for (auto& texture : textures_)
      texture = 0;
    glDeleteFramebuffersEXT(1, &fbo_);
    fbo_ = 0;
  }
  for (auto& image : images_)
    image = nullptr;

  if (!was_current) {
    if (previous_context) {
      previous_context->MakeCurrent(previous_surface.get());
    } else {
      context_->ReleaseCurrent(this);
    }
  }
}

bool GbmSurface::IsSurfaceless() const {
  return false;
}

GbmSurface::~GbmSurface() {
  Destroy();
}

void GbmSurface::BindFramebuffer() {
  gl::ScopedFramebufferBinder fb(fbo_);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            textures_[current_surface_], 0);
}

bool GbmSurface::CreatePixmaps() {
  if (!fbo_)
    return true;
  for (size_t i = 0; i < arraysize(textures_); i++) {
    scoped_refptr<gfx::NativePixmap> pixmap =
        surface_factory()->CreateNativePixmap(
            widget(), GetSize(), display::DisplaySnapshot::PrimaryFormat(),
            gfx::BufferUsage::SCANOUT);
    if (!pixmap)
      return false;
    scoped_refptr<gl::GLImageNativePixmap> image =
        new gl::GLImageNativePixmap(GetSize(), GL_BGRA_EXT);
    if (!image->Initialize(pixmap.get(),
                           display::DisplaySnapshot::PrimaryFormat()))
      return false;
    images_[i] = image;
    // Bind image to texture.
    gl::ScopedTextureBinder binder(GL_TEXTURE_2D, textures_[i]);
    if (!images_[i]->BindTexImage(GL_TEXTURE_2D))
      return false;
  }
  return true;
}

}  // namespace ui
