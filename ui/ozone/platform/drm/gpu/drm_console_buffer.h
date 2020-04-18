// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_CONSOLE_BUFFER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_CONSOLE_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

class SkCanvas;

namespace ui {

class DrmDevice;

// Wrapper for the console buffer. This is the buffer that is allocated by
// default by the system and is used when no application is controlling the
// CRTC. Keeps track of the native properties of the buffer and wraps the pixel
// memory into a SkSurface which can be used to draw into using Skia.
class DrmConsoleBuffer {
 public:
  DrmConsoleBuffer(const scoped_refptr<DrmDevice>& drm, uint32_t framebuffer);
  ~DrmConsoleBuffer();

  SkCanvas* canvas() { return surface_->getCanvas(); }
  sk_sp<SkImage> image() { return surface_->makeImageSnapshot(); }

  // Memory map the backing pixels and wrap them in |surface_|.
  bool Initialize();

 protected:
  scoped_refptr<DrmDevice> drm_;

  // Wrapper around the native pixel memory.
  sk_sp<SkSurface> surface_;

  // Length of a row of pixels.
  uint32_t stride_ = 0;

  // Buffer handle used by the DRM allocator.
  uint32_t handle_ = 0;

  // Buffer ID used by the DRM modesettings API.
  uint32_t framebuffer_ = 0;

  // Memory map base address.
  void* mmap_base_ = nullptr;

  // Memory map size.
  size_t mmap_size_ = 0;

  DISALLOW_COPY_AND_ASSIGN(DrmConsoleBuffer);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_CONSOLE_BUFFER_H_
