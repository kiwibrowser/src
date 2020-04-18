// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_MANAGER_LEGACY_H_
#define UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_MANAGER_LEGACY_H_

#include <stdint.h>

#include "base/macros.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_plane_manager.h"

namespace ui {

class HardwareDisplayPlaneManagerLegacy : public HardwareDisplayPlaneManager {
 public:
  HardwareDisplayPlaneManagerLegacy();
  ~HardwareDisplayPlaneManagerLegacy() override;

  // HardwareDisplayPlaneManager:
  bool Commit(HardwareDisplayPlaneList* plane_list,
              bool test_only) override;
  bool DisableOverlayPlanes(HardwareDisplayPlaneList* plane_list) override;

  bool SetColorCorrectionOnAllCrtcPlanes(
      uint32_t crtc_id,
      ScopedDrmColorCtmPtr ctm_blob_data) override;

  bool ValidatePrimarySize(const OverlayPlane& primary,
                           const drmModeModeInfo& mode) override;

  void RequestPlanesReadyCallback(const OverlayPlaneList& planes,
                                  base::OnceClosure callback) override;

 protected:
  bool SetPlaneData(HardwareDisplayPlaneList* plane_list,
                    HardwareDisplayPlane* hw_plane,
                    const OverlayPlane& overlay,
                    uint32_t crtc_id,
                    const gfx::Rect& src_rect,
                    CrtcController* crtc) override;
  bool IsCompatible(HardwareDisplayPlane* plane,
                    const OverlayPlane& overlay,
                    uint32_t crtc_index) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HardwareDisplayPlaneManagerLegacy);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_MANAGER_LEGACY_H_
