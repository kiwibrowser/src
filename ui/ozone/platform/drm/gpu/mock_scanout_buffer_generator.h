// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_GENERATOR_H_
#define UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_GENERATOR_H_

#include "base/macros.h"

#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

class MockScanoutBufferGenerator : public ScanoutBufferGenerator {
 public:
  MockScanoutBufferGenerator();
  ~MockScanoutBufferGenerator() override;

  // ScanoutBufferGenerator:
  scoped_refptr<ScanoutBuffer> Create(const scoped_refptr<DrmDevice>& drm,
                                      uint32_t format,
                                      const std::vector<uint64_t>& modifiers,
                                      const gfx::Size& size) override;

  scoped_refptr<ScanoutBuffer> CreateWithModifier(
      const scoped_refptr<DrmDevice>& drm,
      uint32_t format,
      uint64_t modifier,
      const gfx::Size& size);

  void set_allocation_failure(bool allocation_failure) {
    allocation_failure_ = allocation_failure;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockScanoutBufferGenerator);

  bool allocation_failure_ = false;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_MOCK_SCANOUT_BUFFER_GENERATOR_H_
