// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_FAKE_PLANE_INFO_H_
#define UI_OZONE_PLATFORM_DRM_GPU_FAKE_PLANE_INFO_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"

namespace ui {

struct FakePlaneInfo {
  FakePlaneInfo(uint32_t plane_id, uint32_t crtc_mask);
  FakePlaneInfo(uint32_t plane_id,
                uint32_t crtc_mask,
                const std::vector<uint32_t>& formats);
  FakePlaneInfo(const FakePlaneInfo& other);
  ~FakePlaneInfo();

  uint32_t id;
  uint32_t allowed_crtc_mask;
  std::vector<uint32_t> allowed_formats;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_FAKE_PLANE_INFO_H_
