// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_FACTORY_H_
#define UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_FACTORY_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/ozone/common/gl_ozone_egl.h"
#include "ui/ozone/public/surface_factory_ozone.h"

namespace ui {

class DrmThreadProxy;
class GbmSurfaceless;

class GbmSurfaceFactory : public SurfaceFactoryOzone {
 public:
  explicit GbmSurfaceFactory(DrmThreadProxy* drm_thread_proxy);
  ~GbmSurfaceFactory() override;

  void RegisterSurface(gfx::AcceleratedWidget widget, GbmSurfaceless* surface);
  void UnregisterSurface(gfx::AcceleratedWidget widget);
  GbmSurfaceless* GetSurface(gfx::AcceleratedWidget widget) const;

  // SurfaceFactoryOzone:
  std::vector<gl::GLImplementation> GetAllowedGLImplementations() override;
  GLOzone* GetGLOzone(gl::GLImplementation implementation) override;

  std::vector<gfx::BufferFormat> GetScanoutFormats(
      gfx::AcceleratedWidget widget) override;
  std::unique_ptr<SurfaceOzoneCanvas> CreateCanvasForWidget(
      gfx::AcceleratedWidget widget) override;
  scoped_refptr<gfx::NativePixmap> CreateNativePixmap(
      gfx::AcceleratedWidget widget,
      gfx::Size size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage) override;
  scoped_refptr<gfx::NativePixmap> CreateNativePixmapFromHandle(
      gfx::AcceleratedWidget widget,
      gfx::Size size,
      gfx::BufferFormat format,
      const gfx::NativePixmapHandle& handle) override;
  void SetGetProtectedNativePixmapDelegate(
      const GetProtectedNativePixmapCallback&
          get_protected_native_pixmap_callback) override;
  scoped_refptr<gfx::NativePixmap> CreateNativePixmapForProtectedBufferHandle(
      gfx::AcceleratedWidget widget,
      gfx::Size size,
      gfx::BufferFormat format,
      const gfx::NativePixmapHandle& handle) override;

 private:
  scoped_refptr<gfx::NativePixmap> CreateNativePixmapFromHandleInternal(
      gfx::AcceleratedWidget widget,
      gfx::Size size,
      gfx::BufferFormat format,
      const gfx::NativePixmapHandle& handle);

  std::unique_ptr<GLOzone> egl_implementation_;
  std::unique_ptr<GLOzone> osmesa_implementation_;

  base::ThreadChecker thread_checker_;

  DrmThreadProxy* drm_thread_proxy_;

  std::map<gfx::AcceleratedWidget, GbmSurfaceless*> widget_to_surface_map_;

  GetProtectedNativePixmapCallback get_protected_native_pixmap_callback_;

  DISALLOW_COPY_AND_ASSIGN(GbmSurfaceFactory);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_GBM_SURFACE_FACTORY_H_
