// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_OSMESA_X11_H_
#define UI_GL_GL_SURFACE_OSMESA_X11_H_

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/x11.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_surface_osmesa.h"

namespace gl {

// This OSMesa GL surface can use XLib to swap the contents of the buffer to a
// view.
class GL_EXPORT GLSurfaceOSMesaX11 : public GLSurfaceOSMesa {
 public:
  explicit GLSurfaceOSMesaX11(gfx::AcceleratedWidget window);

  static bool InitializeOneOff();

  // Implement a subset of GLSurface.
  bool Initialize(GLSurfaceFormat format) override;
  void Destroy() override;
  bool Resize(const gfx::Size& new_size,
              float scale_factor,
              ColorSpace color_space,
              bool alpha) override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  bool SupportsPostSubBuffer() override;
  gfx::SwapResult PostSubBuffer(int x,
                                int y,
                                int width,
                                int height,
                                const PresentationCallback& callback) override;
  bool SupportsPresentationCallback() override;

 protected:
  ~GLSurfaceOSMesaX11() override;

 private:
  Display* xdisplay_;
  GC window_graphics_context_;
  gfx::AcceleratedWidget window_;
  GC pixmap_graphics_context_;
  Pixmap pixmap_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesaX11);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_OSMESA_X11_H_
