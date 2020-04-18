// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/mock_scanout_buffer.h"
#include "ui/ozone/platform/drm/gpu/mock_drm_device.h"

namespace ui {

namespace {

uint32_t g_current_framebuffer_id = 1;

}  // namespace

MockScanoutBuffer::MockScanoutBuffer(const gfx::Size& size,
                                     uint32_t format,
                                     uint64_t modifier,
                                     const scoped_refptr<DrmDevice>& drm)
    : size_(size),
      format_(format),
      modifier_(modifier),
      id_(g_current_framebuffer_id++),
      opaque_id_(g_current_framebuffer_id++),
      drm_(drm) {}

MockScanoutBuffer::~MockScanoutBuffer() {}

uint32_t MockScanoutBuffer::GetFramebufferId() const {
  return id_;
}

uint32_t MockScanoutBuffer::GetOpaqueFramebufferId() const {
  return opaque_id_;
}

uint32_t MockScanoutBuffer::GetHandle() const {
  return 0;
}

gfx::Size MockScanoutBuffer::GetSize() const {
  return size_;
}

uint32_t MockScanoutBuffer::GetFramebufferPixelFormat() const {
  return format_;
}

uint32_t MockScanoutBuffer::GetOpaqueFramebufferPixelFormat() const {
  return format_;
}

uint64_t MockScanoutBuffer::GetFormatModifier() const {
  return modifier_;
}

const DrmDevice* MockScanoutBuffer::GetDrmDevice() const {
  return drm_.get();
}

bool MockScanoutBuffer::RequiresGlFinish() const {
  return false;
}

}  // namespace ui
