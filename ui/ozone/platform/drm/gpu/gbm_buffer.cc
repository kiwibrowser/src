// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/gbm_buffer.h"

#include <drm.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <xf86drm.h>
#include <utility>

#include "base/files/platform_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/native_pixmap_handle.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"
#include "ui/ozone/platform/drm/gpu/gbm_device.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"
#include "ui/ozone/platform/drm/gpu/gbm_surfaceless.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"

namespace ui {

GbmBuffer::GbmBuffer(const scoped_refptr<GbmDevice>& gbm,
                     gbm_bo* bo,
                     uint32_t format,
                     uint32_t flags,
                     uint64_t modifier,
                     std::vector<base::ScopedFD>&& fds,
                     const gfx::Size& size,

                     const std::vector<gfx::NativePixmapPlane>&& planes)
    : drm_(gbm),
      bo_(bo),
      format_modifier_(modifier),
      format_(format),
      flags_(flags),
      fds_(std::move(fds)),
      size_(size),
      planes_(std::move(planes)) {
  if (flags & GBM_BO_USE_SCANOUT) {
    DCHECK(bo_);
    framebuffer_pixel_format_ = format;
    opaque_framebuffer_pixel_format_ = GetFourCCFormatForOpaqueFramebuffer(
        GetBufferFormatFromFourCCFormat(format));

    uint32_t handles[4] = {0};
    uint32_t strides[4] = {0};
    uint32_t offsets[4] = {0};
    uint64_t modifiers[4] = {0};

    for (size_t i = 0; i < gbm_bo_get_num_planes(bo); ++i) {
      handles[i] = gbm_bo_get_plane_handle(bo, i).u32;
      strides[i] = gbm_bo_get_plane_stride(bo, i);
      offsets[i] = gbm_bo_get_plane_offset(bo, i);
      if (modifier != DRM_FORMAT_MOD_INVALID)
        modifiers[i] = modifier;
    }

    // AddFramebuffer2 only considers the modifiers if addfb_flags has
    // DRM_MODE_FB_MODIFIERS set. We only set that when we've created
    // a bo with modifiers, otherwise, we rely on the "no modifiers"
    // behavior doing the right thing.
    const uint32_t addfb_flags =
        gbm->allow_addfb2_modifiers() &&
        modifier != DRM_FORMAT_MOD_INVALID ? DRM_MODE_FB_MODIFIERS : 0;
    bool ret = drm_->AddFramebuffer2(
        gbm_bo_get_width(bo), gbm_bo_get_height(bo), framebuffer_pixel_format_,
        handles, strides, offsets, modifiers, &framebuffer_, addfb_flags);
    PLOG_IF(ERROR, !ret) << "AddFramebuffer2 failed";

    if (opaque_framebuffer_pixel_format_ != framebuffer_pixel_format_) {
      ret = drm_->AddFramebuffer2(gbm_bo_get_width(bo), gbm_bo_get_height(bo),
                                  opaque_framebuffer_pixel_format_, handles,
                                  strides, offsets, modifiers,
                                  &opaque_framebuffer_, addfb_flags);
      PLOG_IF(ERROR, !ret) << "AddFramebuffer2 failed";
    }
  }
}

GbmBuffer::~GbmBuffer() {
  if (framebuffer_)
    drm_->RemoveFramebuffer(framebuffer_);
  if (opaque_framebuffer_)
    drm_->RemoveFramebuffer(opaque_framebuffer_);
  if (bo())
    gbm_bo_destroy(bo());
}

bool GbmBuffer::AreFdsValid() const {
  if (fds_.empty())
    return false;

  for (const auto& fd : fds_) {
    if (fd.get() == -1)
      return false;
  }
  return true;
}

size_t GbmBuffer::GetFdCount() const {
  return fds_.size();
}

int GbmBuffer::GetFd(size_t index) const {
  DCHECK_LT(index, fds_.size());
  return fds_[index].get();
}

int GbmBuffer::GetStride(size_t index) const {
  DCHECK_LT(index, planes_.size());
  return planes_[index].stride;
}

int GbmBuffer::GetOffset(size_t index) const {
  DCHECK_LT(index, planes_.size());
  return planes_[index].offset;
}

size_t GbmBuffer::GetSize(size_t index) const {
  DCHECK_LT(index, planes_.size());
  return planes_[index].size;
}

uint32_t GbmBuffer::GetFramebufferId() const {
  return framebuffer_;
}

uint32_t GbmBuffer::GetOpaqueFramebufferId() const {
  return opaque_framebuffer_ ? opaque_framebuffer_ : framebuffer_;
}

uint32_t GbmBuffer::GetHandle() const {
  return bo() ? gbm_bo_get_handle(bo()).u32 : 0;
}

// TODO(reveman): This should not be needed once crbug.com/597932 is fixed,
// as the size would be queried directly from the underlying bo.
gfx::Size GbmBuffer::GetSize() const {
  return size_;
}

uint32_t GbmBuffer::GetFramebufferPixelFormat() const {
  DCHECK(framebuffer_);
  return framebuffer_pixel_format_;
}

uint32_t GbmBuffer::GetOpaqueFramebufferPixelFormat() const {
  DCHECK(framebuffer_);
  return opaque_framebuffer_pixel_format_;
}

uint64_t GbmBuffer::GetFormatModifier() const {
  return format_modifier_;
}

const DrmDevice* GbmBuffer::GetDrmDevice() const {
  return drm_.get();
}

bool GbmBuffer::RequiresGlFinish() const {
  return !drm_->is_primary_device();
}

scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferForBO(
    const scoped_refptr<GbmDevice>& gbm,
    gbm_bo* bo,
    uint32_t format,
    const gfx::Size& size,
    uint32_t flags) {
  DCHECK(bo);
  std::vector<base::ScopedFD> fds;
  std::vector<gfx::NativePixmapPlane> planes;

  const uint64_t modifier = gbm_bo_get_format_modifier(bo);
  for (size_t i = 0; i < gbm_bo_get_num_planes(bo); ++i) {
    // The fd returned by gbm_bo_get_fd is not ref-counted and need to be
    // kept open for the lifetime of the buffer.
    base::ScopedFD fd(gbm_bo_get_plane_fd(bo, i));

    // TODO(dcastagna): support multiple fds.
    // crbug.com/642410
    if (!i) {
      if (!fd.is_valid()) {
        PLOG(ERROR) << "Failed to export buffer to dma_buf";
        gbm_bo_destroy(bo);
        return nullptr;
      }
      fds.emplace_back(std::move(fd));
    }

    planes.emplace_back(gbm_bo_get_plane_stride(bo, i),
                        gbm_bo_get_plane_offset(bo, i),
                        gbm_bo_get_plane_size(bo, i), modifier);
  }
  scoped_refptr<GbmBuffer> buffer(new GbmBuffer(gbm, bo, format, flags,
                                                modifier, std::move(fds), size,
                                                std::move(planes)));
  if (flags & GBM_BO_USE_SCANOUT && !buffer->GetFramebufferId())
    return nullptr;

  return buffer;
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferWithModifiers(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    uint32_t flags,
    const std::vector<uint64_t>& modifiers) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBufferWithModifiers", "device",
               gbm->device_path().value(), "size", size.ToString());

