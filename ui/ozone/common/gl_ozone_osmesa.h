// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_COMMON_GL_OZONE_OSMESA_H_
#define UI_OZONE_COMMON_GL_OZONE_OSMESA_H_

#include "base/macros.h"
#include "ui/gl/gl_implementation.h"
#include "ui/ozone/public/gl_ozone.h"

namespace ui {

// GLOzone implementation that uses OSMesa.
class GLOzoneOSMesa : public GLOzone {
 public:
  GLOzoneOSMesa();
  ~GLOzoneOSMesa() override;

  // GLOzone:
  bool InitializeGLOneOffPlatform() override;
  bool InitializeStaticGLBindings(gl::GLImplementation implementation) override;
  void InitializeDebugGLBindings() override;
  void SetDisabledExtensionsPlatform(
      const std::string& disabled_extensions) override;
  bool InitializeExtensionSettingsOneOffPlatform() override;
  void ShutdownGL() override;
  bool GetGLWindowSystemBindingInfo(
      gl::GLWindowSystemBindingInfo* info) override;
  scoped_refptr<gl::GLContext> CreateGLContext(
      gl::GLShareGroup* share_group,
      gl::GLSurface* compatible_surface,
      const gl::GLContextAttribs& attribs) override;
  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override;
  scoped_refptr<gl::GLSurface> CreateSurfacelessViewGLSurface(
      gfx::AcceleratedWidget window) override;
  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLOzoneOSMesa);
};

}  // namespace ui

#endif  // UI_OZONE_COMMON_GL_OZONE_OSMESA_H_
