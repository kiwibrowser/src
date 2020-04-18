// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/headless/headless_surface_factory.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/ozone/common/gl_ozone_osmesa.h"
#include "ui/ozone/platform/headless/gl_surface_osmesa_png.h"
#include "ui/ozone/platform/headless/headless_window.h"
#include "ui/ozone/platform/headless/headless_window_manager.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

namespace ui {

namespace {

const base::FilePath::CharType kDevNull[] = FILE_PATH_LITERAL("/dev/null");

void WriteDataToFile(const base::FilePath& location, const SkBitmap& bitmap) {
  DCHECK(!location.empty());
  std::vector<unsigned char> png_data;
  gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, true, &png_data);
  base::WriteFile(location, reinterpret_cast<const char*>(png_data.data()),
                  png_data.size());
}

// TODO(altimin): Find a proper way to capture rendering output.
class FileSurface : public SurfaceOzoneCanvas {
 public:
  explicit FileSurface(const base::FilePath& location) : base_path_(location) {}
  ~FileSurface() override {}

  // SurfaceOzoneCanvas overrides:
  void ResizeCanvas(const gfx::Size& viewport_size) override {
    surface_ = SkSurface::MakeRaster(SkImageInfo::MakeN32Premul(
        viewport_size.width(), viewport_size.height()));
  }
  sk_sp<SkSurface> GetSurface() override { return surface_; }
  void PresentCanvas(const gfx::Rect& damage) override {
    if (base_path_.empty())
      return;
    SkBitmap bitmap;
    bitmap.allocPixels(surface_->getCanvas()->imageInfo());

    // TODO(dnicoara) Use SkImage instead to potentially avoid a copy.
    // See crbug.com/361605 for details.
    if (surface_->getCanvas()->readPixels(bitmap, 0, 0)) {
      base::PostTaskWithTraits(
          FROM_HERE,
          {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
          base::BindOnce(&WriteDataToFile, base_path_, bitmap));
    }
  }
  std::unique_ptr<gfx::VSyncProvider> CreateVSyncProvider() override {
    return nullptr;
  }

 private:
  base::FilePath base_path_;
  sk_sp<SkSurface> surface_;
};

class TestPixmap : public gfx::NativePixmap {
 public:
  explicit TestPixmap(gfx::BufferFormat format) : format_(format) {}

  void* GetEGLClientBuffer() const override { return nullptr; }
  bool AreDmaBufFdsValid() const override { return false; }
  size_t GetDmaBufFdCount() const override { return 0; }
  int GetDmaBufFd(size_t plane) const override { return -1; }
  int GetDmaBufPitch(size_t plane) const override { return 0; }
  int GetDmaBufOffset(size_t plane) const override { return 0; }
  uint64_t GetDmaBufModifier(size_t plane) const override { return 0; }
  gfx::BufferFormat GetBufferFormat() const override { return format_; }
  gfx::Size GetBufferSize() const override { return gfx::Size(); }
  uint32_t GetUniqueId() const override { return 0; }
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int plane_z_order,
                            gfx::OverlayTransform plane_transform,
                            const gfx::Rect& display_bounds,
                            const gfx::RectF& crop_rect,
                            bool enable_blend,
                            gfx::GpuFence* gpu_fence) override {
    return true;
  }
  gfx::NativePixmapHandle ExportHandle() override {
    return gfx::NativePixmapHandle();
  }

 private:
  ~TestPixmap() override {}

  gfx::BufferFormat format_;

  DISALLOW_COPY_AND_ASSIGN(TestPixmap);
};

class GLOzoneOSMesaHeadless : public GLOzoneOSMesa {
 public:
  explicit GLOzoneOSMesaHeadless(HeadlessSurfaceFactory* surface_factory)
      : surface_factory_(surface_factory) {}

  ~GLOzoneOSMesaHeadless() override = default;

  // GLOzone:
  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override {
    return gl::InitializeGLSurface(
        new GLSurfaceOSMesaPng(surface_factory_->GetPathForWidget(window)));
  }

 private:
  HeadlessSurfaceFactory* const surface_factory_;

  DISALLOW_COPY_AND_ASSIGN(GLOzoneOSMesaHeadless);
};

}  // namespace

HeadlessSurfaceFactory::HeadlessSurfaceFactory(base::FilePath base_path)
    : base_path_(base_path) {
  CheckBasePath();
  osmesa_implementation_ = std::make_unique<GLOzoneOSMesaHeadless>(this);
}

HeadlessSurfaceFactory::~HeadlessSurfaceFactory() = default;

base::FilePath HeadlessSurfaceFactory::GetPathForWidget(
    gfx::AcceleratedWidget widget) {
  if (base_path_.empty() || base_path_ == base::FilePath(kDevNull))
    return base_path_;

  // Disambiguate multiple window output files with the window id.
#if defined(OS_WIN)
  std::string path = base::IntToString(reinterpret_cast<int>(widget)) + ".png";
  std::wstring wpath(path.begin(), path.end());
  return base_path_.Append(wpath);
#else
  return base_path_.Append(base::IntToString(widget) + ".png");
#endif
}

std::vector<gl::GLImplementation>
HeadlessSurfaceFactory::GetAllowedGLImplementations() {
  return std::vector<gl::GLImplementation>{gl::kGLImplementationOSMesaGL};
}

GLOzone* HeadlessSurfaceFactory::GetGLOzone(
    gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationOSMesaGL:
      return osmesa_implementation_.get();
    default:
      return nullptr;
  }
}

std::unique_ptr<SurfaceOzoneCanvas>
HeadlessSurfaceFactory::CreateCanvasForWidget(gfx::AcceleratedWidget widget) {
  return std::make_unique<FileSurface>(GetPathForWidget(widget));
}

scoped_refptr<gfx::NativePixmap> HeadlessSurfaceFactory::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return new TestPixmap(format);
}

void HeadlessSurfaceFactory::CheckBasePath() const {
  if (base_path_.empty())
    return;

  if (!DirectoryExists(base_path_) && !base::CreateDirectory(base_path_) &&
      base_path_ != base::FilePath(kDevNull))
    PLOG(FATAL) << "Unable to create output directory";

  if (!base::PathIsWritable(base_path_))
    PLOG(FATAL) << "Unable to write to output location";
}

}  // namespace ui
