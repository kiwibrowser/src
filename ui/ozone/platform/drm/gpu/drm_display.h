// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_DISPLAY_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_DISPLAY_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/geometry/point.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"

typedef struct _drmModeModeInfo drmModeModeInfo;

namespace display {
class DisplaySnapshot;
struct GammaRampRGBEntry;
}

namespace ui {

class DrmDevice;
class HardwareDisplayControllerInfo;
class ScreenManager;

class DrmDisplay {
 public:
  DrmDisplay(ScreenManager* screen_manager,
             const scoped_refptr<DrmDevice>& drm);
  ~DrmDisplay();

  int64_t display_id() const { return display_id_; }
  scoped_refptr<DrmDevice> drm() const { return drm_; }
  uint32_t crtc() const { return crtc_; }
  uint32_t connector() const { return connector_; }
  const std::vector<drmModeModeInfo>& modes() const { return modes_; }

  std::unique_ptr<display::DisplaySnapshot> Update(
      HardwareDisplayControllerInfo* info,
      size_t device_index);

  bool Configure(const drmModeModeInfo* mode, const gfx::Point& origin);
  bool GetHDCPState(display::HDCPState* state);
  bool SetHDCPState(display::HDCPState state);
  void SetColorCorrection(
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix);

 private:
  ScreenManager* screen_manager_;  // Not owned.

  int64_t display_id_ = -1;
  scoped_refptr<DrmDevice> drm_;
  uint32_t crtc_ = 0;
  uint32_t connector_ = 0;
  std::vector<drmModeModeInfo> modes_;
  gfx::Point origin_;

  DISALLOW_COPY_AND_ASSIGN(DrmDisplay);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_DISPLAY_H_
