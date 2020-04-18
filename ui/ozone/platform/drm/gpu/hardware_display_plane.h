// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_H_
#define UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_H_

#include <stddef.h>
#include <stdint.h>

#include <xf86drmMode.h>

#include <vector>

#include "base/macros.h"
#include "ui/ozone/platform/drm/common/scoped_drm_types.h"

namespace ui {

class DrmDevice;

class HardwareDisplayPlane {
 public:
  enum Type { kDummy, kPrimary, kOverlay, kCursor };

  HardwareDisplayPlane(uint32_t plane_id, uint32_t possible_crtcs);

  virtual ~HardwareDisplayPlane();

  bool Initialize(DrmDevice* drm,
                  const std::vector<uint32_t>& formats,
                  const std::vector<drm_format_modifier>& format_info,
                  bool is_dummy,
                  bool test_only);

  bool IsSupportedFormat(uint32_t format);

  std::vector<uint64_t> ModifiersForFormat(uint32_t format);

  bool CanUseForCrtc(uint32_t crtc_index);

  bool in_use() const { return in_use_; }
  void set_in_use(bool in_use) { in_use_ = in_use; }

  uint32_t plane_id() const { return plane_id_; }

  Type type() const { return type_; }
  void set_type(const Type type) { type_ = type; }

  void set_owning_crtc(uint32_t crtc) { owning_crtc_ = crtc; }
  uint32_t owning_crtc() const { return owning_crtc_; }

  const std::vector<uint32_t>& supported_formats() const;

 protected:
  virtual bool InitializeProperties(
      DrmDevice* drm,
      const ScopedDrmObjectPropertyPtr& plane_props);

  uint32_t plane_id_ = 0;
  uint32_t possible_crtcs_ = 0;
  uint32_t owning_crtc_ = 0;
  uint32_t last_used_format_ = 0;
  bool in_use_ = false;
  Type type_ = kPrimary;
  std::vector<uint32_t> supported_formats_;
  std::vector<drm_format_modifier> supported_format_modifiers_;

  DISALLOW_COPY_AND_ASSIGN(HardwareDisplayPlane);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_HARDWARE_DISPLAY_PLANE_H_
