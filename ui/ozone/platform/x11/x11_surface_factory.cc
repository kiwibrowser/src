// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/x11_surface_factory.h"

#include "gpu/vulkan/buildflags.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/common/gl_ozone_egl.h"
#include "ui/ozone/common/gl_ozone_osmesa.h"
#include "ui/ozone/platform/x11/gl_ozone_glx.h"
#include "ui/ozone/platform/x11/gl_surface_egl_ozone_x11.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "gpu/vulkan/x/vulkan_implementation_x11.h"
#endif

namespace ui {
namespace {

class GLOzoneEGLX11 : public GLOzoneEGL {
 public:
  GLOzoneEGLX11() = default;
  ~GLOzoneEGLX11() override = default;

  // GLOzone:
  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override {
    return gl::InitializeGLSurface(new GLSurfaceEGLOzoneX11(window));
  }

  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override {
    return gl::InitializeGLSurface(new gl::PbufferGLSurfaceEGL(size));
  }

 protected:
  // GLOzoneEGL:
  intptr_t GetNativeDisplay() override {
    return reinterpret_cast<intptr_t>(gfx::GetXDisplay());
  }

  bool LoadGLES2Bindings(gl::GLImplementation implementation) override {
    return LoadDefaultEGLGLES2Bindings(implementation);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GLOzoneEGLX11);
};

}  // namespace

X11SurfaceFactory::X11SurfaceFactory()
    : glx_implementation_(std::make_unique<GLOzoneGLX>()),
      egl_implementation_(std::make_unique<GLOzoneEGLX11>()),
      osmesa_implementation_(std::make_unique<GLOzoneOSMesa>()) {}

X11SurfaceFactory::~X11SurfaceFactory() {}

std::vector<gl::GLImplementation>
X11SurfaceFactory::GetAllowedGLImplementations() {
  return std::vector<gl::GLImplementation>{
      gl::kGLImplementationDesktopGL, gl::kGLImplementationEGLGLES2,
      gl::kGLImplementationOSMesaGL, gl::kGLImplementationSwiftShaderGL};
}

GLOzone* X11SurfaceFactory::GetGLOzone(gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationDesktopGL:
      return glx_implementation_.get();
    case gl::kGLImplementationEGLGLES2:
    case gl::kGLImplementationSwiftShaderGL:
      return egl_implementation_.get();
    case gl::kGLImplementationOSMesaGL:
      return osmesa_implementation_.get();
    default:
      return nullptr;
  }
}

#if BUILDFLAG(ENABLE_VULKAN)
std::unique_ptr<gpu::VulkanImplementation>
X11SurfaceFactory::CreateVulkanImplementation() {
  return std::make_unique<gpu::VulkanImplementationX11>();
}
#endif

}  // namespace ui
