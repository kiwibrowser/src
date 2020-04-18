// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/public/surface_factory_ozone.h"

#include <stdlib.h>

#include "base/command_line.h"
#include "gpu/vulkan/buildflags.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "gpu/vulkan/vulkan_instance.h"
#endif

namespace ui {

SurfaceFactoryOzone::SurfaceFactoryOzone() {}

SurfaceFactoryOzone::~SurfaceFactoryOzone() {}

std::vector<gl::GLImplementation>
SurfaceFactoryOzone::GetAllowedGLImplementations() {
  return std::vector<gl::GLImplementation>();
}

GLOzone* SurfaceFactoryOzone::GetGLOzone(gl::GLImplementation implementation) {
  return nullptr;
}

#if BUILDFLAG(ENABLE_VULKAN)
std::unique_ptr<gpu::VulkanImplementation>
SurfaceFactoryOzone::CreateVulkanImplementation() {
  return nullptr;
}
#endif

std::unique_ptr<SurfaceOzoneCanvas> SurfaceFactoryOzone::CreateCanvasForWidget(
    gfx::AcceleratedWidget widget) {
  return nullptr;
}

std::vector<gfx::BufferFormat> SurfaceFactoryOzone::GetScanoutFormats(
    gfx::AcceleratedWidget widget) {
  return std::vector<gfx::BufferFormat>();
}

scoped_refptr<gfx::NativePixmap> SurfaceFactoryOzone::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return nullptr;
}

scoped_refptr<gfx::NativePixmap>
SurfaceFactoryOzone::CreateNativePixmapFromHandle(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    const gfx::NativePixmapHandle& handle) {
  return nullptr;
}

scoped_refptr<gfx::NativePixmap>
SurfaceFactoryOzone::CreateNativePixmapForProtectedBufferHandle(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    const gfx::NativePixmapHandle& handle) {
  return nullptr;
}

void SurfaceFactoryOzone::SetGetProtectedNativePixmapDelegate(
    const GetProtectedNativePixmapCallback&
        get_protected_native_pixmap_callback) {}

}  // namespace ui
