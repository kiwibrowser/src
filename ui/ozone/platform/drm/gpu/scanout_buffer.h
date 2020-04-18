// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_SCANOUT_BUFFER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_SCANOUT_BUFFER_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/geometry/size.h"

namespace ui {

class DrmDevice;

// Abstraction for a DRM buffer that can be scanned-out of.
class ScanoutBuffer : public base::RefCountedThreadSafe<ScanoutBuffer> {
 public:
  // ID allocated by the KMS API when the buffer is registered (via the handle).
  virtual uint32_t GetFramebufferId() const = 0;

  // ID allocated if the buffer is also registered with a different pixel format
  // so that it can be scheduled as an opaque buffer.
  virtual uint32_t GetOpaqueFramebufferId() const = 0;

  // Returns FourCC format representing the way pixel data has been encoded in
  // memory for the registered framebuffer. This can be used to check if frame
  // buffer is compatible with a given hardware plane.
  virtual uint32_t GetFramebufferPixelFormat() const = 0;

  // Returns FourCC format that should be used to schedule this buffer for
  // scanout when used as an opaque buffer.
  virtual uint32_t GetOpaqueFramebufferPixelFormat() const = 0;

  // Returns format modifier for buffer.
  virtual uint64_t GetFormatModifier() const = 0;

  // Handle for the buffer. This is received when allocating the buffer.
  virtual uint32_t GetHandle() const = 0;

  // Size of the buffer.
  virtual gfx::Size GetSize() const = 0;

  // Device on which the buffer was created.
  virtual const DrmDevice* GetDrmDevice() const = 0;

  virtual bool RequiresGlFinish() const = 0;

 protected:
  virtual ~ScanoutBuffer() {}

  friend class base::RefCountedThreadSafe<ScanoutBuffer>;
};

class ScanoutBufferGenerator {
 public:
  virtual ~ScanoutBufferGenerator() {}

  virtual scoped_refptr<ScanoutBuffer> Create(
      const scoped_refptr<DrmDevice>& drm,
      uint32_t format,
      const std::vector<uint64_t>& modifiers,
      const gfx::Size& size) = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_SCANOUT_BUFFER_H_
