// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_ATOMIC_H_
#define UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_ATOMIC_H_

#include "ui/gfx/overlay_transform.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_plane.h"

#include <stdint.h>
#include <xf86drmMode.h>

namespace gfx {
class Rect;
}  // namespace gfx

namespace ui {

class CrtcController;
class DrmDevice;

class HardwareDisplayPlaneAtomic : public HardwareDisplayPlane {
 public:
  HardwareDisplayPlaneAtomic(uint32_t plane_id, uint32_t possible_crtcs);
  ~HardwareDisplayPlaneAtomic() override;

  virtual bool SetPlaneData(drmModeAtomicReq* property_set,
                            uint32_t crtc_id,
                            uint32_t framebuffer,
                            const gfx::Rect& crtc_rect,
                            const gfx::Rect& src_rect,
                            const gfx::OverlayTransform transform,
                            int in_fence_fd);

  void set_crtc(CrtcController* crtc) { crtc_ = crtc; }
  CrtcController* crtc() const { return crtc_; }

 private:
  bool InitializeProperties(
      DrmDevice* drm,
      const ScopedDrmObjectPropertyPtr& plane_props) override;

  // TODO(dnicoara): Merge this with DrmDevice::Property.
  struct Property {
    Property();
    bool Initialize(DrmDevice* drm,
                    const char* name,
                    const ScopedDrmObjectPropertyPtr& plane_properties);
    uint32_t id = 0;
  };

  Property crtc_prop_;
  Property fb_prop_;
  Property crtc_x_prop_;
  Property crtc_y_prop_;
  Property crtc_w_prop_;
  Property crtc_h_prop_;
  Property src_x_prop_;
  Property src_y_prop_;
  Property src_w_prop_;
  Property src_h_prop_;
  Property rotation_prop_;
  Property in_fence_fd_prop_;
  CrtcController* crtc_ = nullptr;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_ATOMIC_H_
