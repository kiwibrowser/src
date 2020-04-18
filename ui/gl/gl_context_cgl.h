// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_CONTEXT_CGL_H_
#define UI_GL_GL_CONTEXT_CGL_H_

#include <OpenGL/CGLTypes.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "ui/gfx/color_space.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_export.h"

namespace gl {

class GLSurface;

// Encapsulates a CGL OpenGL context.
class GL_EXPORT GLContextCGL : public GLContextReal {
 public:
  explicit GLContextCGL(GLShareGroup* share_group);

  // Implement GLContext.
  bool Initialize(GLSurface* compatible_surface,
                  const GLContextAttribs& attribs) override;
  bool MakeCurrent(GLSurface* surface) override;
  void ReleaseCurrent(GLSurface* surface) override;
  bool IsCurrent(GLSurface* surface) override;
  void* GetHandle() override;
  void SetSafeToForceGpuSwitch() override;
  bool ForceGpuSwitchIfNeeded() override;
  YUVToRGBConverter* GetYUVToRGBConverter(
      const gfx::ColorSpace& color_space) override;

 protected:
  ~GLContextCGL() override;

 private:
  void Destroy();
  GpuPreference GetGpuPreference();

  void* context_;
  GpuPreference gpu_preference_;
  std::map<gfx::ColorSpace, std::unique_ptr<YUVToRGBConverter>>
      yuv_to_rgb_converters_;

  CGLPixelFormatObj discrete_pixelformat_;

  int screen_;
  int renderer_id_;
  bool safe_to_force_gpu_switch_;

  DISALLOW_COPY_AND_ASSIGN(GLContextCGL);
};

}  // namespace gl

#endif  // UI_GL_GL_CONTEXT_CGL_H_
