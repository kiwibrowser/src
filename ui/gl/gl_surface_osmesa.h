// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_OSMESA_H_
#define UI_GL_GL_SURFACE_OSMESA_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"

namespace gl {

// A surface that the Mesa software renderer draws to. This is actually just a
// buffer in system memory. GetHandle returns a pointer to the buffer. These
// surfaces can be resized and resizing preserves the contents.
class GL_EXPORT GLSurfaceOSMesa : public GLSurface {
 public:
  GLSurfaceOSMesa(GLSurfaceFormat format, const gfx::Size& size);

  // Implement GLSurface.
  bool Initialize(GLSurfaceFormat format) override;
  void Destroy() override;
  bool Resize(const gfx::Size& new_size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  gfx::Size GetSize() override;
  void* GetHandle() override;
  GLSurfaceFormat GetFormat() override;

 protected:
  ~GLSurfaceOSMesa() override;

 private:
  gfx::Size size_;
  GLSurfaceFormat format_;
  std::unique_ptr<int32_t[]> buffer_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesa);
};

// A thin subclass of |GLSurfaceOSMesa| that can be used in place
// of a native hardware-provided surface when a native surface
// provider is not available.
class GL_EXPORT GLSurfaceOSMesaHeadless : public GLSurfaceOSMesa {
 public:
  GLSurfaceOSMesaHeadless();

  // GLSurfaceOSMesa overrides:
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  bool SupportsPresentationCallback() override;

 protected:
  ~GLSurfaceOSMesaHeadless() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceOSMesaHeadless);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_OSMESA_H_
