// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/cast/surface_factory_cast.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chromecast/public/cast_egl_platform.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/ozone/common/gl_ozone_osmesa.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

namespace ui {

namespace {

class DummySurface : public SurfaceOzoneCanvas {
 public:
  DummySurface() {}
  ~DummySurface() override {}

  // SurfaceOzoneCanvas implementation:
  sk_sp<SkSurface> GetSurface() override { return surface_; }

  void ResizeCanvas(const gfx::Size& viewport_size) override {
    surface_ =
        SkSurface::MakeNull(viewport_size.width(), viewport_size.height());
  }

  void PresentCanvas(const gfx::Rect& damage) override {}

  std::unique_ptr<gfx::VSyncProvider> CreateVSyncProvider() override {
    return nullptr;
  }

 private:
  sk_sp<SkSurface> surface_;

  DISALLOW_COPY_AND_ASSIGN(DummySurface);
};

class CastPixmap : public gfx::NativePixmap {
 public:
  explicit CastPixmap(GLOzoneEglCast* parent) : parent_(parent) {}

  void* GetEGLClientBuffer() const override {
    // TODO(halliwell): try to implement this through CastEglPlatform.
    return nullptr;
  }
  bool AreDmaBufFdsValid() const override { return false; }
  size_t GetDmaBufFdCount() const override { return 0; }
  int GetDmaBufFd(size_t plane) const override { return -1; }
  int GetDmaBufPitch(size_t plane) const override { return 0; }
  int GetDmaBufOffset(size_t plane) const override { return 0; }
  uint64_t GetDmaBufModifier(size_t plane) const override { return 0; }
  gfx::BufferFormat GetBufferFormat() const override {
    return gfx::BufferFormat::BGRA_8888;
  }
  gfx::Size GetBufferSize() const override { return gfx::Size(); }
  uint32_t GetUniqueId() const override { return 0; }

  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int plane_z_order,
                            gfx::OverlayTransform plane_transform,
                            const gfx::Rect& display_bounds,
                            const gfx::RectF& crop_rect,
                            bool enable_blend,
                            gfx::GpuFence* gpu_fence) override {
    parent_->OnOverlayScheduled(display_bounds);
    return true;
  }
  gfx::NativePixmapHandle ExportHandle() override {
    return gfx::NativePixmapHandle();
  }

 private:
  ~CastPixmap() override {}

  GLOzoneEglCast* parent_;

  DISALLOW_COPY_AND_ASSIGN(CastPixmap);
};

}  // namespace

SurfaceFactoryCast::SurfaceFactoryCast() : SurfaceFactoryCast(nullptr) {}

SurfaceFactoryCast::SurfaceFactoryCast(
    std::unique_ptr<chromecast::CastEglPlatform> egl_platform)
    : osmesa_implementation_(std::make_unique<GLOzoneOSMesa>()) {
  if (egl_platform) {
    egl_implementation_ =
        std::make_unique<GLOzoneEglCast>(std::move(egl_platform));
  }
}

SurfaceFactoryCast::~SurfaceFactoryCast() {}

std::vector<gl::GLImplementation>
SurfaceFactoryCast::GetAllowedGLImplementations() {
  std::vector<gl::GLImplementation> impls;
  if (egl_implementation_)
    impls.push_back(gl::kGLImplementationEGLGLES2);
  impls.push_back(gl::kGLImplementationOSMesaGL);
  return impls;
}

GLOzone* SurfaceFactoryCast::GetGLOzone(gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationEGLGLES2:
      return egl_implementation_.get();
    case gl::kGLImplementationOSMesaGL:
      return osmesa_implementation_.get();
    default:
      return nullptr;
  }
}

std::unique_ptr<SurfaceOzoneCanvas> SurfaceFactoryCast::CreateCanvasForWidget(
    gfx::AcceleratedWidget widget) {
  // Software canvas support only in headless mode
  if (egl_implementation_)
    return nullptr;
  return base::WrapUnique<SurfaceOzoneCanvas>(new DummySurface());
}

scoped_refptr<gfx::NativePixmap> SurfaceFactoryCast::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return base::MakeRefCounted<CastPixmap>(egl_implementation_.get());
}

}  // namespace ui
