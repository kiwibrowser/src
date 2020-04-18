// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/linux/native_pixmap_dmabuf.h"

#include "base/posix/eintr_wrapper.h"

namespace gfx {

NativePixmapDmaBuf::NativePixmapDmaBuf(const gfx::Size& size,
                                       gfx::BufferFormat format,
                                       const gfx::NativePixmapHandle& handle)
    : size_(size), format_(format), planes_(handle.planes) {
  for (auto& fd : handle.fds) {
    fds_.emplace_back(fd.fd);
  }
}

NativePixmapDmaBuf::~NativePixmapDmaBuf() {}

void* NativePixmapDmaBuf::GetEGLClientBuffer() const {
  return nullptr;
}

bool NativePixmapDmaBuf::AreDmaBufFdsValid() const {
  if (fds_.empty())
    return false;

  for (const auto& fd : fds_) {
    if (!fd.is_valid())
      return false;
  }
  return true;
}

size_t NativePixmapDmaBuf::GetDmaBufFdCount() const {
  return fds_.size();
}

int NativePixmapDmaBuf::GetDmaBufFd(size_t plane) const {
  DCHECK_LT(plane, fds_.size());
  return fds_[plane].get();
}

int NativePixmapDmaBuf::GetDmaBufPitch(size_t plane) const {
  DCHECK_LT(plane, planes_.size());
  return planes_[plane].stride;
}

int NativePixmapDmaBuf::GetDmaBufOffset(size_t plane) const {
  DCHECK_LT(plane, planes_.size());
  return planes_[plane].offset;
}

uint64_t NativePixmapDmaBuf::GetDmaBufModifier(size_t plane) const {
  DCHECK_LT(plane, planes_.size());
  return planes_[plane].modifier;
}

gfx::BufferFormat NativePixmapDmaBuf::GetBufferFormat() const {
  return format_;
}

gfx::Size NativePixmapDmaBuf::GetBufferSize() const {
  return size_;
}

uint32_t NativePixmapDmaBuf::GetUniqueId() const {
  return 0;
}

bool NativePixmapDmaBuf::ScheduleOverlayPlane(
    gfx::AcceleratedWidget widget,
    int plane_z_order,
    gfx::OverlayTransform plane_transform,
    const gfx::Rect& display_bounds,
    const gfx::RectF& crop_rect,
    bool enable_blend,
    gfx::GpuFence* gpu_fence) {
  return false;
}

gfx::NativePixmapHandle NativePixmapDmaBuf::ExportHandle() {
  return gfx::NativePixmapHandle();
}

}  // namespace gfx
