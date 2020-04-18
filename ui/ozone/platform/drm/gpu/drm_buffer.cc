// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_buffer.h"

#include <drm_fourcc.h>

#include "base/logging.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"

namespace ui {

namespace {

uint32_t GetFourCCCodeForSkColorType(SkColorType type) {
  switch (type) {
    case kUnknown_SkColorType:
    case kAlpha_8_SkColorType:
      return 0;
    case kRGB_565_SkColorType:
      return DRM_FORMAT_RGB565;
    case kARGB_4444_SkColorType:
      return DRM_FORMAT_ARGB4444;
    case kN32_SkColorType:
      return DRM_FORMAT_ARGB8888;
    default:
      NOTREACHED();
      return 0;
  }
}

}  // namespace

DrmBuffer::DrmBuffer(const scoped_refptr<DrmDevice>& drm) : drm_(drm) {
}

DrmBuffer::~DrmBuffer() {
  if (framebuffer_ && !drm_->RemoveFramebuffer(framebuffer_))
    PLOG(ERROR) << "DrmBuffer: RemoveFramebuffer: fb " << framebuffer_;

  if (mmap_base_ && !drm_->UnmapDumbBuffer(mmap_base_, mmap_size_))
    PLOG(ERROR) << "DrmBuffer: UnmapDumbBuffer: handle " << handle_;

  if (handle_ && !drm_->DestroyDumbBuffer(handle_))
    PLOG(ERROR) << "DrmBuffer: DestroyDumbBuffer: handle " << handle_;
}

bool DrmBuffer::Initialize(const SkImageInfo& info,
                           bool should_register_framebuffer) {
  if (!drm_->CreateDumbBuffer(info, &handle_, &stride_)) {
    PLOG(ERROR) << "DrmBuffer: CreateDumbBuffer: width " << info.width()
                << " height " << info.height();
    return false;
  }

  mmap_size_ = info.computeByteSize(stride_);
  if (!drm_->MapDumbBuffer(handle_, mmap_size_, &mmap_base_)) {
    PLOG(ERROR) << "DrmBuffer: MapDumbBuffer: handle " << handle_;
    return false;
  }

  if (should_register_framebuffer) {
    uint32_t handles[4] = {0};
    handles[0] = handle_;
    uint32_t strides[4] = {0};
    strides[0] = stride_;
    uint32_t offsets[4] = {0};
    fb_pixel_format_ = GetFourCCCodeForSkColorType(info.colorType());
    if (!drm_->AddFramebuffer2(info.width(), info.height(), fb_pixel_format_,
                               handles, strides, offsets, nullptr,
                               &framebuffer_, 0)) {
      PLOG(ERROR) << "DrmBuffer: AddFramebuffer2: handle " << handle_;
      return false;
    }
  }

  surface_ = SkSurface::MakeRasterDirect(info, mmap_base_, stride_);
  if (!surface_) {
    LOG(ERROR) << "DrmBuffer: Failed to create SkSurface: handle " << handle_;
    return false;
  }

  return true;
}

SkCanvas* DrmBuffer::GetCanvas() const {
  return surface_->getCanvas();
}

uint32_t DrmBuffer::GetFramebufferId() const {
  return framebuffer_;
}

uint32_t DrmBuffer::GetFramebufferPixelFormat() const {
  return fb_pixel_format_;
}

uint32_t DrmBuffer::GetOpaqueFramebufferId() const {
  return framebuffer_;
}

uint32_t DrmBuffer::GetOpaqueFramebufferPixelFormat() const {
  return fb_pixel_format_;
}

uint64_t DrmBuffer::GetFormatModifier() const {
  return DRM_FORMAT_MOD_NONE;
}

uint32_t DrmBuffer::GetHandle() const {
  return handle_;
}

gfx::Size DrmBuffer::GetSize() const {
  return gfx::Size(surface_->width(), surface_->height());
}

const DrmDevice* DrmBuffer::GetDrmDevice() const {
  return drm_.get();
}

bool DrmBuffer::RequiresGlFinish() const {
  return false;
}

}  // namespace ui