  gbm_bo* bo =
      gbm_bo_create_with_modifiers(gbm->device(), size.width(), size.height(),
                                   format, modifiers.data(), modifiers.size());
  if (!bo)
    return nullptr;

  return CreateBufferForBO(gbm, bo, format, size, flags);
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBuffer(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    uint32_t flags) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBuffer", "device",
               gbm->device_path().value(), "size", size.ToString());

  gbm_bo* bo =
      gbm_bo_create(gbm->device(), size.width(), size.height(), format, flags);
  if (!bo)
    return nullptr;

  return CreateBufferForBO(gbm, bo, format, size, flags);
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferFromFds(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    std::vector<base::ScopedFD>&& fds,
    const std::vector<gfx::NativePixmapPlane>& planes) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBufferFromFD", "device",
               gbm->device_path().value(), "size", size.ToString());
  DCHECK_LE(fds.size(), planes.size());
  DCHECK_EQ(planes[0].offset, 0);

  // Try to use scanout if supported.
  int gbm_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_TEXTURING;
  if (!gbm_device_is_format_supported(gbm->device(), format, gbm_flags))
    gbm_flags &= ~GBM_BO_USE_SCANOUT;

  gbm_bo* bo = nullptr;
  if (gbm_device_is_format_supported(gbm->device(), format, gbm_flags)) {
    struct gbm_import_fd_planar_data fd_data;
    fd_data.width = size.width();
    fd_data.height = size.height();
    fd_data.format = format;

    DCHECK_LE(planes.size(), 3u);
    for (size_t i = 0; i < planes.size(); ++i) {
      fd_data.fds[i] = fds[i < fds.size() ? i : 0].get();
      fd_data.strides[i] = planes[i].stride;
      fd_data.offsets[i] = planes[i].offset;
      fd_data.format_modifiers[i] = planes[i].modifier;
    }

    // The fd passed to gbm_bo_import is not ref-counted and need to be
    // kept open for the lifetime of the buffer.
    bo = gbm_bo_import(gbm->device(), GBM_BO_IMPORT_FD_PLANAR, &fd_data,
                       gbm_flags);
    if (!bo) {
      LOG(ERROR) << "nullptr returned from gbm_bo_import";
      return nullptr;
    }
  }

  scoped_refptr<GbmBuffer> buffer(
      new GbmBuffer(gbm, bo, format, gbm_flags, planes[0].modifier,
                    std::move(fds), size, std::move(planes)));

  return buffer;
}

