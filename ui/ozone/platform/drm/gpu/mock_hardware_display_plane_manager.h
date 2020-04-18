// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_MOCK_HARDWARE_DISPLAY_PLANE_MANAGER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_MOCK_HARDWARE_DISPLAY_PLANE_MANAGER_H_

#include <stddef.h>
#include <stdint.h>

#include "ui/ozone/platform/drm/gpu/hardware_display_plane_manager_legacy.h"

namespace ui {
struct FakePlaneInfo;

class MockHardwareDisplayPlaneManager
    : public HardwareDisplayPlaneManagerLegacy {
 public:
  MockHardwareDisplayPlaneManager(DrmDevice* drm,
                                  const std::vector<uint32_t>& crtcs,
                                  uint32_t planes_per_crtc);

  explicit MockHardwareDisplayPlaneManager(DrmDevice* drm);

  ~MockHardwareDisplayPlaneManager() override;

  // Normally we'd use DRM to figure out the controller configuration. But we
  // can't use DRM in unit tests, so we just create a fake configuration.
  void InitForTest(const FakePlaneInfo* planes,
                   size_t count,
                   const std::vector<uint32_t>& crtcs);

  void SetPlaneProperties(const std::vector<FakePlaneInfo>& planes);
  void SetCrtcInfo(const std::vector<uint32_t>& crtcs);

  bool DisableOverlayPlanes(HardwareDisplayPlaneList* plane_list) override;
  bool SetPlaneData(HardwareDisplayPlaneList* plane_list,
                    HardwareDisplayPlane* hw_plane,
                    const OverlayPlane& overlay,
                    uint32_t crtc_id,
                    const gfx::Rect& src_rect,
                    CrtcController* crtc) override;

  int plane_count() const;
  void ResetPlaneCount();

 private:
  int plane_count_ = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_MOCK_HARDWARE_DISPLAY_PLANE_MANAGER_H_
