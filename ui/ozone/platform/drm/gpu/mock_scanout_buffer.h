// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_H_

#include <drm_fourcc.h>
#include <stdint.h>

#include "base/macros.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

class MockScanoutBuffer : public ScanoutBuffer {
 public:
  MockScanoutBuffer(const gfx::Size& size,
                    uint32_t format = DRM_FORMAT_XRGB8888,
                    uint64_t modifier = DRM_FORMAT_MOD_NONE,
                    const scoped_refptr<DrmDevice>& drm = nullptr);

  // ScanoutBuffer:
  uint32_t GetFramebufferId() const override;
  uint32_t GetOpaqueFramebufferId() const override;
  uint32_t GetHandle() const override;
  gfx::Size GetSize() const override;
  uint32_t GetFramebufferPixelFormat() const override;
  uint32_t GetOpaqueFramebufferPixelFormat() const override;
  uint64_t GetFormatModifier() const override;
  const DrmDevice* GetDrmDevice() const override;
  bool RequiresGlFinish() const override;

 private:
  ~MockScanoutBuffer() override;

  gfx::Size size_;
  uint32_t format_;
  uint64_t modifier_;
  uint32_t id_;
  uint32_t opaque_id_;
  scoped_refptr<DrmDevice> drm_;

  DISALLOW_COPY_AND_ASSIGN(MockScanoutBuffer);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_H_
