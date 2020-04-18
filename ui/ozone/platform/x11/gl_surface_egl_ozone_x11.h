// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_X11_GL_SURFACE_EGL_OZONE_X11_H_
#define UI_OZONE_PLATFORM_X11_GL_SURFACE_EGL_OZONE_X11_H_

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_surface_egl.h"

namespace ui {

// GLSurface implementation for Ozone X11 EGL. This does not create a new
// XWindow or observe XExpose events like GLSurfaceEGLX11 does.
class GLSurfaceEGLOzoneX11 : public gl::NativeViewGLSurfaceEGL {
 public:
  explicit GLSurfaceEGLOzoneX11(EGLNativeWindowType window);

  // gl::NativeViewGLSurfaceEGL:
  EGLConfig GetConfig() override;
  bool Resize(const gfx::Size& size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;

 private:
  ~GLSurfaceEGLOzoneX11() override;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceEGLOzoneX11);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_X11_GL_SURFACE_EGL_OZONE_X11_H_
