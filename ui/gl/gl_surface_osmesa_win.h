// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_OSMESA_WIN_H_
#define UI_GL_GL_SURFACE_OSMESA_WIN_H_

#include <dwmapi.h>

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_surface_osmesa.h"

namespace gl {

// This OSMesa GL surface can use GDI to swap the contents of the buffer to a
// view.
class GL_EXPORT GLSurfaceOSMesaWin : public GLSurfaceOSMesa {
 public:
  explicit GLSurfaceOSMesaWin(gfx::AcceleratedWidget window);

  // Implement subset of GLSurface.
  bool Initialize(GLSurfaceFormat format) override;
  void Destroy() override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  bool SupportsPresentationCallback() override;
  bool SupportsPostSubBuffer() override;
  gfx::SwapResult PostSubBuffer(int x,
                                int y,
                                int width,
                                int height,
                                const PresentationCallback& callback) override;

 private:
  ~GLSurfaceOSMesaWin() override;

  gfx::AcceleratedWidget window_;
  HDC device_context_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesaWin);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_OSMESA_WIN_H_