GbmPixmap::GbmPixmap(GbmSurfaceFactory* surface_manager,
                     const scoped_refptr<GbmBuffer>& buffer)
    : surface_manager_(surface_manager), buffer_(buffer) {}

gfx::NativePixmapHandle GbmPixmap::ExportHandle() {
  gfx::NativePixmapHandle handle;
  gfx::BufferFormat format =
      ui::GetBufferFormatFromFourCCFormat(buffer_->GetFormat());
  // TODO(dcastagna): Use gbm_bo_get_num_planes once all the formats we use are
  // supported by gbm.
  for (size_t i = 0; i < gfx::NumberOfPlanesForBufferFormat(format); ++i) {
    // Some formats (e.g: YVU_420) might have less than one fd per plane.
    if (i < buffer_->GetFdCount()) {
      base::ScopedFD scoped_fd(HANDLE_EINTR(dup(buffer_->GetFd(i))));
      if (!scoped_fd.is_valid()) {
        PLOG(ERROR) << "dup";
        return gfx::NativePixmapHandle();
      }
      handle.fds.emplace_back(
          base::FileDescriptor(scoped_fd.release(), true /* auto_close */));
    }
    handle.planes.emplace_back(buffer_->GetStride(i), buffer_->GetOffset(i),
                               buffer_->GetSize(i),
                               buffer_->GetFormatModifier());
  }
  return handle;
}

GbmPixmap::~GbmPixmap() {
}

void* GbmPixmap::GetEGLClientBuffer() const {
  return nullptr;
}

bool GbmPixmap::AreDmaBufFdsValid() const {
  return buffer_->AreFdsValid();
}

size_t GbmPixmap::GetDmaBufFdCount() const {
  return buffer_->GetFdCount();
}

int GbmPixmap::GetDmaBufFd(size_t plane) const {
  return buffer_->GetFd(plane);
}

int GbmPixmap::GetDmaBufPitch(size_t plane) const {
  return buffer_->GetStride(plane);
}

int GbmPixmap::GetDmaBufOffset(size_t plane) const {
  return buffer_->GetOffset(plane);
}

uint64_t GbmPixmap::GetDmaBufModifier(size_t plane) const {
  return buffer_->GetFormatModifier();
}

gfx::BufferFormat GbmPixmap::GetBufferFormat() const {
  return ui::GetBufferFormatFromFourCCFormat(buffer_->GetFormat());
}

gfx::Size GbmPixmap::GetBufferSize() const {
  return buffer_->GetSize();
}

uint32_t GbmPixmap::GetUniqueId() const {
  return buffer_->GetHandle();
}

bool GbmPixmap::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                     int plane_z_order,
                                     gfx::OverlayTransform plane_transform,
                                     const gfx::Rect& display_bounds,
                                     const gfx::RectF& crop_rect,
                                     bool enable_blend,
                                     gfx::GpuFence* gpu_fence) {
  DCHECK(buffer_->GetFlags() & GBM_BO_USE_SCANOUT);
  // |framebuffer_id| might be 0 if AddFramebuffer2 failed, in that case we
  // already logged the error in GbmBuffer ctor. We avoid logging the error
  // here since this method might be called every pageflip.
  if (buffer_->GetFramebufferId()) {
    surface_manager_->GetSurface(widget)->QueueOverlayPlane(
        OverlayPlane(buffer_, plane_z_order, plane_transform, display_bounds,
                     crop_rect, enable_blend, gpu_fence));
  }

  return true;
}

}  // namespace ui
